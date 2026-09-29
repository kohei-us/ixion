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

#include <iosfwd>
#include <string>
#include <memory>
#include <variant>
#include <vector>

namespace ixion {

class cell_access;
class dirty_cell_tracker;
class formula_cell;
class formula_name_resolver;
class formula_result;
class matrix;
class model_cell_range;
class model_iterator;
class named_expressions_iterator;
class named_expressions_range;
struct abs_address_t;
struct abs_range_t;
struct abs_rc_range_t;
struct config;
struct named_expression_t;
struct table_ref_t;
struct table_t;

namespace iface {

class session_handler;

}

namespace detail {

class model_context_impl;

}

class sheet_view;

/**
 * This class stores all cell values of different types organized in multiple
 * sheets. It also stores named expressions both in global scope and
 * sheet-local scope, as well as tables and the dirty cell tracker used
 * during calculation.  It holds only what the formula engine needs in
 * order to perform a full calculation.
 *
 * @note Overwriting or emptying a formula cell does not remove it from the
 *       dirty cell tracker.  Call unregister_formula_cell() on the cell
 *       before replacing its content.
 */
class IXION_DLLPUBLIC model_context final
{
    friend class named_expressions_iterator;
    friend class cell_access;

    std::unique_ptr<detail::model_context_impl> mp_impl;

public:
    /**
     * Constructs a new @ref iface::session_handler instance for each
     * formula-cell evaluation.
     *
     * The factory indirection exists so cell calculation can run in
     * parallel: each cell-evaluation thread asks the factory for its
     * own handler instance, so handlers accumulate per-cell trace
     * state without sharing mutable data across threads.  The host
     * configures the factory once via
     * @ref set_session_handler_factory; the model and the formula
     * interpreter call @ref create per cell to obtain a fresh handler.
     */
    class IXION_DLLPUBLIC session_handler_factory
    {
    public:
        /**
         * Create a new session handler instance.  The default implementation
         * returns nullptr, which means no handler.
         *
         * @return New session handler instance, or nullptr for no handler.
         */
        virtual std::unique_ptr<iface::session_handler> create() const;
        virtual ~session_handler_factory();
    };

    /**
     * Cell value only to be used to input a collection of cells to sheet.
     * Formula cells are not supported.
     */
    struct IXION_DLLPUBLIC input_cell
    {
        /** Type of the stored value, one of boolean, numeric or string. */
        using value_type = std::variant<bool, double, std::string_view>;

        /** Type of the cell.  For an empty cell, the value is not used. */
        cell_t type;
        /** Value of the cell. */
        value_type value;

        /** Initializes the cell to be empty. */
        input_cell(std::nullptr_t);
        /** Boolean cell value. */
        input_cell(bool b);
        /** The char array must be null-terminated. */
        input_cell(const char* s);
        /** Numeric cell value. */
        input_cell(double v);

        /** Copy constructor. */
        input_cell(const input_cell& other);
    };

    /**
     * One row of cell values to be passed to set_cell_values().
     */
    class IXION_DLLPUBLIC input_row
    {
        std::initializer_list<input_cell> m_cells;
    public:
        /**
         * Constructor.
         *
         * @param cells Cell values of the row, from the first column onward.
         */
        input_row(std::initializer_list<input_cell> cells);

        /**
         * Get the cell values of the row.
         *
         * @return Cell values of the row.
         */
        const std::initializer_list<input_cell>& cells() const;
    };

    /**
     * Construct a model with the default sheet size, which is 1048576 rows
     * by 16384 columns.
     */
    model_context();

    /**
     * Construct a model with a custom sheet size.  All sheets in the model
     * share the same size.
     *
     * @param sheet_size Number of rows and columns of each sheet.
     */
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

    /**
     * Get the configuration of the model.
     *
     * @return Current configuration.
     */
    const config& get_config() const;

    /**
     * Get the dirty cell tracker of the model, which records the
     * dependencies between formula cells and the cells they reference.
     *
     * @return Dirty cell tracker.
     */
    dirty_cell_tracker& get_cell_tracker();

    /** @copydoc get_cell_tracker() */
    const dirty_cell_tracker& get_cell_tracker() const;

    /**
     * Check whether a cell is empty.
     *
     * @param addr Position of the cell.
     *
     * @return True if the cell is empty, false otherwise.
     */
    bool is_empty(const abs_address_t& addr) const;

