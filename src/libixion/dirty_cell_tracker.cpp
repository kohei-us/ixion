/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/dirty_cell_tracker.hpp"
#include "ixion/global.hpp"

namespace ixion {

struct dirty_cell_tracker::impl
{
    abs_address_set_t m_volatile_cells;
    impl() {}
};

dirty_cell_tracker::dirty_cell_tracker() : mp_impl(ixion::make_unique<impl>()) {}
dirty_cell_tracker::~dirty_cell_tracker() {}

void dirty_cell_tracker::add(const abs_address_t& src, const abs_address_t& dest)
{
}

void dirty_cell_tracker::add(const abs_address_t& cell, const abs_range_t& range)
{
}

void dirty_cell_tracker::remove(const abs_address_t& src, const abs_address_t& dest)
{
}

void dirty_cell_tracker::remove(const abs_address_t& cell, const abs_range_t& range)
{
}

void dirty_cell_tracker::add_volatile(const abs_address_t& pos)
{
    mp_impl->m_volatile_cells.insert(pos);
}

void dirty_cell_tracker::remove_volatile(const abs_address_t& pos)
{
    mp_impl->m_volatile_cells.erase(pos);
}

abs_address_set_t dirty_cell_tracker::query_dirty_cells(const abs_address_set_t& modified_cells) const
{
    abs_address_set_t dirty_formula_cells;

    // Volatile cells are in theory always formula cells and therefore always
    // should be included.
    dirty_formula_cells.insert(mp_impl->m_volatile_cells.begin(), mp_impl->m_volatile_cells.end());

    return dirty_formula_cells;
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
