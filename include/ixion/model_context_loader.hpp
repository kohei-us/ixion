/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "env.hpp"
#include "formula_tokens_fwd.hpp"
#include "types.hpp"
#include "address.hpp"
#include "model_context.hpp"
#include "table.hpp"

#include <initializer_list>
#include <memory>
#include <string>
#include <string_view>

namespace ixion {

class formula_cell;
class formula_result;

/**
 * Loads content into a model in bulk.  It's a load-time wrapper on a model
 * the caller owns: it takes the place of model_context for the duration of
 * the load, defers the work that the model_context setters do on every call
 * until finalize(), and can be discarded once the load is done.
 *
 * The setters write and nothing else.  A formula cell doesn't get validated
 * or registered with the dirty cell tracker until finalize(), so the
 * formula cells don't depend on the named expressions and tables they
 * reference being loaded first.
 *
 * @note Each cell gets written at most once: a setter aimed at a cell that
 *       isn't empty throws model_context_error (loader_cell_not_empty).
 *       The model itself doesn't have to be empty, and more content can be
 *       loaded later with a new loader on the same model.
 *       Every sheet a formula references must exist when the formula gets
 *       set, and every named expression and table it references must exist
 *       when finalize() gets called.  Don't modify cells or calculate the
 *       model through model_context while a loader is working on it, and
 *       use one loader at a time per model.  finalize() ends the load:
 *       every call on the loader afterwards throws model_context_error
 *       (loader_already_finalized).  The destructor doesn't finalize; the
 *       formula cells of a loader that never got finalized stay
 *       unregistered.
 */
class IXION_DLLPUBLIC model_context_loader
{
    struct impl;
    std::unique_ptr<impl> mp_impl;

public:
    model_context_loader() = delete;
    model_context_loader(const model_context_loader&) = delete;
    model_context_loader& operator=(const model_context_loader&) = delete;

    /**
     * Constructor.
     *
     * @param cxt Model to load into.  It must outlive the loader.
     */
    explicit model_context_loader(model_context& cxt);
    ~model_context_loader();

    /** @copydoc model_context::append_sheet(std::string) */
    sheet_t append_sheet(std::string name);

    /** @copydoc model_context::set_named_expression(std::string, formula_tokens_t) */
    void set_named_expression(std::string name, formula_tokens_t expr);

    /** @copydoc model_context::set_named_expression(std::string, const abs_address_t&, formula_tokens_t) */
    void set_named_expression(std::string name, const abs_address_t& origin, formula_tokens_t expr);

    /** @copydoc model_context::set_named_expression(sheet_t, std::string, formula_tokens_t) */
    void set_named_expression(sheet_t sheet, std::string name, formula_tokens_t expr);

    /** @copydoc model_context::set_named_expression(sheet_t, std::string, const abs_address_t&, formula_tokens_t) */
    void set_named_expression(
        sheet_t sheet, std::string name, const abs_address_t& origin, formula_tokens_t expr);

    /** @copydoc model_context::set_table(table_t) */
    void set_table(table_t tab);

    /**
     * Append a new string to the indexed string pool, without checking for
     * duplicates.  An empty string gets appended too.
     *
     * @param s String to append.
     *
     * @return Identifier of the appended string.
     */
    string_id_t append_string(std::string_view s);

    /**
     * Add a string to the indexed string pool, unless the same string is
     * already in it.
     *
     * @param s String to add.
     *
     * @return Identifier of the string, existing or new.
     */
    string_id_t add_string(std::string_view s);

    /**
     * Set a numeric value to an empty cell.
     *
     * @param addr Position of the cell.
     * @param val Numeric value.
     */
    void set_numeric_cell(const abs_address_t& addr, double val);

    /**
     * Set a boolean value to an empty cell.
     *
     * @param addr Position of the cell.
     * @param val Boolean value.
     */
    void set_boolean_cell(const abs_address_t& addr, bool val);