    /**
     * Check whether all cells in a range are empty.  The part of the range
     * that lies outside the sheets is ignored.
     *
     * @param range Range to check.
     *
     * @return True if the range contains no cell values, false otherwise.
     */
    bool is_empty(const abs_range_t& range) const;

    /**
     * Get the type of a cell.  A formula cell is reported as a formula cell
     * regardless of its result.
     *
     * @param addr Position of the cell.
     *
     * @return Type of the cell.
     */
    cell_t get_celltype(const abs_address_t& addr) const;

    /**
     * Get the type of the value a cell holds.  Unlike get_celltype(), a
     * formula cell is classified by the type of its result, which may be an
     * error type.
     *
     * @param addr Position of the cell.
     *
     * @return Type of the cell value.
     */
    cell_value_t get_cell_value_type(const abs_address_t& addr) const;

    /**
     * Get a numeric representation of the cell value at the specified position.
     * A boolean cell yields 1 or 0, and a string or empty cell yields 0.  For
     * a formula cell, this is the numeric value of its result; if the result
     * has not yet been computed, the call blocks until it becomes available
     * while a calculation is in progress.
     *
     * @param addr position of the cell.
     *
     * @return numeric representation of the cell value.
     *
     * @throw formula_error If the cell is a formula cell whose result is an
     *                      error or cannot be converted to a number.
     *
     * @note When the result is a matrix, the value of one element is returned:
     *       the element at the cell's position within its group for a grouped
     *       formula cell, or the top-left element for a non-grouped formula
     *       cell.
     */
    double get_numeric_value(const abs_address_t& addr) const;

    /**
     * Get a boolean representation of the cell value at the specified position.
     * A numeric value, or the numeric result of a formula cell, is true when
     * it is not zero.  A string or empty cell is false.
     *
     * @param addr Position of the cell.
     *
     * @return Boolean representation of the cell value.
     *
     * @throw formula_error If the cell is a formula cell whose result is an
     *                      error or cannot be converted to a number.
     */
    bool get_boolean_value(const abs_address_t& addr) const;

    /**
     * Get the string identifier of a string cell.  Only a cell whose string
     * was set by its identifier, via the string_id_t overload of
     * set_string_cell(), has one.  For any other cell, including a string
     * cell with an inline string value, this returns empty_string_id.
     *
     * @param addr Position of the cell.
     *
     * @return String identifier of the cell, or empty_string_id.
     */
    string_id_t get_string_identifier(const abs_address_t& addr) const;

    /**
     * Get the string value of the cell at the specified position.  A string
     * cell yields its text whether the string is stored inline or by its
     * identifier in the indexed string pool, and a formula cell yields its
     * string result.
     *
     * @param addr Position of the cell.
     *
     * @return String value of the cell, or an empty view if the cell is
     *         neither a string cell nor a formula cell.
     *
     * @throw formula_error If the cell is a formula cell whose result is an
     *                      error or not a string.
     *
     * @note When the result is a matrix, the value of one element is returned:
     *       the element at the cell's position within its group for a grouped
     *       formula cell, or the top-left element for a non-grouped formula
     *       cell.
     */
    std::string_view get_string_value(const abs_address_t& addr) const;

    /**
     * Get the formula cell at the specified position for reading.  The cell
     * storage is left as is, so a column that is still shared with a copied
     * sheet stays shared.
     *
     * @param addr Position of the cell.
     *
     * @return Pointer to the formula cell, or nullptr if the cell is not a
     *         formula cell.  The pointer remains valid until the cell is
     *         overwritten or emptied.
     */
    const formula_cell* get_formula_cell(const abs_address_t& addr) const;

    /**
     * Get the formula cell at the specified position for modification.  If
     * the column the cell is in is still shared with a copied sheet, this
     * gives the sheet its own copy of the column first, since the returned
     * cell may get modified.  Use the const overload when you only need to
     * read the cell.
     *
     * @param addr Position of the cell.
     *
     * @return Pointer to the formula cell, or nullptr if the cell is not a
     *         formula cell.  The pointer remains valid until the cell is
     *         overwritten or emptied.
     */
    formula_cell* get_formula_cell(const abs_address_t& addr);

