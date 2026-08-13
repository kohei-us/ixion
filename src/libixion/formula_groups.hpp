/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "column_store_type.hpp"

#include <vector>

namespace ixion { namespace detail {

/**
 * Entry that covers either a whole formula group or a single ungrouped
 * formula cell of a column.
 */
struct formula_group_entry
{
    formula_cell** cells; ///< Contiguous cells of the covered group.
    row_t row;            ///< Row position of the top-most cell.
    row_t size;           ///< Number of cells covered.
};

/**
 * Get the formula cells of a column as one entry per formula group.  A
 * formula group never spans multiple columns, and its cells are contiguous
 * within a single block.
 *
 * The entries point into the storage of the column; any change to the
 * column, including a call to detach(), invalidates them.
 *
 * The cells are exposed as mutable pointers regardless of how the column
 * store is accessed; only mutate them on a column store that does not
 * share its storage with another column store, i.e. call detach() on it
 * first.
 */
std::vector<formula_group_entry> get_formula_groups(const column_store_t& col);

}}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
