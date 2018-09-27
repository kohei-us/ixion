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

namespace {

using rtree_type = mdds::rtree<rc_t, abs_range_set_t>;
using rtree_array_type = std::deque<rtree_type>;

} // anonymous namespace

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

    const rtree_type* fetch_grid(size_t n) const
    {
        return (n < m_grids.size()) ? &m_grids[n] : nullptr;
    }

    rtree_type* fetch_grid(size_t n)
    {
        return (n < m_grids.size()) ? &m_grids[n] : nullptr;
    }

    abs_range_set_t get_affected_cell_ranges(const abs_range_t& range) const
    {
        const rtree_type* grid = fetch_grid(range.first.sheet);
        if (!grid)
            return abs_range_set_t();

        rtree_type::const_search_results res = grid->search(
            {{range.first.row, range.first.column}, {range.last.row, range.last.column}},
            rtree_type::search_type::overlap);

        abs_range_set_t ranges;

        for (const abs_range_set_t& range_set : res)
            ranges.insert(range_set.begin(), range_set.end());

        return ranges;
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

    if (!dest.valid())
    {
        std::ostringstream os;
        os << "dirty_cell_tracker::add: invalid destination address " << dest;
        throw std::invalid_argument(os.str());
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

void dirty_cell_tracker::add(const abs_address_t& src, const abs_range_t& range)
{
    if (range.first.sheet < 0)
    {
        BOOST_LOG_TRIVIAL(warning) << "Invalid sheet position (" << range.first.sheet << ")";
        return;
    }

    if (!range.valid())
    {
        std::ostringstream os;
        os << "dirty_cell_tracker::add: invalid destination range " << range;
        throw std::invalid_argument(os.str());
    }

    rtree_type& tree = mp_impl->fetch_grid_or_resize(range.first.sheet);

    rtree_type::extent_type search_box(
        {{range.first.row, range.first.column}, {range.last.row, range.last.column}});

    rtree_type::search_results res = tree.search(search_box, rtree_type::search_type::match);

    if (res.begin() == res.end())
    {
        // No listener for this destination range.  Insert a new one.
        abs_range_set_t listener;
        listener.emplace(src);
        tree.insert(search_box, std::move(listener));
    }
    else
    {
        // A listener already exists for this destination cell.
        abs_range_set_t& listener = *res.begin();
        listener.emplace(src);
    }
}

void dirty_cell_tracker::remove(const abs_address_t& src, const abs_address_t& dest)
{
    if (dest.sheet < 0)
    {
        BOOST_LOG_TRIVIAL(warning) << "Invalid sheet position (" << dest.sheet << ")";
        return;
    }

    if (!dest.valid())
    {
        std::ostringstream os;
        os << "dirty_cell_tracker::remove: invalid destination address " << dest;
        throw std::invalid_argument(os.str());
    }

    rtree_type* tree = mp_impl->fetch_grid(dest.sheet);
    if (!tree)
    {
        BOOST_LOG_TRIVIAL(warning) << "dirty_cell_tracker::remove: nothing is tracked on sheet " << dest.sheet << ".";
        return;
    }

    rtree_type::search_results res = tree->search(
        {dest.row, dest.column}, rtree_type::search_type::match);

    if (res.begin() == res.end())
    {
        // No listener for this destination cell. Nothing to remove.
        BOOST_LOG_TRIVIAL(warning) << "dirty_cell_tracker::remove: cell " << dest << " is not being tracked by anybody.";
        return;
    }

    rtree_type::iterator it_listener = res.begin();
    abs_range_set_t& listener = *it_listener;
    size_t n_removed = listener.erase(src);

    if (!n_removed)
        BOOST_LOG_TRIVIAL(warning) << "dirty_cell_tracker::remove: cell " << src << " was not tracking cell " << dest << ".";

    if (listener.empty())
        // Remove this from the R-tree.
        tree->erase(it_listener);
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
    dirty_formula_cells.insert(
        mp_impl->m_volatile_cells.begin(), mp_impl->m_volatile_cells.end());

    abs_range_set_t cur_modified_cells;
    for (const abs_address_t& mc : modified_cells)
        cur_modified_cells.emplace(mc);

    while (!cur_modified_cells.empty())
    {
        abs_range_set_t next_modified_cells;
        for (const abs_range_t& mc : cur_modified_cells)
        {
            next_modified_cells = mp_impl->get_affected_cell_ranges(mc);

            for (const abs_range_t& r : next_modified_cells)
                dirty_formula_cells.insert(r.first);
        }

        cur_modified_cells.swap(next_modified_cells);
    }

    return dirty_formula_cells;
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