    /**
     * Set a string value to an empty cell.  The string gets stored in the
     * model, so the caller doesn't need to keep it alive.
     *
     * @param addr Position of the cell.
     * @param s String value.
     */
    void set_string_cell(const abs_address_t& addr, std::string_view s);

    /**
     * Set a string from the indexed string pool to an empty cell.
     *
     * @param addr Position of the cell.
     * @param identifier Identifier of the string, as returned by
     *                   append_string() or add_string().
     */
    void set_string_cell(const abs_address_t& addr, string_id_t identifier);

    /**
     * Duplicate the value of the source cell to one or more empty cells
     * located immediately below it.
     *
     * @param src Position of the source cell to copy the value from.
     * @param n_dst Number of cells below to copy the value to.  It must be
     *              at least one.
     */
    void fill_down_cells(const abs_address_t& src, std::size_t n_dst);

    /**
     * A convenient way to mass-insert a range of cell values into empty
     * cells.  You can use a nested initializer list representing a range of
     * cell values.  The outer list represents rows.
     *
     * @param sheet Sheet index.
     * @param rows Nested list of cell values.  The outer list represents
     *             rows.
     */
    void set_cell_values(sheet_t sheet, std::initializer_list<model_context::input_row> rows);

    /**
     * Set a formula cell at an empty cell, like
     * model_context::set_formula_cell() except that the cell doesn't get
     * validated or registered with the dirty cell tracker until finalize().
     *
     * @param addr Address at which to set the formula cell.
     * @param tokens Formula tokens to put into the formula cell.
     *
     * @return Pointer to the formula cell instance inserted into the model.
     */
    formula_cell* set_formula_cell(const abs_address_t& addr, formula_tokens_t tokens);

    /**
     * @copydoc set_formula_cell(const abs_address_t&, formula_tokens_t)
     *
     * This variant takes a formula tokens store that can be shared between
     * multiple formula cell instances.
     */
    formula_cell* set_formula_cell(
        const abs_address_t& addr, const formula_tokens_store_ptr_t& tokens);

    /**
     * @copydoc set_formula_cell(const abs_address_t&, formula_tokens_t)
     *
     * This variant takes a formula tokens store that can be shared between
     * multiple formula cell instances, and a cached result.
     *
     * @param result Cached result of the formula cell.
     */
    formula_cell* set_formula_cell(
        const abs_address_t& addr, const formula_tokens_store_ptr_t& tokens, formula_result result);

    /**
     * Set a group of formula cells sharing one set of formula tokens over a
     * range of empty cells, like model_context::set_grouped_formula_cells()
     * except that the group doesn't get validated or registered with the
     * dirty cell tracker until finalize().
     *
     * @param group_range Range of the group.  It must be on one sheet.
     * @param tokens Formula tokens shared by all cells of the group.  They
     *               are relative to the top-left cell of the group.
     */
    void set_grouped_formula_cells(const abs_range_t& group_range, formula_tokens_t tokens);

    /**
     * @copydoc set_grouped_formula_cells(const abs_range_t&, formula_tokens_t)
     *
     * This variant takes a cached result.
     *
     * @param result Cached result of the group.  It must be a matrix whose
     *               dimensions equal those of the group.
     *
     * @throw std::invalid_argument When the result is not a matrix, or its
     *                              dimensions differ from those of the group.
     */
    void set_grouped_formula_cells(
        const abs_range_t& group_range, formula_tokens_t tokens, formula_result result);

    /**
     * Register the formula cells set through the loader with the dirty cell
     * tracker.  This ends the load; call it once.
     *
     * Every cell gets validated before any of them gets registered, so a
     * rejected cell leaves the tracker unchanged.  The rejected cell itself
     * stays in the model.
     *
     * @throw model_context_error When a reference in a formula, including
     *                            one reached through a named expression,
     *                            points at an invalid sheet
     *                            (invalid_sheet_reference), or when called a
     *                            second time (loader_already_finalized).
     */
    void finalize();
};

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
