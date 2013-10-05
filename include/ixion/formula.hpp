/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __IXION_FORMULA_HPP__
#define __IXION_FORMULA_HPP__

#include "ixion/formula_tokens.hpp"
#include "ixion/interface/model_context.hpp"
#include "ixion/env.hpp"

#include <string>

namespace ixion {

/**
 * Parse a raw formula expression string into formula tokens.
 *
 * @param cxt model context.
 * @param pos address of the cell that has the formula expression.
 * @param p pointer to the first character of raw formula expression string.
 * @param n size of the raw formula expression string.
 * @param tokens formula tokens representing the parsed formula expression.
 */
void IXION_DLLPUBLIC parse_formula_string(
    iface::model_context& cxt, const abs_address_t& pos,
    const char* p, size_t n, formula_tokens_t& tokens);

/**
 * Convert formula tokens into a human-readable string representation.
 *
 * @param cxt model context.
 * @param pos address of the cell that has the formula tokens.
 * @param tokens formula tokens.
 * @param str string representation of the formula tokens.
 */
void IXION_DLLPUBLIC print_formula_tokens(
    const iface::model_context& cxt, const abs_address_t& pos,
    const formula_tokens_t& tokens, std::string& str);

/**
 * Regisiter a formula cell with cell dependency tracker.
 *
 * @param cxt model context.
 * @param pos address of the cell being registered.
 */
void IXION_DLLPUBLIC register_formula_cell(
    iface::model_context& cxt, const abs_address_t& pos);

/**
 * Unregister a formula cell with cell dependency tracker if a formula cell
 * exists at specified cell address.  If there is no existing cell at the
 * specified address, or the cell is not a formula cell, this function is a
 * no-op.
 *
 * @param cxt model context.
 * @param pos address of the cell being unregistered.
 */
void IXION_DLLPUBLIC unregister_formula_cell(
    iface::model_context& cxt, const abs_address_t& pos);

/**
 * Get all cells that directly or indirectly depend on known modified cells.
 * We call such cells "dirty cells".
 *
 * @param cxt model context
 * @param addrs list of addresses of cells that have been modified.  Note
 *              that this call may add additional cells to this list in a
 *              presence of volatile cells.
 * @param cells all dirty cells are inserted into this container when this
 *              function returns.
 */
void IXION_DLLPUBLIC get_all_dirty_cells(
    iface::model_context& cxt, modified_cells_t& addrs, dirty_formula_cells_t& cells);

/**
 * Calculate all dirty cells in order of dependency.
 *
 * @param cxt model context.
 * @param cells all dirty cells to be calculated.
 * @param thread_count number of calculation threads to use.  Note that
 *                     passing 0 will make the process use the main thread
 *                     only, while passing any number greater than 0 will
 *                     make the process spawn specified number of
 *                     calculation threads plus one additional thread to
 *                     manage the calculation threads.
 */
void IXION_DLLPUBLIC calculate_cells(
    iface::model_context& cxt, dirty_formula_cells_t& cells, size_t thread_count);

}

#endif
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
