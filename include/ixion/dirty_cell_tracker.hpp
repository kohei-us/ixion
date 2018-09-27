/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_DIRTY_CELL_TRACKER_HPP
#define INCLUDED_IXION_DIRTY_CELL_TRACKER_HPP

#include "ixion/address.hpp"

#include <memory>

namespace ixion {

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
     * Add a tracking relationship from a source cell to a destination cell or
     * a range of cells.
     *
     * @param src source cell that includes reference to (therefore listens
     *             to) the range.
     * @param dest destination cell or range referenced tracked by the source
     *             cell.
     */
    void add(const abs_address_t& src, const abs_range_t& dest);

    /**
     * Remove an existing tracking relationship from a source cell to a
     * destination. If no such relationship exists, it does nothing.
     *
     * @param src cell that includes reference to the range.
     * @param dest cell or range referenced by the cell.
     */
    void remove(const abs_address_t& src, const abs_range_t& dest);

    /**
     * Register a formula cell located at the specified position as volatile.
     * Note that the caller should ensure that the cell at the specified
     * position is indeed a formula cell.
     *
     * @param pos position of the cell to register as a volatile cell.
     */
    void add_volatile(const abs_address_t& pos);

    /**
     * Remove the specified cell position from the internal set of registered
     * volatile formula cells.
     *
     * @param pos position of the cell to unregister as a volatile cell.
     */
    void remove_volatile(const abs_address_t& pos);

    abs_address_set_t query_dirty_cells(const abs_address_set_t& modified_cells) const;
};

}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