    /**
     * Get the cached result of the formula cell at the specified position.
     * For a grouped formula cell, the result is the single value assigned to
     * the position of the cell within its group.
     *
     * @param addr Position of the formula cell.
     *
     * @return Cached result of the formula cell.
     *
     * @note A non-grouped formula cell whose result is a matrix gets the
     *       matrix back as is.
     */
    formula_result get_formula_result(const abs_address_t& addr) const;

    /**
     * Get a named expression token set associated with the specified name if
     * present.  It first searches the local sheet scope for the name, then if
     * it's not present, it searches the global scope.
     *
     * @param sheet index of the sheet scope to search in.
     * @param name name of the expression.
     *
     * @return const pointer to the token set if exists, nullptr otherwise.
     */
    const named_expression_t* get_named_expression(sheet_t sheet, std::string_view name) const;

    /**
     * Count the cells in a range whose values are of the specified types.  A
     * formula cell is classified by the type of its result.  The part of the
     * range that lies outside the sheets is ignored.
     *
     * @param range Range to count the cells in.  It may span multiple sheets.
     * @param values_type Types of values to count, as a combination of
     *                    value_t flags.
     *
     * @return Number of matching cells.
     */
    std::size_t count_range(const abs_range_t& range, values_t values_type) const;

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
    std::unique_ptr<iface::session_handler> create_session_handler() const;

    /**
     * Get a table associated with the specified name.
     *
     * @param name Name of the table.
     *
     * @return Const pointer to the table if it exists, nullptr otherwise.
     *         The pointer remains valid while the table remains in the
     *         model.
     */
    const table_t* get_table(std::string_view name) const;

    /**
     * Get all tables whose ranges are on the specified sheet, sorted by
     * name in ascending order.
     *
     * @param sheet 0-based index of the sheet.
     *
     * @return Const pointers to the tables on the specified sheet.
     */
    std::vector<const table_t*> get_tables(sheet_t sheet) const;

    /**
     * Resolve a named table reference to the range it references.
     *
     * @param name Name of the table.
     * @param column_first Name of the first column of the reference, or an
     *                     empty view when the reference only contains area
     *                     specifiers.
     * @param column_last Name of the last column of the reference, or an
     *                    empty view for a single-column reference.
     * @param areas Area specifier value, which may consist of one or more
     *              values of ixion::table_area_t.
     *
     * @return Referenced range, or an invalid range when the reference
     *         does not resolve.
     */
    abs_range_t get_table_range(
        std::string_view name, std::string_view column_first,
        std::string_view column_last, table_areas_t areas) const;

    /**
     * Resolve an unnamed table reference to the range it references.  The
     * position of the referencing cell determines which table the
     * reference is for.
     *
     * @param pos Position of the referencing cell.
     * @param column_first Name of the first column of the reference, or an
     *                     empty view when the reference only contains area
     *                     specifiers.
     * @param column_last Name of the last column of the reference, or an
     *                    empty view for a single-column reference.
     * @param areas Area specifier value, which may consist of one or more
     *              values of ixion::table_area_t.
     *
     * @return Referenced range, or an invalid range when the reference
     *         does not resolve.
     */
    abs_range_t get_table_range(
        const abs_address_t& pos, std::string_view column_first,
        std::string_view column_last, table_areas_t areas) const;

    /**
     * Resolve a table reference to the range it references.  When the
     * reference does not include a table name, the position of the
     * referencing cell determines which table the reference is for.
     *
     * @param ref Table reference to resolve.
     * @param pos Position of the referencing cell, only used when the
     *            reference does not include a table name.
     *
     * @return Referenced range, or an invalid range when the reference
     *         does not resolve.
     */
    abs_range_t get_table_range(const table_ref_t& ref, const abs_address_t& pos) const;

    /**
     * Append a new string to the string pool.  The string being passed will be
     * inserted into the pool with a new string ID which is the current maximum
     * ID incremented by one.
     *
     * @param s String to append to the pool.
     *
     * @return Integer value associated with the appended string.
     *
     * @note This function will not check for duplicates in the pool.  An empty
     *       string will also be inserted into the pool.
     */
    string_id_t append_string(std::string_view s);

    /**
     * Try to add a new string to the indexed string pool.  If the same
     * string already exists in the pool, the existing id is returned and
     * no new entry is added.
     *
     * @param s string to try to add to the pool.
     *
     * @return integer id representing the string.
     */
    string_id_t add_string(std::string_view s);

