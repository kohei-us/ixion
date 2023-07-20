/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_FORMULA_HPP
#define INCLUDED_IXION_FORMULA_HPP

#include "formula_tokens.hpp"
#include "types.hpp"
#include "env.hpp"

#include <string>

namespace ixion {

class formula_cell;
class formula_name_resolver;
class model_context;

/**
 * Parse a raw formula expression string into formula tokens.
 *
 * @param cxt model context.
 * @param pos address of the cell that has the formula expression.
 * @param resolver name resolver object used to resolve name tokens.
 * @param formula raw formula expression string to parse.
 *
 * @return formula tokens representing the parsed formula expression.
 */
IXION_DLLPUBLIC formula_tokens_t parse_formula_string(
    model_context& cxt, const abs_address_t& pos,
    const formula_name_resolver& resolver, std::string_view formula);

/**
 * Create a set of tokens that represent an invalid formula.
 *
 * This can be used for a cell containing an invalid formula expression, and
 * the error information needs to be preserved.
 *
 * @param cxt model context.
 * @param src_formula original formula string.
 * @param error error string.
 *
 * @return a set of tokens, the first of which is a token of type fop_error,
 *         followed by two string tokens.  The second token stores the
 *         original formula string, whereas the third one stores the error
 *         string.  The first token stores the number of tokens that follows
 *         as its value of type std::size_t, which is always 2 in the current
 *         implementation.
 */
IXION_DLLPUBLIC formula_tokens_t create_formula_error_tokens(
    model_context& cxt, std::string_view src_formula,
    std::string_view error);

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
    const model_context& cxt, const abs_address_t& pos,
    const formula_name_resolver& resolver, const formula_tokens_t& tokens);

/**
 * Convert formula tokens into a human-readable string representation.
 *
 * @param config Configuration options for printing preferences.
 * @param cxt Model context.
 * @param pos Address of the cell that has the formula tokens.
 * @param resolver Name resolver object used to print name tokens.
 * @param tokens Formula tokens to print.
 *
 * @return string representation of the formula tokens.
 */
IXION_DLLPUBLIC std::string print_formula_tokens(
    const print_config& config, const model_context& cxt, const abs_address_t& pos,
    const formula_name_resolver& resolver, const formula_tokens_t& tokens);

/**
 * Convert an individual formula token into a human-readable string
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
    const model_context& cxt, const abs_address_t& pos,
    const formula_name_resolver& resolver, const formula_token& token);

/**
 * Convert an individual formula token into a human-readable string
 * representation.
 *
 * @param config Configuration options for printing preferences.
 * @param cxt Model context.
 * @param pos Address of the cell that has the formula tokens.
 * @param resolver Name resolver object used to print name tokens.
 * @param token Formula token to convert.
 *
 * @return string representation of the formula token.
 */
IXION_DLLPUBLIC std::string print_formula_token(
    const print_config& config, const model_context& cxt, const abs_address_t& pos,
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
    model_context& cxt, const abs_address_t& pos, const formula_cell* cell = nullptr);

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
    model_context& cxt, const abs_address_t& pos);

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
    model_context& cxt, const abs_address_set_t& modified_cells);

/**
 * Get a sequence of the positions of all formula cells that track at least
 * one of the specified modified cells either directly or indirectly.  Such
 * formula cells are referred to as "dirty" formula cells.  The sequence
 * returned from this function is already sorted in topological order based
 * on the dependency relationships between the affected formula cells.  Note
 * that if the model contains volatile formula cells, they will be included
 * in the returned sequence each and every time.
 *
 * Use query_dirty_cells() instead if you don't need the results to be sorted
 * in order of dependency, to avoid the extra overhead incurred by the
 * sorting.
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
    model_context& cxt, const abs_range_set_t& modified_cells,
    const abs_range_set_t* dirty_formula_cells = nullptr);

/**
 * Calculate all specified formula cells in the order they occur in the
 * sequence.
 *
 * @param cxt model context.
 * @param formula_cells formula cells to be calculated.  The cells will be
 *                      calculated in the order they appear in the sequence.
 *                      In a typical use case, this will be the returned
 *                      value from query_and_sort_dirty_cells.
 * @param thread_count number of calculation threads to use.  Note that
 *                     passing 0 will make the process use the main thread
 *                     only, while passing any number greater than 0 will
 *                     make the process spawn specified number of
 *                     calculation threads plus one additional thread to
 *                     manage the calculation threads.
 */
void IXION_DLLPUBLIC calculate_sorted_cells(
    model_context& cxt, const std::vector<abs_range_t>& formula_cells, size_t thread_count);

} // namespace ixion

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
