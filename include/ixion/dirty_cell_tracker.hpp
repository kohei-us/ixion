/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_DIRTY_CELL_TRACKER_HPP
#define INCLUDED_IXION_DIRTY_CELL_TRACKER_HPP

#include "address.hpp"

#include <memory>

namespace ixion {

namespace iface { class formula_model_access; }

/**
 * This class is designed to track in-direct dependencies of dirty formula
 * cells.  A "dirty" formula cell is a formula cell whose result needs to be
 * re-calculated because at least one of its references have their values
 * updated.
 *
 * This class also takes volatile functions into account when determining
 * the status of the formula cel result.  A volatile function is a cell
 * function whose value needs to get re-calculated unconditionally on every
 * re-calculation.  One example of a volatile function is NOW(), which
 * returns the current time at the time of calculation.
 */
class IXION_DLLPUBLIC dirty_cell_tracker
{
    struct impl;
    std::unique_ptr<impl> mp_impl;

public:
    dirty_cell_tracker(const dirty_cell_tracker&) = delete;
    dirty_cell_tracker& operator= (const dirty_cell_tracker&) = delete;

    dirty_cell_tracker();
    ~dirty_cell_tracker();

    /**
     * Add a tracking relationship from a source cell or cell range to a
     * destination cell or cell range.
     *
     * @param src source cell or cell range that includes reference to
     *             (therefore listens to) the range.
     * @param dest destination cell or range referenced tracked by the source
     *             cell.
     */
    void add(const abs_range_t& src, const abs_range_t& dest);

    /**
     * Remove an existing tracking relationship from a source cell or cell
     * range to a destination cell or cell range. If no such relationship
     * exists, it does nothing.
     *
     * @param src cell or cell range that includes reference to the range.
     * @param dest cell or range referenced by the cell.
     */
    void remove(const abs_range_t& src, const abs_range_t& dest);

    /**
     * Register a formula cell located at the specified position as volatile.
     * Note that the caller should ensure that the cell at the specified
     * position is indeed a formula cell.
     *
     * @param pos position of the cell to register as a volatile cell.
     */
    void add_volatile(const abs_range_t& pos);

    /**
     * Remove the specified cell position from the internal set of registered
     * volatile formula cells.
     *
     * @param pos position of the cell to unregister as a volatile cell.
     */
    void remove_volatile(const abs_range_t& pos);

    abs_range_set_t query_dirty_cells(const abs_range_t& modified_cell) const;

    abs_range_set_t query_dirty_cells(const abs_range_set_t& modified_cells) const;

    std::vector<abs_range_t> query_and_sort_dirty_cells(const abs_range_t& modified_cell) const;

    std::vector<abs_range_t> query_and_sort_dirty_cells(
        const abs_range_set_t& modified_cells, const abs_range_set_t* dirty_formula_cells = nullptr) const;

    std::string to_string() const;

    bool empty() const;
};

}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