    /**
     * Get a string from the indexed string pool by its identifier.
     *
     * @param identifier Identifier of the string, as returned by
     *                   append_string() or add_string().
     *
     * @return Pointer to the string, or nullptr if no string has that
     *         identifier.  The pointer remains valid for the lifetime of the
     *         model.
     */
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

    /**
     * Set the size of the sheets.  This is only allowed while the model has
     * no sheets.
     *
     * @param sheet_size Number of rows and columns of each sheet.
     *
     * @throw model_context_error When the model already has a sheet.
     */
    void set_sheet_size(const rc_size_t& sheet_size);

    /**
     * Set the configuration of the model.
     *
     * @param cfg New configuration.
     */
    void set_config(const config& cfg);

    /**
     * Empty a cell, discarding whatever value it holds.
     *
     * @param addr Position of the cell.
     */
    void empty_cell(const abs_address_t& addr);

    /**
     * Set a numeric value to a cell, replacing its current content.
     *
     * @param addr Position of the cell.
     * @param val Numeric value.
     */
    void set_numeric_cell(const abs_address_t& addr, double val);

    /**
     * Set a boolean value to a cell, replacing its current content.
     *
     * @param adr Position of the cell.
     * @param val Boolean value.
     */
    void set_boolean_cell(const abs_address_t& adr, bool val);

    /**
     * Set a string value to a cell, replacing its current content.  The
     * string gets stored in the model, so the caller doesn't need to keep it
     * alive.
     *
     * @param addr Position of the cell.
     * @param s String value.
     */
    void set_string_cell(const abs_address_t& addr, std::string_view s);

    /**
     * Set a string from the indexed string pool to a cell, replacing its
     * current content.
     *
     * @param addr Position of the cell.
     * @param identifier Identifier of the string, as returned by
     *                   append_string() or add_string().
     */
    void set_string_cell(const abs_address_t& addr, string_id_t identifier);

    /**
     * Get an accessor for a cell, for repeated queries on the same cell.
     *
     * @param addr Position of the cell.
     *
     * @return Accessor for the cell.
     */
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

    /**
     * Set a group of formula cells sharing one set of formula tokens over a
     * range.  This is how an array formula is stored: the tokens get
     * interpreted once, at the top-left cell of the group, and each cell of
     * the group takes its own element of the resulting matrix as its value.
     * Register the group with the dirty cell tracker through its top-left
     * cell.
     *
     * @param group_range Range of the group.  It must be on one sheet.
     * @param tokens Formula tokens shared by all cells of the group.  They
     *               are relative to the top-left cell of the group.
     */
    void set_grouped_formula_cells(const abs_range_t& group_range, formula_tokens_t tokens);

    /**
     * Set a group of formula cells sharing one set of formula tokens over a
     * range, with a cached result.
     *
     * @param group_range Range of the group.  It must be on one sheet.
     * @param tokens Formula tokens shared by all cells of the group.  They
     *               are relative to the top-left cell of the group.
     * @param result Cached result of the group.  It must be a matrix whose
     *               dimensions equal those of the group.
     *
     * @throw std::invalid_argument When the result is not a matrix, or its
     *                              dimensions differ from those of the group.
     */
    void set_grouped_formula_cells(const abs_range_t& group_range, formula_tokens_t tokens, formula_result result);

    /**
     * Get the smallest range that covers all non-empty cells of a sheet.
     *
     * @param sheet Index of the sheet.
     *
     * @return Range covering the data of the sheet, or an invalid range if
     *         the sheet has no non-empty cells.
     */
    abs_rc_range_t get_data_range(sheet_t sheet) const;

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
     * Result of a sheet-copy operation performed by append_sheet_copy().
     * More attributes may get added in the future as needed.
     */
    struct IXION_DLLPUBLIC sheet_copy_result
    {
        /** Index of the newly-inserted sheet. */
        sheet_t sheet = invalid_sheet;

        /**
         * Positions of the formula cells on the new sheet whose cached
         * results, carried over from their source cells, may no longer be
         * valid on the new sheet and therefore need re-calculating.
         *
         * Note that volatile formula cells are not included; they get
         * re-calculated on every calculation run anyway once registered.
         */
        abs_range_set_t recalc_cells;
    };

