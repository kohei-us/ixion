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

namespace iface { class formula_model_access; }

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

    void get_all_dirty_cells(
        const iface::formula_model_access& cxt, modified_cells_t& addrs, dirty_formula_cells_t& cells) const;
};

}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
