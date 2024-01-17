/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_MODEL_CONTEXT_HPP
#define INCLUDED_IXION_MODEL_CONTEXT_HPP

#include "env.hpp"
#include "formula_tokens_fwd.hpp"
#include "types.hpp"

#include <string>
#include <memory>
#include <variant>

namespace ixion {

class cell_access;
class dirty_cell_tracker;
class formula_cell;
class formula_name_resolver;
class formula_result;
class matrix;
class model_iterator;
class named_expressions_iterator;
struct abs_address_t;
struct abs_range_t;
struct abs_rc_range_t;
struct config;
struct named_expression_t;

namespace iface {

class session_handler;
class table_handler;

}

namespace detail {

class model_context_impl;

}

/**
 * This class stores all data relevant to current session.  You can think of
 * this like a document model for each formula calculation run.  Note that
 * only those methods called from the formula interpreter are specified in
 * the interface; this explains why accessors for the most part only have
 * the 'get' method not paired with its 'set' counterpart.
 */
class IXION_DLLPUBLIC model_context final
{
    friend class named_expressions_iterator;
    friend class cell_access;

    std::unique_ptr<detail::model_context_impl> mp_impl;

public:
    class IXION_DLLPUBLIC session_handler_factory
    {
    public:
        virtual std::unique_ptr<iface::session_handler> create();
        virtual ~session_handler_factory();
    };

    /**
     * Cell value only to be used to input a collection of cells to sheet.
     * Formula cells are not supported.
     */
    struct IXION_DLLPUBLIC input_cell
    {
        using value_type = std::variant<bool, double, std::string_view>;

        celltype_t type;
        value_type value;

        /** Initializes the cell to be empty. */
        input_cell(std::nullptr_t);
        /** Boolean cell value. */
        input_cell(bool b);
        /** The char array must be null-terminated. */
        input_cell(const char* s);
        /** Numeric cell value. */
        input_cell(double v);

        input_cell(const input_cell& other);
    };

    class IXION_DLLPUBLIC input_row
    {
        std::initializer_list<input_cell> m_cells;
    public:
        input_row(std::initializer_list<input_cell> cells);

        const std::initializer_list<input_cell>& cells() const;
    };

    model_context();
    model_context(const rc_size_t& sheet_size);
    ~model_context();

    /**
     * Query the current policy on what to do when a formula cell result is
     * being requested while the result has not yet been computed.
     */
    formula_result_wait_policy_t get_formula_result_wait_policy() const;

    /**
     * This method is used to notify the model access implementer of formula
     * events.
     *
     * @param event event type.
     */
    void notify(formula_event_t event);

    const config& get_config() const;
    dirty_cell_tracker& get_cell_tracker();
    const dirty_cell_tracker& get_cell_tracker() const;

    bool is_empty(const abs_address_t& addr) const;
    bool is_empty(const abs_range_t& range) const;
    celltype_t get_celltype(const abs_address_t& addr) const;
    cell_value_t get_cell_value_type(const abs_address_t& addr) const;

    /**
     * Get a numeric representation of the cell value at specified position.
     * If the cell at the specified position is a formula cell and its result
     * has not yet been computed, it will block until the result becomes
     * available.
     *
     * @param addr position of the cell.
     *
     * @return numeric representation of the cell value.
     */
    double get_numeric_value(const abs_address_t& addr) const;
    bool get_boolean_value(const abs_address_t& addr) const;
    string_id_t get_string_identifier(const abs_address_t& addr) const;

    /**
     * Get a string value associated with the cell at the specified position.
     * It returns a valid string value only when the cell is a string cell, or
     * is a formula cell containing a string result.  Otherwise, it returns a
     * nullptr.
     *
     * @param addr position of the cell.
     *
     * @return pointer to a string value if the cell stores a valid string
     *         value, else nullptr.
     */
    std::string_view get_string_value(const abs_address_t& addr) const;
    const formula_cell* get_formula_cell(const abs_address_t& addr) const;
    formula_cell* get_formula_cell(const abs_address_t& addr);

    formula_result get_formula_result(const abs_address_t& addr) const;

    /**
     * Get a named expression token set associated with specified name if
     * present.  It first searches the local sheet scope for the name, then if
     * it's not present, it searches the global scope.
     *
     * @param sheet index of the sheet scope to search in.
     * @param name name of the expression.
     *
     * @return const pointer to the token set if exists, nullptr otherwise.
     */
    const named_expression_t* get_named_expression(sheet_t sheet, std::string_view name) const;

