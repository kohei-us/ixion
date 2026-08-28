/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <ixion/types.hpp>

#include <vector>

namespace ixion {

class model_context;
struct abs_rc_range_t;

namespace detail {

class sheet_store;

/**
 * Sort the rows of a range in place.  The rows of the range move as units
 * across all of its columns; cells outside the range never move.
 *
 * The sort is stable, and orders cells of different types as numeric
 * values first, then strings, then false, then true, then error values,
 * with empty cells always last regardless of the direction.
 *
 * Formula cells sort by their cached results through the same order; a
 * formula cell without a cached result sorts like an empty cell.  String
 * comparison is byte-wise.
 *
 * @param cxt Model context whose string pool resolves the string values of
 *            the sorted cells.
 * @param store Sheet store to sort.
 * @param range Range to sort.
 * @param keys Sort keys in order of precedence.  Every key column must lie
 *             within the columns of the range.
 *
 * @return Permutation of the sorted rows: element i holds the original row
 *         of the cells that now occupy row range.first.row + i.
 *
 * @throw std::invalid_argument When the range does not fit within the
 *        sheet store, no keys are given, or a key column lies outside the
 *        range.
 */
std::vector<row_t> sort_range(
    const model_context& cxt, sheet_store& store,
    const abs_rc_range_t& range, const sort_keys_t& keys);

}}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
