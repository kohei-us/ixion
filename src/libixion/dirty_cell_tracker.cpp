/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/dirty_cell_tracker.hpp"
#include "ixion/global.hpp"
#include "ixion/formula_name_resolver.hpp"

#include "depth_first_search.hpp"

#include <mdds/rtree.hpp>
#include <deque>
#include <limits>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

namespace ixion {

namespace {

using rtree_type = mdds::rtree<rc_t, abs_range_set_t>;
using rtree_array_type = std::deque<rtree_type>;

} // anonymous namespace

struct dirty_cell_tracker::impl
{
    rtree_array_type m_grids;
    abs_range_set_t m_volatile_cells;

    mutable std::unique_ptr<formula_name_resolver> m_resolver;

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

    /**
     * Given a modified cell range, return all ranges that are directly
     * affected by it.
     *
     * @param range modified cell range.
     *
     * @return collection of ranges that are directly affected by the modified
     *         cell range.
     */
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

    std::string print(const abs_range_t& range) const
    {
        if (!m_resolver)
            m_resolver = formula_name_resolver::get(formula_name_resolver_t::excel_a1, nullptr);

        abs_address_t origin(0, 0, 0);
        range_t rrange = range;
        rrange.set_absolute(false);

        if (rrange.first == rrange.last)
            return m_resolver->get_name(rrange.first, origin, false);

        return m_resolver->get_name(rrange, origin, false);
    }
};

dirty_cell_tracker::dirty_cell_tracker() : mp_impl(ixion::make_unique<impl>()) {}
dirty_cell_tracker::dirty_cell_tracker(const iface::formula_model_access& fma) : mp_impl(ixion::make_unique<impl>()) {}
dirty_cell_tracker::~dirty_cell_tracker() {}

void dirty_cell_tracker::add(const abs_range_t& src, const abs_range_t& dest)
{
    if (!dest.valid())
    {
        std::ostringstream os;
        os << "dirty_cell_tracker::add: invalid destination range: src=" << src << "; dest=" << dest;
        throw std::invalid_argument(os.str());
    }

    if (dest.first.sheet != dest.last.sheet)
    {
        throw std::runtime_error("TODO: implement this.");
    }

    if (dest.all_columns() || dest.all_rows())
    {
        std::ostringstream os;
        os << "dirty_cell_tracker::add: unset column or row range is not allowed " << dest;
        throw std::invalid_argument(os.str());
    }

    rtree_type& tree = mp_impl->fetch_grid_or_resize(dest.first.sheet);

    rtree_type::extent_type search_box(
        {{dest.first.row, dest.first.column}, {dest.last.row, dest.last.column}});

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

void dirty_cell_tracker::remove(const abs_range_t& src, const abs_range_t& dest)
{
    if (!dest.valid())
    {
        std::ostringstream os;
        os << "dirty_cell_tracker::remove: invalid destination range: src=" << src << "; dest=" << dest;
        throw std::invalid_argument(os.str());
    }

    if (dest.first.sheet != dest.last.sheet)
    {
        throw std::runtime_error("TODO: implement this.");
    }

    if (dest.all_columns() || dest.all_rows())
    {
        std::ostringstream os;
        os << "dirty_cell_tracker::remove: unset column or row range is not allowed " << dest;
        throw std::invalid_argument(os.str());
    }

    rtree_type* tree = mp_impl->fetch_grid(dest.first.sheet);
    if (!tree)
    {
        SPDLOG_DEBUG(spdlog::get("ixion"), "Nothing is tracked on sheet {}.", dest.first.sheet);
        return;
    }

    rtree_type::extent_type search_box(
        {{dest.first.row, dest.first.column}, {dest.last.row, dest.last.column}});

    rtree_type::search_results res = tree->search(search_box, rtree_type::search_type::match);

    if (res.begin() == res.end())
    {
        // No listener for this destination cell. Nothing to remove.
        SPDLOG_DEBUG(spdlog::get("ixion"), "Cell {} is not being tracked by anybody.", dest);
        return;
    }

    rtree_type::iterator it_listener = res.begin();
    abs_range_set_t& listener = *it_listener;
    size_t n_removed = listener.erase(src);

    if (!n_removed)
    {
        SPDLOG_DEBUG(spdlog::get("ixion"), "Cell {} was not tracking cell {}.", src, dest);
    }

    if (listener.empty())
        // Remove this from the R-tree.
        tree->erase(it_listener);
}

void dirty_cell_tracker::add_volatile(const abs_range_t& pos)
{
    mp_impl->m_volatile_cells.insert(pos);
}

void dirty_cell_tracker::remove_volatile(const abs_range_t& pos)
{
    mp_impl->m_volatile_cells.erase(pos);
}

abs_range_set_t dirty_cell_tracker::query_dirty_cells(const abs_range_t& modified_cell) const
{
    abs_range_set_t mod_cells;
    mod_cells.insert(modified_cell);
    return query_dirty_cells(mod_cells);
}

abs_range_set_t dirty_cell_tracker::query_dirty_cells(const abs_range_set_t& modified_cells) const
{
    abs_range_set_t dirty_formula_cells;

    // Volatile cells are in theory always formula cells and therefore always
    // should be included.
    dirty_formula_cells.insert(
        mp_impl->m_volatile_cells.begin(), mp_impl->m_volatile_cells.end());

    abs_range_set_t cur_modified_cells = modified_cells;
    for (const abs_range_t& r : mp_impl->m_volatile_cells)
        cur_modified_cells.insert(r);

    while (!cur_modified_cells.empty())
    {
        abs_range_set_t next_modified_cells;

        for (const abs_range_t& mc : cur_modified_cells)
        {
            abs_range_set_t affected_ranges = mp_impl->get_affected_cell_ranges(mc);
            for (const abs_range_t& r : affected_ranges)
            {
                auto res = dirty_formula_cells.insert(r);
                if (res.second)
                    // This affected range has not yet been visited.  Put it
                    // in the chain for the next round of checks.
                    next_modified_cells.insert(r);
            }
        }

        cur_modified_cells.swap(next_modified_cells);
    }

    return dirty_formula_cells;
}

std::vector<abs_range_t> dirty_cell_tracker::query_and_sort_dirty_cells(const abs_range_t& modified_cell) const
{
    abs_range_set_t mod_cells;
    mod_cells.insert(modified_cell);
    return query_and_sort_dirty_cells(mod_cells);
}

std::vector<abs_range_t> dirty_cell_tracker::query_and_sort_dirty_cells(
    const abs_range_set_t& modified_cells, const abs_range_set_t* dirty_formula_cells) const
{
    abs_range_set_t cur_modified_cells = modified_cells;

    abs_range_set_t final_dirty_formula_cells;

    // Get the initial set of formula cells affected by the modified cells.
    // Note that these modified cells are not dirty formula cells.
    if (!cur_modified_cells.empty())
    {
        abs_range_set_t next_modified_cells;
        for (const abs_range_t& mc : cur_modified_cells)
        {
            for (const abs_range_t& r : mp_impl->get_affected_cell_ranges(mc))
            {
                auto res = final_dirty_formula_cells.insert(r);
                if (res.second)
                    // This affected range has not yet been visited.  Put it
                    // in the chain for the next round of checks.
                    next_modified_cells.insert(r);
            }
        }

        cur_modified_cells.swap(next_modified_cells);
    }

    // Because the modified cells in the subsequent rounds are all dirty
    // formula cells, we need to track precedent-dependent relationships for
    // later sorting.

    cur_modified_cells.insert(mp_impl->m_volatile_cells.begin(), mp_impl->m_volatile_cells.end());

    if (dirty_formula_cells)
        cur_modified_cells.insert(dirty_formula_cells->begin(), dirty_formula_cells->end());

    using dfs_type = depth_first_search<abs_range_t, abs_range_t::hash>;
    dfs_type::relations rels;

    while (!cur_modified_cells.empty())
    {
        abs_range_set_t next_modified_cells;
        for (const abs_range_t& mc : cur_modified_cells)
        {
            for (const abs_range_t& r : mp_impl->get_affected_cell_ranges(mc))
            {
                // Record each precedent-dependent relationship (r =
                // precedent; mc = dependent).
                rels.insert(r, mc);

                auto res = final_dirty_formula_cells.insert(r);
                if (res.second)
                    // This affected range has not yet been visited.  Put it
                    // in the chain for the next round of checks.
                    next_modified_cells.insert(r);
            }
        }

        cur_modified_cells.swap(next_modified_cells);
    }

    // Volatile cells are always formula cells and therefore always should be
    // included.
    final_dirty_formula_cells.insert(
        mp_impl->m_volatile_cells.begin(), mp_impl->m_volatile_cells.end());

    if (dirty_formula_cells)
    {
        final_dirty_formula_cells.insert(
            dirty_formula_cells->begin(), dirty_formula_cells->end());
    }

    // Perform topological sort on the dirty formula cell ranges.
    std::vector<abs_range_t> retval;
    dfs_type sorter(final_dirty_formula_cells.begin(), final_dirty_formula_cells.end(), rels, dfs_type::back_inserter(retval));
    sorter.run();

    return retval;
}

std::string dirty_cell_tracker::to_string() const
{
    auto resolver = formula_name_resolver::get(formula_name_resolver_t::excel_a1, nullptr);
    const abs_address_t origin(0, 0, 0);

    rc_t max_val = std::numeric_limits<rc_t>::max();
    std::vector<std::string> lines;

    for (rc_t i = 0, n = mp_impl->m_grids.size(); i < n; ++i)
    {
        const rtree_type& grid = mp_impl->m_grids[i];
        rtree_type::const_search_results res =
            grid.search({{0, 0}, {max_val, max_val}}, rtree_type::search_type::overlap);

        for (auto it = res.cbegin(); it != res.cend(); ++it)
        {
            const rtree_type::extent_type& ext = it.extent();
            const abs_range_set_t& srcs = *it;

            range_t dest(
                address_t(i, ext.start.d[0], ext.start.d[1]),
                address_t(i, ext.end.d[0], ext.end.d[1]));

            dest.set_absolute(false);

            std::string dest_name = ext.is_point() ?
                resolver->get_name(dest.first, origin, false) :
                resolver->get_name(dest, origin, false);

            for (const abs_range_t& src : srcs)
            {
                std::ostringstream os;
                os << mp_impl->print(src);
                os << " -> " << dest_name;
                lines.push_back(os.str());
            }
        }
    }

    if (lines.empty())
        return std::string();

    std::ostringstream os;
    auto it = lines.begin();
    os << *it;
    for (++it; it != lines.end(); ++it)
        os << std::endl << *it;
    return os.str();
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