    double count_range(const abs_range_t& range, values_t values_type) const;

    /**
     * Obtain range value in matrix form.  Multi-sheet ranges are not
     * supported.  If the specified range consists of multiple sheets, it
     * throws an exception.
     *
     * @param range absolute, single-sheet range address.  Multi-sheet ranges
     *              are not allowed.
     *
     * @return range value represented as matrix.
     */
    matrix get_range_value(const abs_range_t& range) const;

    /**
     * Session handler instance receives various events from the formula
     * interpretation run, in order to respond to those events.  This is
     * optional; the model context implementation is not required to provide a
     * handler.
     *
     * @return a new session handler instance.  It may be nullptr.
     */
    std::unique_ptr<iface::session_handler> create_session_handler();

    /**
     * Table interface provides access to all table ranges stored in the
     * document model.  A table is a 2-dimensional range of cells that include
     * named columns.  It is used when resolving a table reference that refers
     * to a cell or a range of cells by the table name and/or column name.
     *
     * @return non-null pointer to the table storage inside the model, or
     *         nullptr if no table is present or supported by the model
     *         implementation.
     */
    iface::table_handler* get_table_handler();
    const iface::table_handler* get_table_handler() const;

    /**
     * Try to add a new string to the string pool. If the same string already
     * exists in the pool, the new string won't be added to the pool.
     *
     * @param s string to try to add to the pool.
     *
     * @return string_id_t integer value representing the string.
     */
    string_id_t add_string(std::string_view s);
    const std::string* get_string(string_id_t identifier) const;

    /**
     * Get the index of sheet from sheet name.  If the sheet name doesn't exist,
     * it returns a value equal to <code>ixion::invalid_sheet</code>.
     *
     * @param name sheet name.
     *
     * @return 0-based sheet index, or <code>ixion::invalid_sheet</code> in case
     *         the document doesn't have a sheet by the specified name.
     */
    sheet_t get_sheet_index(std::string_view name) const;

    /**
     * Get the name of a sheet specified by a 0-based sheet index.
     *
     * @param sheet 0-based sheet index.
     *
     * @return Name of the sheet if the sheet index is valid.
     *
     * @exception std::invalid_argument When the sheet index is invalid.
     */
    std::string_view get_sheet_name(sheet_t sheet) const;

    /**
     * Set a new name to an existing sheet.
     *
     * @param sheet 0-based sheet index.
     * @param name New name of a sheet.
     *
     * @exception std::invalid_argument When the sheet index is invalid.
     */
    void set_sheet_name(sheet_t sheet, std::string name);

    /**
     * Get the size of a sheet.
     *
     * @return sheet size.
     */
    rc_size_t get_sheet_size() const;

    /**
     * Return the number of sheets.
     *
     * @return number of sheets.
     */
    size_t get_sheet_count() const;

    string_id_t append_string(std::string_view s);

    void set_sheet_size(const rc_size_t& sheet_size);
    void set_config(const config& cfg);

    void empty_cell(const abs_address_t& addr);

    void set_numeric_cell(const abs_address_t& addr, double val);
    void set_boolean_cell(const abs_address_t& adr, bool val);
    void set_string_cell(const abs_address_t& addr, std::string_view s);
    void set_string_cell(const abs_address_t& addr, string_id_t identifier);

    cell_access get_cell_access(const abs_address_t& addr) const;

    /**
     * Duplicate the value of the source cell to one or more cells located
     * immediately below it.
     *
     * @param src position of the source cell to copy the value from.
     * @param n_dst number of cells below to copy the value to.  It must be at
     *              least one.
     */
    void fill_down_cells(const abs_address_t& src, size_t n_dst);

    /**
     * Set a formula cell at a specified address.
     *
     * @param addr address at which to set a formula cell.
     * @param tokens formula tokens to put into the formula cell.
     *
     * @return pointer to the formula cell instance inserted into the model.
     */
    formula_cell* set_formula_cell(const abs_address_t& addr, formula_tokens_t tokens);

    /**
     * Set a formula cell at a specified address.  This variant takes a
     * formula tokens store that can be shared between multiple formula cell
     * instances.
     *
     * @param addr address at which to set a formula cell.
     * @param tokens formula tokens to put into the formula cell.
     *
     * @return pointer to the formula cell instance inserted into the model.
     */
    formula_cell* set_formula_cell(const abs_address_t& addr, const formula_tokens_store_ptr_t& tokens);

