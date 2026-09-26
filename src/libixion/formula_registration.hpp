/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <ixion/address.hpp>

namespace ixion {

class formula_cell;
class model_context;

namespace detail {

/**
 * Check that a formula cell can be registered with the dependency tracker.
 * Nothing gets modified, so the cell doesn't have to be in the model yet.
 *
 * @throw formula_registration_error If the position is that of a grouped
 *                                   cell other than the top-left cell of its
 *                                   group, or a reference in the formula
 *                                   points at an invalid sheet.
 */
void validate_formula_registration(
    const model_context& cxt, const abs_address_t& pos, const formula_cell& cell);

/**
 * Register a formula cell with the dependency tracker.  The cell must be in
 * the model at the specified position, and must have passed
 * validate_formula_registration(); nothing gets checked here.
 */
void apply_formula_registration(
    model_context& cxt, const abs_address_t& pos, const formula_cell& cell);

/**
 * Unregister a formula cell from the dependency tracker.
 *
 * @throw formula_registration_error If the position is that of a grouped
 *                                   cell other than the top-left cell of its
 *                                   group, or a reference in the formula
 *                                   points at an invalid sheet.
 */
void remove_formula_registration(
    model_context& cxt, const abs_address_t& pos, const formula_cell& cell);

/**
 * Register all formula cells in a range, each group once at its top-left
 * cell.  Every cell gets validated before any of them gets registered, so a
 * rejected cell leaves the tracker unchanged.
 *
 * @param sheet Index of the sheet the range is on.
 * @param range Range with all corners set.
 *
 * @throw formula_registration_error If a formula group lies only partly
 *                                   inside the range, or a cell fails
 *                                   validation.
 */
void register_formula_cells(model_context& cxt, sheet_t sheet, const abs_rc_range_t& range);

/**
 * Unregister all formula cells in a range, each group once at its top-left
 * cell.
 *
 * @param sheet Index of the sheet the range is on.
 * @param range Range with all corners set.
 *
 * @throw formula_registration_error If a formula group lies only partly
 *                                   inside the range.
 */
void unregister_formula_cells(model_context& cxt, sheet_t sheet, const abs_rc_range_t& range);

}}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
