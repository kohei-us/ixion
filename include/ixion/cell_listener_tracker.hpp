/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_RANGE_LISTENER_TRACKER_HPP
#define INCLUDED_IXION_RANGE_LISTENER_TRACKER_HPP

#include "ixion/global.hpp"
#include "ixion/address.hpp"

#include <unordered_set>
#include <memory>

namespace ixion {

class formula_name_resolver;
class model_context;

/**
 * Track all single and range references being listened to by individual
 * cells.
 */
class IXION_DLLPUBLIC cell_listener_tracker
{
    struct impl;
    std::unique_ptr<impl> mp_impl;

public:

    cell_listener_tracker() = delete;
    cell_listener_tracker(const cell_listener_tracker&) = delete;
    cell_listener_tracker& operator=(const cell_listener_tracker&) = delete;

    typedef std::unordered_set<abs_address_t, abs_address_t::hash> address_set_type;

    cell_listener_tracker(model_context& cxt);
    ~cell_listener_tracker();

    /**
     * Add a reference relationship between two single cells.
     *
     * @param src cell that references (therefore listens to) dest cell.
     * @param dest cell being referenced (therefore being listened to) by dest
     *             cell.
     */
    void add(const abs_address_t& src, const abs_address_t& dest);

    /**
     * Add a reference relationship from a single cell to a range.  The cell
     * references the range.  Duplicates are silently ignored.
     *
     * @param cell cell that includes reference to the range.
     * @param range range referenced by the cell.
     */
    void add(const abs_address_t& cell, const abs_range_t& range);

    void remove(const abs_address_t& src, const abs_address_t& dest);

    /**
     * Remove an existing reference relationship from a single cell to a
     * range.  If no such relationship exists, it does nothing.
     *
     * @param cell cell that includes reference to the range.
     * @param range range referenced by the cell.
     */
    void remove(const abs_address_t& cell, const abs_range_t& range);

    void add_volatile(const abs_address_t& pos);
    void remove_volatile(const abs_address_t& pos);
    const address_set_type& get_volatile_cells() const;

    void get_all_cell_listeners(const abs_address_t& target, dirty_formula_cells_t& listeners) const;

    /**
     * Given a modified cell (target), get all formula cells that need to be
     * re-calculated based on the range-to-cells listener relationships.
     *
     * @param target
     * @param listeners
     */
    void get_all_range_listeners(const abs_address_t& target, dirty_formula_cells_t& listeners) const;

    /**
     * Register a new grouped range.  A grouped range is a range whose
     * top-left cell is always used to track dependency.  One example of a
     * grouped range is a group of matrix formula cells.
     *
     * Note that grouped ranges cannot overlap with each other.
     *
     * @param sheet index of the sheet on which the grouped range exists.
     * @param range grouped range to add to the collection.
     * @param identity identity of the grouped range.
     */
    void add_grouped_range(sheet_t sheet, const abs_rc_range_t& range, uintptr_t identity);

    /**
     * Remove an existing grouped range from being tracked.
     *
     * @param sheet index of the sheet on which the grouped range to be
     *              removed currently exists.
     * @param identity identity of the range to be removed.
     */
    void remove_grouped_range(sheet_t sheet, uintptr_t identity);

    /**
     * Move the cell position to the top-left corner of a grouped range if the
     * specified position falls within a grouped range.  Nothing happens if
     * the specified position is not within any grouped range.
     *
     * @param pos cell position to move.
     */
    abs_rc_address_t move_to_grouped_range_origin(sheet_t sheet, const abs_rc_address_t& pos) const;
};

}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