    /**
     * Append a new sheet to the model as a copy of an existing sheet.  All
     * cells of the source sheet get copied over to the new sheet.  A copied
     * formula cell shares its formula token store with its source cell, and
     * the cached results of the source formula cells carry over to their
     * copied counterparts.  The sheet-local named expressions of the source
     * sheet also get copied, with their origins re-anchored to the new sheet.
     * The tables of the source sheet also get copied, with their names
     * auto-renamed to unique names, and the table references of the copied
     * formula cells get rewritten to reference the copied tables.  A
     * formula cell whose table references get rewritten receives its own
     * new token store instead of sharing one with its source cell.
     *
     * Note that the formula cells of the new sheet do not get registered for
     * dependency tracking; that remains the responsibility of the caller.
     * The formula cells whose carried-over results may no longer be valid on
     * the new sheet get reported in the returned result object; the caller
     * should have them re-calculated.
     *
     * @param src Index of the sheet to copy.
     * @param name Name of the sheet to be inserted.  The caller must ensure
     *             that it is unique within the model context, else a
     *             model_context_error exception gets thrown.
     *
     * @return Result of the copy operation, which includes the sheet index of
     *         the inserted sheet.
     *
     * @throw model_context_error When the sheet name already exists.
     * @throw std::invalid_argument When the source sheet index is invalid.
     */
    sheet_copy_result append_sheet_copy(sheet_t src, std::string name);

    /**
     * Create a named view of a sheet.  The view takes a snapshot of the
     * current content of the sheet, and answers reads from that snapshot.
     * The model context owns the view; the returned reference stays valid
     * until the view gets removed or the model context gets destroyed.
     *
     * @param sheet Index of the sheet to create a view of.
     * @param name Name of the view.  It must be unique among the views of the
     *             same sheet.
     *
     * @return Reference to the new view.
     *
     * @throw model_context_error When a view of the same name already exists
     *                            on the sheet.
     * @throw std::invalid_argument When the sheet index is invalid.
     */
    sheet_view& create_sheet_view(sheet_t sheet, std::string name);

    /**
     * Get a named view of a sheet.
     *
     * @param sheet Index of the sheet the view belongs to.
     * @param name Name of the view.
     *
     * @return Pointer to the view, or nullptr if no view of that name exists
     *         on the sheet.
     */
    sheet_view* get_sheet_view(sheet_t sheet, std::string_view name);

    /** @copydoc get_sheet_view(sheet_t, std::string_view) */
    const sheet_view* get_sheet_view(sheet_t sheet, std::string_view name) const;

    /**
     * Remove a named view of a sheet.  It does nothing if no view of that
     * name exists on the sheet.
     *
     * @param sheet Index of the sheet the view belongs to.
     * @param name Name of the view.
     */
    void remove_sheet_view(sheet_t sheet, std::string_view name);

    /**
     * A convenient way to mass-insert a range of cell values.  You can
     * use a nested initializer list representing a range of cell values.  The
     * outer list represents rows.
     *
     * @param sheet sheet index.
     * @param rows nested list of cell values.  The outer list represents
     *             rows.
     */
    void set_cell_values(sheet_t sheet, std::initializer_list<input_row> rows);

    /**
     * Set the factory that creates a session handler for each formula cell
     * interpretation.  Without one, no session handler gets created.
     *
     * @param factory Factory to use.  The model does not take ownership; the
     *                factory must outlive the model, or be replaced before
     *                it gets destroyed.
     */
    void set_session_handler_factory(session_handler_factory* factory);

    /**
     * Insert a new table into the model.  A table is a 2-dimensional range
     * of cells with named columns, referenced by table references in
     * formula expressions.
     *
     * @param tab Table to insert.  It must have a non-empty name unique
     *            within the model, a valid sheet index and a valid range.
     *
     * @throw std::invalid_argument When the name is empty, the sheet index
     *        is invalid, or the range is invalid.
     * @throw model_context_error When a table by the same name already
     *        exists in the model.
     */
    void set_table(table_t tab);

    /**
     * Get the number of strings in the indexed string pool.
     *
     * @return Number of strings in the pool.
     */
    size_t get_string_count() const;

    /**
     * Print the content of the indexed string pool to standard output, for
     * debugging.
     */
    void dump_strings() const;

