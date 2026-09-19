/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once
#include "address.hpp"

#include <memory>

namespace ixion {

/**
 * This class is designed to track in-direct dependencies of dirty formula
 * cells.  A "dirty" formula cell is a formula cell whose result needs to be
 * re-calculated because at one or more of its references have updated values.
 *
 * This class also takes volatile functions into account when determining
 * the status of the formula cell result.  A volatile function is a cell
 * function whose value needs to be re-calculated on every re-calculation. One
 * example of a volatile function is NOW(), which returns the current time at
 * the time of calculation.
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

    /**
     * Get the positions of the formula cells that need re-calculating after
     * a modification: the registered volatile cells, and the cells that
     * directly or indirectly reference the modified cell.
     *
     * @param modified_cell Cell or range whose value has been modified.
     *
     * @return Positions of the dirty formula cells, in no particular order.
     *         A formula group appears as a single range.
     */
    abs_range_set_t query_dirty_cells(const abs_range_t& modified_cell) const;

    /**
     * Get the positions of the formula cells that need re-calculating after
     * modifications: the registered volatile cells, and the cells that
     * directly or indirectly reference any of the modified cells.
     *
     * @param modified_cells Cells or ranges whose values have been modified.
     *
     * @return Positions of the dirty formula cells, in no particular order.
     *         A formula group appears as a single range.
     */
    abs_range_set_t query_dirty_cells(const abs_range_set_t& modified_cells) const;

    /**
     * Get the positions of the formula cells that need re-calculating after
     * a modification, sorted in order of dependency so that each cell comes
     * after the cells it depends on.
     *
     * @param modified_cell Cell or range whose value has been modified.
     *
     * @return Positions of the dirty formula cells in calculation order.  A
     *         formula group appears as a single range.
     */
    std::vector<abs_range_t> query_and_sort_dirty_cells(const abs_range_t& modified_cell) const;

    /**
     * Get the positions of the formula cells that need re-calculating after
     * modifications, sorted in order of dependency so that each cell comes
     * after the cells it depends on.
     *
     * @param modified_cells Cells or ranges whose values have been modified.
     * @param dirty_formula_cells Formula cells to treat as dirty regardless
     *                            of the modifications, such as cells whose
     *                            formulas are new or have changed.  Their
     *                            dependents get included as well.
     *
     * @return Positions of the dirty formula cells in calculation order.  A
     *         formula group appears as a single range.
     */
    std::vector<abs_range_t> query_and_sort_dirty_cells(
        const abs_range_set_t& modified_cells, const abs_range_set_t* dirty_formula_cells = nullptr) const;

    /**
     * Get a string representation of all tracked relationships, for
     * debugging.
     */
    std::string to_string() const;

    /**
     * Check whether the tracker has any tracked relationship.  Registered
     * volatile cells are not taken into account.
     */
    bool empty() const;
};

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
