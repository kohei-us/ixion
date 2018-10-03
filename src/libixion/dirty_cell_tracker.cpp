/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/dirty_cell_tracker.hpp"
#include "ixion/global.hpp"
#include "ixion/formula_name_resolver.hpp"

#include <mdds/rtree.hpp>
#include <deque>
#include <limits>

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
};

dirty_cell_tracker::dirty_cell_tracker() : mp_impl(ixion::make_unique<impl>()) {}
dirty_cell_tracker::dirty_cell_tracker(const iface::formula_model_access& fma) : mp_impl(ixion::make_unique<impl>()) {}
dirty_cell_tracker::~dirty_cell_tracker() {}

void dirty_cell_tracker::add(const abs_range_t& src, const abs_range_t& dest)
{
    if (dest.first.sheet < 0)
    {
        BOOST_LOG_TRIVIAL(warning) << "Invalid sheet position (" << dest.first.sheet << ")";
        return;
    }

    if (!dest.valid())
    {
        std::ostringstream os;
        os << "dirty_cell_tracker::add: invalid destination cell or range " << dest;
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
    if (dest.first.sheet < 0)
    {
        BOOST_LOG_TRIVIAL(warning) << "Invalid sheet position (" << dest.first.sheet << ")";
        return;
    }

    if (!dest.valid())
    {
        std::ostringstream os;
        os << "dirty_cell_tracker::add: invalid destination range " << dest;
        throw std::invalid_argument(os.str());
    }

    rtree_type* tree = mp_impl->fetch_grid(dest.first.sheet);
    if (!tree)
    {
        BOOST_LOG_TRIVIAL(warning) << "dirty_cell_tracker::remove: nothing is tracked on sheet " << dest.first.sheet << ".";
        return;
    }

    rtree_type::extent_type search_box(
        {{dest.first.row, dest.first.column}, {dest.last.row, dest.last.column}});

    rtree_type::search_results res = tree->search(search_box, rtree_type::search_type::match);

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

void dirty_cell_tracker::add_volatile(const abs_address_t& pos)
{
    mp_impl->m_volatile_cells.insert(pos);
}

void dirty_cell_tracker::remove_volatile(const abs_address_t& pos)
{
    mp_impl->m_volatile_cells.erase(pos);
}

abs_address_set_t dirty_cell_tracker::query_dirty_cells(const abs_address_t& modified_cell) const
{
    abs_address_set_t mod_cells;
    mod_cells.insert(modified_cell);
    return query_dirty_cells(mod_cells);
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
            abs_range_set_t affected_ranges = mp_impl->get_affected_cell_ranges(mc);
            for (const abs_range_t& r : affected_ranges)
            {
                auto res = dirty_formula_cells.insert(r.first);
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

            for (range_t src : srcs) // conversion from abs_range_t to range_t.
            {
                src.set_absolute(false);
                std::ostringstream os;
                if (src.first == src.last)
                    os << resolver->get_name(src.first, origin, false);
                else
                    os << resolver->get_name(src, origin, false);

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