    /**
     * Dump the content of a sheet to an output stream as a human-readable
     * text grid, primarily for debugging.  The grid spans the data area of
     * the sheet and includes column and row headers.  An empty sheet
     * produces no output at all.
     *
     * @param os Output stream to dump the sheet content to.
     * @param sheet Index of the sheet to dump.
     * @param mode Amount of detail to include in the output.
     * @param resolver Name resolver that determines the column label style
     *                 as well as the way formula expressions get printed in
     *                 verbose mode.  When null, an Excel A1 resolver gets
     *                 created and used internally.
     */
    void dump_sheet(
        std::ostream& os, sheet_t sheet, sheet_dump_mode_t mode,
        const formula_name_resolver* resolver = nullptr) const;

    /**
     * Get an integer string ID from a string value.  If the string value
     * doesn't exist in the pool, the value equal to empty_string_id gets
     * returned.
     *
     * @deprecated Only strings registered via @ref append_string live in
     *             the indexed string pool.  Strings written through
     *             @ref set_string_cell with a `string_view` go to a
     *             separate inline pool and are not visible here.  Read
     *             cell strings via get_string_value instead.
     *
     * @param s string value.
     *
     * @return string_id_t integer string ID associated with the string value
     *         given.
     */
    [[deprecated("indexed-pool lookup only finds strings added via append_string; use get_string_value for cell text")]]
    string_id_t get_identifier_from_string(std::string_view s) const;

    /**
     * Intern a string into the inline string pool and return a stable string
     * view instance valid for the lifetime of the model.
     *
     * @param s String to intern.
     * @return Stable view into pool-owned storage.
     */
    std::string_view intern_string(std::string_view s);

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
    [[deprecated("use iterate_cells()")]]
    model_iterator get_model_iterator(
        sheet_t sheet, rc_direction_t dir, const abs_rc_range_t& range) const;

    /**
     * Get an STL-compliant range over cell values in one sheet.  Supports
     * range-`for` and STL/range algorithms.  <i>The caller has to ensure that
     * the model content does not change for the duration of the iteration.</i>
     *
     * @param sheet Sheet index.
     * @param dir Direction of the iteration.
     * @param range Range on the specified sheet to iterate over.
     *
     * @return Cell range instance.
     */
    model_cell_range iterate_cells(
        sheet_t sheet, rc_direction_t dir, const abs_rc_range_t& range) const;

    /**
     * Get an iterator for global named expressions.
     *
     * @deprecated Use iterate_named_expressions() instead.
     */
    [[deprecated("use iterate_named_expressions()")]]
    named_expressions_iterator get_named_expressions_iterator() const;

    /**
     * Get an interator for sheet-local named expressions.
     *
     * @deprecated Use iterate_named_expressions(sheet_t) instead.
     *
     * @param sheet 0-based index of the sheet where the named expressions are
     *              stored.
     */
    [[deprecated("use iterate_named_expressions()")]]
    named_expressions_iterator get_named_expressions_iterator(sheet_t sheet) const;

    /**
     * Get an STL-compliant range over the global named expressions.
     * Supports range-`for` and STL/range algorithms.  <i>The caller has to
     * ensure that the named expressions do not change for the duration of
     * the iteration.</i>
     *
     * @return Range over the global named expressions.
     */
    named_expressions_range iterate_named_expressions() const;

    /**
     * Get an STL-compliant range over the named expressions local to one
     * sheet.  Supports range-`for` and STL/range algorithms.  <i>The caller
     * has to ensure that the named expressions do not change for the
     * duration of the iteration.</i>
     *
     * @param sheet 0-based index of the sheet where the named expressions are
     *              stored.
     *
     * @return Range over the sheet-local named expressions.
     */
    named_expressions_range iterate_named_expressions(sheet_t sheet) const;

    /**
     * Traverse a range of a sheet one storage block at a time, column by
     * column.  The callback gets called once for each block segment that
     * falls inside the range, with the column, the first and last row of the
     * segment, and the shape of the block.  Traversal stops when the callback
     * returns false.
     *
     * @param sheet Index of the sheet.
     * @param range Range to traverse.
     * @param cb Callback to call for each block segment.
     */
    void walk(
        sheet_t sheet, const abs_rc_range_t& range, column_block_callback_t cb) const;

    /**
     * Check whether the model has any sheets.
     *
     * @return True if the model has no sheets, false otherwise.
     */
    bool empty() const;
};

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
