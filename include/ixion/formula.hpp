/*************************************************************************
 *
 * Copyright (c) 2011 Kohei Yoshida
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 ************************************************************************/

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
 * @param cell instance of the cell being registered.
 */
void IXION_DLLPUBLIC register_formula_cell(
    iface::model_context& cxt, const abs_address_t& pos, formula_cell* cell);

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
 * @param addrs list of addresses of cells that have been modified.
 * @param cells all dirty cells are inserted into this container when this
 *              function returns.
 */
void IXION_DLLPUBLIC get_all_dirty_cells(
    iface::model_context& cxt, const dirty_cell_addrs_t& addrs, dirty_cells_t& cells);

/**
 * Calculate all dirty cells in order of dependency.
 *
 * @param cxt model context.
 * @param cells all dirty cells to be calculated.
 * @param thread_count number of calculation threads to use.  Note that
 *                     passing 0 will make the process use the main thread
 *                     only, while passing any number greater than 0 will
 *                     make the process spawn specified number of
 *                     calculation threads plus one additional method
 *                     (besides the main thread) to manage the calculation
 *                     threads.
 */
void IXION_DLLPUBLIC calculate_cells(
    iface::model_context& cxt, dirty_cells_t& cells, size_t thread_count);

}

#endif
