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
}

void dirty_cell_tracker::remove_volatile(const abs_address_t& pos)
{
}

cell_address_set_t dirty_cell_tracker::query_dirty_cells(const cell_address_set_t& modified_cells) const
{
    return cell_address_set_t();
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
