/*************************************************************************
 *
 * Copyright (c) 2011 Kohei Yoshida
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 ************************************************************************/

#include "ixion/cell_listener_tracker.hpp"
#include "ixion/formula_name_resolver.hpp"
#include "ixion/interface/model_context.hpp"
#include "ixion/cell.hpp"

#include <boost/scoped_ptr.hpp>

#define DEBUG_CELL_LISTENER_TRACKER 0

#if DEBUG_CELL_LISTENER_TRACKER
#include <iostream>
#endif

using namespace std;

namespace ixion {

namespace {

boost::scoped_ptr<ixion::cell_listener_tracker> p_instance;

}

cell_listener_tracker& cell_listener_tracker::get(interface::model_context& cxt)
{
    if (!p_instance)
        p_instance.reset(new cell_listener_tracker(cxt));
    return *p_instance;
}

void cell_listener_tracker::reset()
{
    p_instance.reset(NULL);
}

cell_listener_tracker::cell_listener_tracker(interface::model_context& cxt) :
    m_context(cxt) {}

cell_listener_tracker::~cell_listener_tracker()
{
    // Delete all the listener set instances.
    for_each(m_range_listeners.begin(), m_range_listeners.end(), delete_map_value<range_store_type>());
    for_each(m_cell_listeners.begin(), m_cell_listeners.end(), delete_map_value<cell_store_type>());
}

void cell_listener_tracker::add(const abs_address_t& src, const abs_address_t& dest)
{
#if DEBUG_CELL_LISTENER_TRACKER
    const formula_name_resolver& res = m_context.get_name_resolver();
    __IXION_DEBUG_OUT__ << "adding - cell src: " << res.get_name(src, false) 
        << "  cell dest: " << res.get_name(dest, false) << endl;
#endif
    cell_store_type::iterator itr = m_cell_listeners.find(dest);
    if (itr == m_cell_listeners.end())
    {
        // No container for this src cell yet.  Create one.
        pair<cell_store_type::iterator, bool> r =
            m_cell_listeners.insert(cell_store_type::value_type(dest, new address_set_type));
        if (!r.second)
            throw general_error("failed to insert new address set to cell listener tracker.");
        itr = r.first;
    }
    itr->second->insert(src);
}

void cell_listener_tracker::add(const abs_address_t& cell, const abs_range_t& range)
{
#if DEBUG_CELL_LISTENER_TRACKER
    const formula_name_resolver& res = m_context.get_name_resolver();
    __IXION_DEBUG_OUT__ << "adding - cell: " << res.get_name(cell, false) 
        << "  range: " << res.get_name(range, false) << endl;
#endif
    range_store_type::iterator itr = m_range_listeners.find(range);
    if (itr == m_range_listeners.end())
    {
        // No container for this range yet.  Create one.
        pair<range_store_type::iterator, bool> r =
            m_range_listeners.insert(range_store_type::value_type(range, new address_set_type));
        if (!r.second)
            throw general_error("failed to insert new address set to range listener tracker.");
        itr = r.first;

        // Insert the container to the rectangle set as well (for lookup).
#if DEBUG_CELL_LISTENER_TRACKER
        cout << "x1=" << range.first.column << ",y1=" << range.first.row
            << ",x2=" << (range.last.column+1) << ",y2=" << (range.last.row+1) << ",p=" << itr->second << endl;
#endif
        m_query_set.insert(
            range.first.column, range.first.row, range.last.column+1, range.last.row+1, itr->second);
    }
    itr->second->insert(cell);
}

void cell_listener_tracker::remove(const abs_address_t& src, const abs_address_t& dest)
{
    cell_store_type::iterator itr = m_cell_listeners.find(dest);
    if (itr == m_cell_listeners.end())
        // No listeners for this cell.  Bail out.
        return;

    address_set_type* p = itr->second;
    p->erase(src);
    if (p->empty())
    {
        // This list is empty.  Remove it from the containers and destroy the instance.
        m_cell_listeners.erase(itr);
        delete p;
    }
}

void cell_listener_tracker::remove(const abs_address_t& cell, const abs_range_t& range)
{
#if DEBUG_CELL_LISTENER_TRACKER
    const formula_name_resolver& res = m_context.get_name_resolver();
    __IXION_DEBUG_OUT__ << "removing - cell: " << res.get_name(cell, false) << "  range: " << res.get_name(range, false) << endl;
#endif
    range_store_type::iterator itr = m_range_listeners.find(range);
    if (itr == m_range_listeners.end())
        // No listeners for this range.  Bail out.
        return;

    address_set_type* p = itr->second;
    p->erase(cell);
    if (p->empty())
    {
        // This list is empty.  Remove it from the containers and destroy the instance.
        m_range_listeners.erase(itr);
        m_query_set.remove(p);
        delete p;
    }
}

namespace {

class dirty_cell_inserter : public std::unary_function<cell_listener_tracker::address_set_type*, void>
{
    interface::model_context& m_context;
    dirty_cells_t& m_dirty_cells;
    cell_listener_tracker::address_set_type& m_addrs;
public:
    dirty_cell_inserter(interface::model_context& cxt, dirty_cells_t& dirty_cells, cell_listener_tracker::address_set_type& addrs) :
        m_context(cxt), m_dirty_cells(dirty_cells), m_addrs(addrs) {}

