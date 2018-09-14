/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/cell_listener_tracker.hpp"
#include "ixion/formula_name_resolver.hpp"
#include "ixion/cell.hpp"
#include "ixion/model_context.hpp"

#include <mdds/rectangle_set.hpp>

#define DEBUG_CELL_LISTENER_TRACKER 0

#include <cassert>
#include <unordered_map>

#if DEBUG_CELL_LISTENER_TRACKER
#include <iostream>
#endif

#include "grouped_ranges.hpp"

using namespace std;

namespace ixion {

namespace {

typedef mdds::rectangle_set<row_t, cell_listener_tracker::address_set_type*> range_query_set_type;
typedef std::unordered_map<abs_address_t, cell_listener_tracker::address_set_type*, abs_address_t::hash> cell_store_type;
typedef std::unordered_map<abs_range_t, cell_listener_tracker::address_set_type*, abs_range_t::hash> range_store_type;

class dirty_cell_inserter : public std::unary_function<cell_listener_tracker::address_set_type*, void>
{
    model_context& m_context;
    dirty_formula_cells_t& m_dirty_cells;
    cell_listener_tracker::address_set_type& m_addrs;
public:
    dirty_cell_inserter(model_context& cxt, dirty_formula_cells_t& dirty_cells, cell_listener_tracker::address_set_type& addrs) :
        m_context(cxt), m_dirty_cells(dirty_cells), m_addrs(addrs) {}

    void operator() (const cell_listener_tracker::address_set_type* p)
    {
        // Add all addresses in this set to the dirty cells list.
        cell_listener_tracker::address_set_type::const_iterator itr = p->begin(), itr_end = p->end();
        for (; itr != itr_end; ++itr)
        {
            const abs_address_t& addr = *itr;
            if (m_context.get_celltype(addr) != celltype_t::formula)
                continue;

            // Formula cell exists at this address.
            m_dirty_cells.insert(addr);
            m_addrs.insert(addr);
        }
    }
};

}

struct cell_listener_tracker::impl
{
    model_context& m_context;

    mutable range_query_set_type m_query_set; ///< used for fast lookup of range listeners.
    cell_store_type m_cell_listeners;         ///< store listeners for single cells.
    range_store_type m_range_listeners;       ///< store listeners for ranges.
    cell_listener_tracker::address_set_type m_volatile_cells;

    impl(model_context& cxt) :
        m_context(cxt) {}

    ~impl()
    {
        // Delete all the listener set instances.
        for_each(m_range_listeners.begin(), m_range_listeners.end(), delete_map_value<range_store_type>());
        for_each(m_cell_listeners.begin(), m_cell_listeners.end(), delete_map_value<cell_store_type>());
    }