    /**
     * Set a formula cell at a specified address.  This variant takes a
     * formula tokens store that can be shared between multiple formula cell
     * instances.
     *
     * @param addr address at which to set a formula cell.
     * @param tokens formula tokens to put into the formula cell.
     * @param result cached result of this formula cell.
     *
     * @return pointer to the formula cell instance inserted into the model.
     */
    formula_cell* set_formula_cell(const abs_address_t& addr, const formula_tokens_store_ptr_t& tokens, formula_result result);

    void set_grouped_formula_cells(const abs_range_t& group_range, formula_tokens_t tokens);

    void set_grouped_formula_cells(const abs_range_t& group_range, formula_tokens_t tokens, formula_result result);

    abs_range_t get_data_range(sheet_t sheet) const;

    /**
     * Set a named expression associated with a string name in the global
     * scope.
     *
     * @param name name of the expression.
     * @param expr formula tokens to use for the named expression.
     */
    void set_named_expression(std::string name, formula_tokens_t expr);

    /**
     * Set a named expression associated with a string name in the global
     * scope.
     *
     * @param name name of the expression.
     * @param origin position of the origin cell.  Origin cell is relevant
     *               only when you need to convert the tokens into a string
     *               representation.
     * @param expr formula tokens to use for the named expression.
     */
    void set_named_expression(std::string name, const abs_address_t& origin, formula_tokens_t expr);

    /**
     * Set a named expression associated with a string name in a sheet-local
     * scope.
     *
     * @param sheet 0-based index of the sheet to register this expression
     *              with.
     * @param name name of the expression.
     * @param expr formula tokens to use for the named expression.
     */
    void set_named_expression(sheet_t sheet, std::string name, formula_tokens_t expr);

    /**
     * Set a named expression associated with a string name in a sheet-local
     * scope.
     *
     * @param sheet 0-based index of the sheet to register this expression
     *              with.
     * @param name name of the expression.
     * @param origin position of the origin cell.  Origin cell is relevant
     *               only when you need to convert the tokens into a string
     *               representation.
     * @param expr formula tokens to use for the named expression.
     */
    void set_named_expression(sheet_t sheet, std::string name, const abs_address_t& origin, formula_tokens_t expr);

    /**
     * Append a new sheet to the model.  The caller must ensure that the name
     * of the new sheet is unique within the model context.  When the name
     * being used for the new sheet already exists, it throws a
     * model_context_error exception.
     *
     * @param name name of the sheet to be inserted.
     *
     * @return sheet index of the inserted sheet.
     *
     * @throw model_context_error
     */
    sheet_t append_sheet(std::string name);

    /**
     * A convenient way to mass-insert a range of cell values.  You can
     * use a nested initializet list representing a range of cell values.  The
     * outer list represents rows.
     *
     * @param sheet sheet index.
     * @param rows nested list of cell values.  The outer list represents
     *             rows.
     */
    void set_cell_values(sheet_t sheet, std::initializer_list<input_row> rows);

    void set_session_handler_factory(session_handler_factory* factory);

    void set_table_handler(iface::table_handler* handler);

    size_t get_string_count() const;

    void dump_strings() const;

    /**
     * Get an integer string ID from a string value.  If the string value
     * doesn't exist in the pool, the value equal to empty_string_id gets
     * returned.
     *
     * @param s string value.
     *
     * @return string_id_t integer string ID associated with the string value
     *         given.
     */
    string_id_t get_identifier_from_string(std::string_view s) const;

    /**
     * Get an immutable iterator that lets you iterate cell values in one
     * sheet one at a time.  <i>The caller has to ensure that the model
     * content does not change for the duration of the iteration.</i>
     *
     * @param sheet sheet index.
     * @param dir direction of the iteration.
     * @param range range on the specified sheet to iterate over.
     *
     * @return model iterator instance.
     */
    model_iterator get_model_iterator(
        sheet_t sheet, rc_direction_t dir, const abs_rc_range_t& range) const;

    /**
     * Get an iterator for global named expressions.
     */
    named_expressions_iterator get_named_expressions_iterator() const;

    /**
     * Get an interator for sheet-local named expressions.
     *
     * @param sheet 0-based index of the sheet where the named expressions are
     *              stored.
     */
    named_expressions_iterator get_named_expressions_iterator(sheet_t sheet) const;

    void walk(
        sheet_t sheet, const abs_rc_range_t& range, column_block_callback_t cb) const;

    bool empty() const;
};

}

#endif
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
