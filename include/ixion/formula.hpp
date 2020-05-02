/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_FORMULA_HPP
#define INCLUDED_IXION_FORMULA_HPP

#include "ixion/formula_tokens.hpp"
#include "ixion/interface/formula_model_access.hpp"
#include "ixion/env.hpp"

#include <string>

namespace ixion {

class formula_name_resolver;

/**
 * Parse a raw formula expression string into formula tokens.
 *
 * @param cxt model context.
 * @param pos address of the cell that has the formula expression.
 * @param resolver name resolver object used to resolve name tokens.
 * @param p pointer to the first character of raw formula expression string.
 * @param n size of the raw formula expression string.
 *
 * @return formula tokens representing the parsed formula expression.
 */
IXION_DLLPUBLIC formula_tokens_t parse_formula_string(
    iface::formula_model_access& cxt, const abs_address_t& pos,
    const formula_name_resolver& resolver, const char* p, size_t n);

/**
 * Create a set of tokens that represent an invalid formula.
 *
 * @param cxt model context.
 * @param p_src_formula pointer to the buffer containing the original
 *                      formula string.
 * @param n_src_formula length of the buffer containinig the original
 *                      formula string.
 * @param p_error pointer to the buffer containing the error string.
 * @param n_error length of the buffer containing the erorr string.
 *
 * @return a set of tokens, the first of which is a token of type {@link
 *         fop_error}, followed by two string tokens.  The first string
 *         token stores original formula string, whereas the second one
 *         stores the error string.
 */
IXION_DLLPUBLIC formula_tokens_t create_formula_error_tokens(
    iface::formula_model_access& cxt, const char* p_src_formula, size_t n_src_formula,
    const char* p_error, size_t n_error);

/**
 * Convert formula tokens into a human-readable string representation.
 *
 * @param cxt model context.
 * @param pos address of the cell that has the formula tokens.
 * @param resolver name resolver object used to print name tokens.
 * @param tokens formula tokens.
 *
 * @return string representation of the formula tokens.
 */
IXION_DLLPUBLIC std::string print_formula_tokens(
    const iface::formula_model_access& cxt, const abs_address_t& pos,
    const formula_name_resolver& resolver, const formula_tokens_t& tokens);

/**
 * Convert an individual formula token intoa human-readable string
 * representation.
 *
 * @param cxt model context.
 * @param pos address of the cell that has the formula tokens.
 * @param resolver name resolver object used to print name tokens.
 * @param token formula token to convert.
 *
 * @return string representation of the formula token.
 */
IXION_DLLPUBLIC std::string print_formula_token(
    const iface::formula_model_access& cxt, const abs_address_t& pos,
    const formula_name_resolver& resolver, const formula_token& token);

/**
 * Regisiter a formula cell with cell dependency tracker.
 *
 * @param cxt model context.
 * @param pos address of the cell being registered.  In case of grouped
 *            cells, the position must be that of teh top-left cell of that
 *            group.
 * @param cell (optional) pointer to the formula cell object to register.
 *             You can skip this parameter, in which case the formula cell
 *             object will be fetched from the address of the cell.  But
 *             passing a pointer will save the overhead of fetching.
 */
void IXION_DLLPUBLIC register_formula_cell(
    iface::formula_model_access& cxt, const abs_address_t& pos, const formula_cell* cell = nullptr);

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
    iface::formula_model_access& cxt, const abs_address_t& pos);

/**
 * Get the positions of those formula cells that directly or indirectly
 * depend on the specified source cells.
 *
 * @param cxt model context.
 * @param modified_cells collection of the postiions of cells that have been
 *                       modified.
 *
 * @return collection of the positions of formula cells that directly or
 *         indirectly depend on at least one of the specified source cells.
 */
IXION_DLLPUBLIC abs_address_set_t query_dirty_cells(
    iface::formula_model_access& cxt, const abs_address_set_t& modified_cells);

/**
 * Get a sequence of the positions of all formula cells that track at least
 * one of the specified modified cells either directly or indirectly.  Such
 * formula cells are referred to as "dirty" formula cells.  The sequence
 * returned from this function is already sorted in topological order based
 * on the dependency relationships between the affected formula cells.  Note
 * that if the model contains volatile formula cells, they will be included
 * in the returned sequence each and every time.
 *
 * Use {@link ixion::query_dirty_cells} instead if you don't need the
 * results to be sorted in order of dependency, to avoid the extra overhead
 * incurred by the sorting.
 *
 * @param cxt model context.
 * @param modified_cells a collection of non-formula cells whose values have
 *                       been updated.  You can specify one or more ranges
 *                       of cells rather than individual cell positions.
 * @param dirty_formula_cells (optional) a collection of formula cells that
 *                            are already known to be dirty.  These formula
 *                            cells will be added to the list of the
 *                            affected formula cells returned from this
 *                            function.  Note that even though this
 *                            parameter is a set of cell ranges, regular
 *                            formula cell positions must be given as single
 *                            cell addresses.  Only the positions of grouped
 *                            formula cells must be given as ranges.
 *
 * @return an sequence containing the positions of the formula cells that
 *         track at least one of the modified cells, as well as those
 *         formula cells that are already known to be dirty.
 */
IXION_DLLPUBLIC std::vector<abs_range_t> query_and_sort_dirty_cells(
    iface::formula_model_access& cxt, const abs_range_set_t& modified_cells,
    const abs_range_set_t* dirty_formula_cells = nullptr);

/**
 * Calculate all specified formula cells in the order they occur in the
 * sequence.
 *
 * @param cxt model context.
 * @param formula_cells formula cells to be calculated.  The cells will be
 *                      calculated in the order they appear in the sequence.
 *                      In a typical use case, this will be the returned
 *                      value from {@link
 *                      ixion::query_and_sort_dirty_cells}.
 * @param thread_count number of calculation threads to use.  Note that
 *                     passing 0 will make the process use the main thread
 *                     only, while passing any number greater than 0 will
 *                     make the process spawn specified number of
 *                     calculation threads plus one additional thread to
 *                     manage the calculation threads.
 */
void IXION_DLLPUBLIC calculate_sorted_cells(
    iface::formula_model_access& cxt, const std::vector<abs_range_t>& formula_cells, size_t thread_count);

} // namespace ixion

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