    void get_all_range_listeners_re(
        const abs_address_t& origin_target, const abs_address_t& target,
        dirty_formula_cells_t& listeners, address_set_type& listeners_addr) const;
};

void cell_listener_tracker::impl::get_all_range_listeners_re(
    const abs_address_t& origin_target, const abs_address_t& target, dirty_formula_cells_t& listeners, address_set_type& listeners_addrs) const
{
    if (listeners_addrs.count(target))
    {
        // Target is included in the listener list.  No need to scan twice.
#if DEBUG_CELL_LISTENER_TRACKER
        __IXION_DEBUG_OUT__ << "listeners for this target has already been retrieved." << endl;
#endif
        return;
    }

    dirty_formula_cells_t new_listeners;
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
}

cell_listener_tracker::cell_listener_tracker(model_context& cxt) :
    mp_impl(make_unique<impl>(cxt)) {}

cell_listener_tracker::~cell_listener_tracker() {}

void cell_listener_tracker::add(const abs_address_t& src, const abs_address_t& dest)
{
#if DEBUG_CELL_LISTENER_TRACKER
    {
        abs_address_t origin(0,0,0);
        auto res = formula_name_resolver::get(formula_name_resolver_t::excel_a1, &mp_impl->m_context);
        __IXION_DEBUG_OUT__ << "adding - cell src: " << res->get_name(src, origin, false)
            << "  cell dest: " << res->get_name(dest, origin, false) << endl;
    }
#endif
    cell_store_type::iterator itr = mp_impl->m_cell_listeners.find(dest);
    if (itr == mp_impl->m_cell_listeners.end())
    {
        // No container for this src cell yet.  Create one.
        pair<cell_store_type::iterator, bool> r =
            mp_impl->m_cell_listeners.insert(cell_store_type::value_type(dest, new address_set_type));
        if (!r.second)
            throw general_error("failed to insert new address set to cell listener tracker.");
        itr = r.first;
    }
    itr->second->insert(src);
}

void cell_listener_tracker::add(const abs_address_t& cell, const abs_range_t& range)
{
#if DEBUG_CELL_LISTENER_TRACKER
    {
        abs_address_t origin(0,0,0);
        auto res = formula_name_resolver::get(formula_name_resolver_t::excel_a1, &mp_impl->m_context);
        __IXION_DEBUG_OUT__ << "adding - cell src: " << res->get_name(cell, origin, false)
            << "  range dest: " << res->get_name(range, origin, false) << endl;
    }
#endif
    range_store_type::iterator itr = mp_impl->m_range_listeners.find(range);
    if (itr == mp_impl->m_range_listeners.end())
    {
        // No container for this range yet.  Create one.
        pair<range_store_type::iterator, bool> r =
            mp_impl->m_range_listeners.insert(range_store_type::value_type(range, new address_set_type));
        if (!r.second)
            throw general_error("failed to insert new address set to range listener tracker.");
        itr = r.first;

        // Insert the container to the rectangle set as well (for lookup).
#if DEBUG_CELL_LISTENER_TRACKER
        cout << "x1=" << range.first.column << ",y1=" << range.first.row
            << ",x2=" << (range.last.column+1) << ",y2=" << (range.last.row+1) << ",p=" << itr->second << endl;
#endif
        mp_impl->m_query_set.insert(
            range.first.column, range.first.row, range.last.column+1, range.last.row+1, itr->second);
    }
    itr->second->insert(cell);
}

void cell_listener_tracker::remove(const abs_address_t& src, const abs_address_t& dest)
{
    cell_store_type::iterator itr = mp_impl->m_cell_listeners.find(dest);
    if (itr == mp_impl->m_cell_listeners.end())
        // No listeners for this cell.  Bail out.
        return;

    address_set_type* p = itr->second;
    p->erase(src);
    if (p->empty())
    {
        // This list is empty.  Remove it from the containers and destroy the instance.
        mp_impl->m_cell_listeners.erase(itr);
        delete p;
    }
}

void cell_listener_tracker::remove(const abs_address_t& cell, const abs_range_t& range)
{
    range_store_type::iterator itr = mp_impl->m_range_listeners.find(range);
    if (itr == mp_impl->m_range_listeners.end())
        // No listeners for this range.  Bail out.
        return;

    address_set_type* p = itr->second;
    p->erase(cell);
    if (p->empty())
    {
        // This list is empty.  Remove it from the containers and destroy the instance.
        mp_impl->m_range_listeners.erase(itr);
        mp_impl->m_query_set.remove(p);
        delete p;
    }
}

void cell_listener_tracker::add_volatile(const abs_address_t& pos)
{
    mp_impl->m_volatile_cells.insert(pos);
}

void cell_listener_tracker::remove_volatile(const abs_address_t& pos)
{
    mp_impl->m_volatile_cells.erase(pos);
}

const cell_listener_tracker::address_set_type& cell_listener_tracker::get_volatile_cells() const
{
    return mp_impl->m_volatile_cells;
}

namespace {

class cell_addr_printer : public std::unary_function<abs_address_t, void>
{
    const formula_name_resolver& m_resolver;
public:
    cell_addr_printer(const formula_name_resolver& resolver) : m_resolver(resolver) {}
    void operator() (const abs_address_t& addr) const
    {
        address_t pos_display(addr);
        pos_display.set_absolute(false);
        cout << m_resolver.get_name(pos_display, abs_address_t(), false) << " ";
    }
};

}

void cell_listener_tracker::get_all_cell_listeners(
    const abs_address_t& target, dirty_formula_cells_t& listeners) const
{
    cell_store_type::const_iterator itr = mp_impl->m_cell_listeners.find(target);
    if (itr == mp_impl->m_cell_listeners.end())
        // This target cell has no listeners.
        return;

    const address_set_type& addrs = *itr->second;
    address_set_type::const_iterator itr2 = addrs.begin(), itr2_end = addrs.end();
    for (; itr2 != itr2_end; ++itr2)
    {
        const abs_address_t& addr = *itr2; // listener cell address
        if (mp_impl->m_context.get_celltype(addr) != celltype_t::formula)
            // Referenced cell is empty or not a formula cell.  Ignore this.
            continue;

        if (listeners.count(addr) == 0)
        {
            // This cell is not yet on the dirty cell list.  Run recursively.
            listeners.insert(addr);
            get_all_cell_listeners(addr, listeners);
            get_all_range_listeners(addr, listeners);
        }
    }
}

void cell_listener_tracker::get_all_range_listeners(
    const abs_address_t& target, dirty_formula_cells_t& listeners) const
{
    address_set_type listeners_addrs; // to keep track of circular references.
    mp_impl->get_all_range_listeners_re(target, target, listeners, listeners_addrs);
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
