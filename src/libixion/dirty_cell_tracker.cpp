/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/dirty_cell_tracker.hpp"
#include "ixion/global.hpp"

#include <mdds/rtree.hpp>
#include <deque>

#include <boost/log/trivial.hpp>

namespace ixion {

using rtree_type = mdds::rtree<rc_t, abs_range_set_t>;
using rtree_array_type = std::deque<rtree_type>;

struct dirty_cell_tracker::impl
{
    rtree_array_type m_grids;
    abs_address_set_t m_volatile_cells;

    impl() {}

    rtree_type& fetch_grid_or_resize(size_t n)
    {
        if (m_grids.size() <= n)
            m_grids.resize(n+1);

        return m_grids[n];
    }

    const rtree_type& fetch_grid(size_t n) const
    {
        return m_grids.at(n);
    }

    abs_address_set_t get_affected_cells(const abs_address_t& cell) const
    {
        const rtree_type& grid = fetch_grid(cell.sheet);
        rtree_type::const_search_results res = grid.search(
            {cell.row, cell.column}, rtree_type::search_type::overlap);

        abs_address_set_t cells;

        for (const abs_range_set_t& range_set : res)
        {
            for (const abs_range_t& range : range_set)
                cells.insert(range.first);
        }

        return cells;
    }
};

dirty_cell_tracker::dirty_cell_tracker() : mp_impl(ixion::make_unique<impl>()) {}
dirty_cell_tracker::~dirty_cell_tracker() {}

void dirty_cell_tracker::add(const abs_address_t& src, const abs_address_t& dest)
{
    if (dest.sheet < 0)
    {
        BOOST_LOG_TRIVIAL(warning) << "Invalid sheet position (" << dest.sheet << ")";
        return;
    }

    rtree_type& tree = mp_impl->fetch_grid_or_resize(dest.sheet);
    rtree_type::search_results res = tree.search(
        {dest.row, dest.column}, rtree_type::search_type::match);

    if (res.begin() == res.end())
    {
        // No listener for this destination cell.  Insert a new one.
        abs_range_set_t listener;
        listener.emplace(src);
        tree.insert({dest.row, dest.column}, std::move(listener));
    }
    else
    {
        // A listener already exists for this destination cell.
        abs_range_set_t& listener = *res.begin();
        listener.emplace(src);
    }
}

void dirty_cell_tracker::add(const abs_address_t& cell, const abs_range_t& range)
{
    assert(!"TESTME");
}

void dirty_cell_tracker::remove(const abs_address_t& src, const abs_address_t& dest)
{
    assert(!"TESTME");
}

void dirty_cell_tracker::remove(const abs_address_t& cell, const abs_range_t& range)
{
    assert(!"TESTME");
}

void dirty_cell_tracker::add_volatile(const abs_address_t& pos)
{
    assert(!"TESTME");
    mp_impl->m_volatile_cells.insert(pos);
}

void dirty_cell_tracker::remove_volatile(const abs_address_t& pos)
{
    assert(!"TESTME");
    mp_impl->m_volatile_cells.erase(pos);
}

abs_address_set_t dirty_cell_tracker::query_dirty_cells(const abs_address_set_t& modified_cells) const
{
    abs_address_set_t dirty_formula_cells;

    // Volatile cells are in theory always formula cells and therefore always
    // should be included.
    dirty_formula_cells.insert(mp_impl->m_volatile_cells.begin(), mp_impl->m_volatile_cells.end());

    for (const abs_address_t& mc : modified_cells)
    {
        auto cells = mp_impl->get_affected_cells(mc);
        dirty_formula_cells.insert(cells.begin(), cells.end());
    }

    return dirty_formula_cells;
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