    void operator() (const cell_listener_tracker::address_set_type* p)
    {
        // Add all addresses in this set to the dirty cells list.
        cell_listener_tracker::address_set_type::const_iterator itr = p->begin(), itr_end = p->end();
        for (; itr != itr_end; ++itr)
        {
            const abs_address_t& addr = *itr;
            base_cell* p = m_context.get_cell(addr);
            if (p && p->get_celltype() == celltype_formula)
            {
                // Formula cell exists at this address.
                m_dirty_cells.insert(static_cast<formula_cell*>(p));
                m_addrs.insert(addr);
            }
        }
    }
};

class cell_addr_printer : public std::unary_function<abs_address_t, void>
{
    const formula_name_resolver& m_resolver;
public:
    cell_addr_printer(const formula_name_resolver& resolver) : m_resolver(resolver) {}
    void operator() (const abs_address_t& addr) const
    {
        cout << m_resolver.get_name(addr, false) << " ";
    }
};

}

void cell_listener_tracker::get_all_cell_listeners(
    const abs_address_t& target, dirty_cells_t& listeners) const
{
#if DEBUG_CELL_LISTENER_TRACKER
    const formula_name_resolver& res = m_context.get_name_resolver();
    __IXION_DEBUG_OUT__ << "target cell: " << res.get_name(target, false) << endl;
#endif
    cell_store_type::const_iterator itr = m_cell_listeners.find(target);
    if (itr == m_cell_listeners.end())
        // This target cell has no listeners.
        return;

    const address_set_type& addrs = *itr->second;
    address_set_type::iterator itr2 = addrs.begin(), itr2_end = addrs.end();
    for (; itr2 != itr2_end; ++itr2)
    {
        const abs_address_t& addr = *itr2; // listener cell address
        base_cell* p = m_context.get_cell(addr);
        if (!p || p->get_celltype() != celltype_formula)
            // Referenced cell is empty or not a formula cell.  Ignore this.
            continue;

        formula_cell* fcell = static_cast<formula_cell*>(p);

        if (listeners.count(fcell) == 0)
        {
            // This cell is not yet on the dirty cell list.  Run recursively.
            listeners.insert(fcell);
            get_all_cell_listeners(addr, listeners);
            get_all_range_listeners(addr, listeners);
        }
    }
}

void cell_listener_tracker::get_all_range_listeners(
    const abs_address_t& target, dirty_cells_t& listeners) const
{
#if DEBUG_CELL_LISTENER_TRACKER
    __IXION_DEBUG_OUT__ << get_formula_result_output_separator() << endl;
    __IXION_DEBUG_OUT__ << "get all listeners for target " << m_context.get_name_resolver().get_name(target, false) << endl;
#endif

    address_set_type listeners_addrs; // to keep track of circular references.
    get_all_range_listeners_re(target, target, listeners, listeners_addrs);
}

void cell_listener_tracker::print_cell_listeners(const abs_address_t& target) const
{
    const formula_name_resolver& resolver = m_context.get_name_resolver();
    cout << "The following cells listen to cell " << resolver.get_name(target, false) << endl;
    cell_store_type::const_iterator itr = m_cell_listeners.find(target);
    if (itr == m_cell_listeners.end())
        // No one listens to this target.
        return;

    const address_set_type& addrs = *itr->second;
    address_set_type::iterator itr2 = addrs.begin(), itr2_end = addrs.end();
    for (; itr2 != itr2_end; ++itr2)
        cout << "  cell " << resolver.get_name(*itr2, false) << endl;
}

void cell_listener_tracker::get_all_range_listeners_re(
    const abs_address_t& origin_target, const abs_address_t& target, dirty_cells_t& listeners, address_set_type& listeners_addrs) const
{
#if DEBUG_CELL_LISTENER_TRACKER
    __IXION_DEBUG_OUT__ << "--- begin: target address: " << m_context.get_name_resolver().get_name(target, false) << endl;
#endif
    if (listeners_addrs.count(target))
    {
        // Target is included in the listener list.  No need to scan twice.
#if DEBUG_CELL_LISTENER_TRACKER
        __IXION_DEBUG_OUT__ << "listeners for this target has already been retrieved." << endl;
#endif
        return;
    }

    dirty_cells_t new_listeners;
    address_set_type new_listeners_addrs;
    range_query_set_type::search_result res = m_query_set.search(target.column, target.row);

#if DEBUG_CELL_LISTENER_TRACKER
    __IXION_DEBUG_OUT__ << "query set count: " << m_query_set.size() << "  search result count: " << res.size() << endl;
#endif

    std::for_each(
        res.begin(), res.end(), dirty_cell_inserter(m_context, new_listeners, new_listeners_addrs));
    assert(new_listeners.size() == new_listeners_addrs.size());

#if DEBUG_CELL_LISTENER_TRACKER
    __IXION_DEBUG_OUT__ << "new listener count: " << new_listeners.size() << endl;
#endif

    // Go through the new listeners and get their listeners as well.
    address_set_type::const_iterator itr = new_listeners_addrs.begin(), itr_end = new_listeners_addrs.end();
    for (; itr != itr_end; ++itr)
    {
        if (*itr == origin_target)
        {
#if DEBUG_CELL_LISTENER_TRACKER
         __IXION_DEBUG_OUT__ << "this equals origin target.  skipping." << endl;
#endif
            continue;
        }
        get_all_range_listeners_re(origin_target, *itr, listeners, listeners_addrs);
    }

    // Add new listeners to the caller's list.
    listeners.insert(new_listeners.begin(), new_listeners.end());
    listeners_addrs.insert(new_listeners_addrs.begin(), new_listeners_addrs.end());
#if DEBUG_CELL_LISTENER_TRACKER
    __IXION_DEBUG_OUT__ << "new listeners: ";
    std::for_each(new_listeners_addrs.begin(), new_listeners_addrs.end(),
                  cell_addr_printer(m_context.get_name_resolver()));
    cout << endl;
    __IXION_DEBUG_OUT__ << "--- end: target address: " << m_context.get_name_resolver().get_name(target, false) << endl;
#endif
}

}
