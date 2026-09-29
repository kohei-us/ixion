/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <ixion/address.hpp>

#include <vector>

namespace ixion {

class formula_cell;
class model_context;
struct formula_token;

namespace detail {

/**
 * Check that a formula cell can be registered with the dependency tracker,
 * and collect the references to register it with, including those reached
 * through named expressions.  Nothing gets modified, so the cell doesn't
 * have to be in the model yet.
 *
 * @return Reference tokens of the formula, to pass to
 *         apply_formula_registration().
 *
 * @throw model_context_error If the position is that of a grouped cell other
 *                            than the top-left cell of its group
 *                            (partial_formula_group), or a reference in the
 *                            formula points at an invalid sheet
 *                            (invalid_sheet_reference).
 */
std::vector<const formula_token*> validate_formula_registration(
    const model_context& cxt, const abs_address_t& pos, const formula_cell& cell);

/**
 * Register a formula cell with the dependency tracker.  The cell must be in
 * the model at the specified position, and the reference tokens must come
 * from validate_formula_registration() on it; nothing gets checked here.
 */
void apply_formula_registration(
    model_context& cxt, const abs_address_t& pos, const formula_cell& cell,
    const std::vector<const formula_token*>& ref_tokens);

/**
 * Unregister a formula cell from the dependency tracker.
 *
 * @throw model_context_error If the position is that of a grouped cell other
 *                            than the top-left cell of its group
 *                            (partial_formula_group), or a reference in the
 *                            formula points at an invalid sheet
 *                            (invalid_sheet_reference).
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
 * @throw model_context_error If a formula group lies only partly inside the
 *                            range (partial_formula_group), or a cell fails
 *                            validation (invalid_sheet_reference).
 */
void register_formula_cells(model_context& cxt, sheet_t sheet, const abs_rc_range_t& range);

/**
 * Unregister all formula cells in a range, each group once at its top-left
 * cell.
 *
 * @param sheet Index of the sheet the range is on.
 * @param range Range with all corners set.
 *
 * @throw model_context_error If a formula group lies only partly inside the
 *                            range (partial_formula_group).
 */
void unregister_formula_cells(model_context& cxt, sheet_t sheet, const abs_rc_range_t& range);

}}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
