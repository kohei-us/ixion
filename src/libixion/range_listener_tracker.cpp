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

#include "ixion/range_listener_tracker.hpp"

#define DEBUG_RANGE_LISTENER_TRACKER 0

#if DEBUG_RANGE_LISTENER_TRACKER
#include "ixion/formula_name_resolver.hpp"
#include <iostream>
#endif

using namespace std;

namespace ixion {

range_listener_tracker::range_listener_tracker(model_context& cxt) :
    m_context(cxt) {}

range_listener_tracker::~range_listener_tracker()
{
    // Delete all the listener set instances.
    range_store_type::iterator itr = m_data.begin(), itr_end = m_data.end();
    for (; itr != itr_end; ++itr)
        delete itr->second;
}

void range_listener_tracker::add(const abs_address_t& cell, const abs_range_t& range)
{
#if DEBUG_RANGE_LISTENER_TRACKER
    const formula_name_resolver_base& res = m_context.get_name_resolver();
    __IXION_DEBUG_OUT__ << "cell: " << res.get_name(cell) << "  range: " << res.get_name(range) << endl;
#endif
    range_store_type::iterator itr = m_data.find(range);
    if (itr == m_data.end())
    {
        // No container for this range yet.  Create one.
        pair<range_store_type::iterator, bool> r = 
            m_data.insert(range_store_type::value_type(range, new address_set_type));
        if (!r.second)
            throw general_error("failed to insert new address set to range listener tracker.");
        itr = r.first;

        // Insert the container to the rectangle set as well (for lookup).
#if DEBUG_RANGE_LISTENER_TRACKER
        cout << "x1=" << range.first.column << ",y1=" << range.first.row
            << ",x2=" << range.last.column << ",y2=" << range.last.row << ",p=" << itr->second << endl;
#endif
        m_query_set.insert(
            range.first.column, range.first.row, range.last.column+1, range.last.row+1, itr->second);
    }
    itr->second->insert(cell);
}

void range_listener_tracker::remove(const abs_address_t& cell, const abs_range_t& range)
{
#if DEBUG_RANGE_LISTENER_TRACKER
    const formula_name_resolver_base& res = m_context.get_name_resolver();
    __IXION_DEBUG_OUT__ << "cell: " << res.get_name(cell) << "  range: " << res.get_name(range) << endl;
#endif
    range_store_type::iterator itr = m_data.find(range);
    if (itr == m_data.end())
        // No listeners for this range.  Bail out.
        return;

    address_set_type* p = itr->second;
    p->erase(cell);
    if (p->empty())
    {
        // This list is empty.  Remove it from the containers and destroy the instance.
        m_data.erase(itr);
        m_query_set.remove(p);
        delete p;
    }
}

namespace {

class dirty_cell_inserter : public std::unary_function<range_listener_tracker::address_set_type*, void>
{
    model_context& m_context;
    dirty_cells_t& m_dirty_cells;
    range_listener_tracker::address_set_type& m_addrs;
public:
    dirty_cell_inserter(model_context& cxt, dirty_cells_t& dirty_cells, range_listener_tracker::address_set_type& addrs) : 
        m_context(cxt), m_dirty_cells(dirty_cells), m_addrs(addrs) {}

    void operator() (const range_listener_tracker::address_set_type* p)
    {
        // Add all addresses in this set to the dirty cells list.
        range_listener_tracker::address_set_type::const_iterator itr = p->begin(), itr_end = p->end();
        for (; itr != itr_end; ++itr)
        {
            const abs_address_t& addr = *itr;
            base_cell* p = m_context.get_cell(addr);
            if (p && p->get_celltype() == celltype_formula)
            {
                // Formula cell exists at this address.
                m_dirty_cells.insert(p);
                m_addrs.insert(addr);
            }
        }
    }
};

}

void range_listener_tracker::get_all_listeners(
    const abs_address_t& target, dirty_cells_t& listeners) const
{
#if DEBUG_RANGE_LISTENER_TRACKER
    __IXION_DEBUG_OUT__ << "range_listener_tracker: get all listeners recursively" << endl;
#endif

    address_set_type listeners_addrs; // to keep track of circular references.
    get_all_listeners_re(target, listeners, listeners_addrs);
}

void range_listener_tracker::get_all_listeners_re(
    const abs_address_t& target, dirty_cells_t& listeners, address_set_type& listeners_addrs) const
{
    if (listeners_addrs.count(target))
    {
        // Target is included in the listener list. Possible circular reference.
#if DEBUG_RANGE_LISTENER_TRACKER
        __IXION_DEBUG_OUT__ << "Possible circular reference" << endl;
#endif
        return;
    }

    dirty_cells_t new_listeners;
    address_set_type new_listeners_addrs;
    range_query_set_type::search_result res = m_query_set.search(target.row, target.column);
    std::for_each(
        res.begin(), res.end(), dirty_cell_inserter(m_context, new_listeners, new_listeners_addrs));

#if DEBUG_RANGE_LISTENER_TRACKER
    __IXION_DEBUG_OUT__ << "new listener count: " << new_listeners.size() << endl;
#endif

    // Add new listeners to the caller's list.
    listeners.insert(new_listeners.begin(), new_listeners.end());
    listeners_addrs.insert(new_listeners_addrs.begin(), new_listeners_addrs.end());

    // Go through the new listeners and get their listeners as well.
    address_set_type::const_iterator itr = new_listeners_addrs.begin(), itr_end = new_listeners_addrs.end();
    for (; itr != itr_end; ++itr)
        get_all_listeners_re(*itr, listeners, listeners_addrs);
}

}
