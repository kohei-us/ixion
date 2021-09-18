/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_MODEL_CONTEXT_HPP
#define INCLUDED_IXION_MODEL_CONTEXT_HPP

#include "env.hpp"
#include "./interface/formula_model_access.hpp"

#include <string>
#include <memory>
#include <variant>

namespace ixion {

struct abs_address_t;
struct abs_rc_range_t;
struct config;
class matrix;
class model_iterator;
class named_expressions_iterator;
class cell_access;

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
class IXION_DLLPUBLIC model_context : public iface::formula_model_access
{
    friend class named_expressions_iterator;
    friend class cell_access;

    std::unique_ptr<detail::model_context_impl> mp_impl;

    formula_result_wait_policy_t get_formula_result_wait_policy() const;

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
    virtual ~model_context() override;

    virtual void notify(formula_event_t event) override;

    virtual const config& get_config() const override;
    virtual dirty_cell_tracker& get_cell_tracker() override;
    virtual const dirty_cell_tracker& get_cell_tracker() const override;

    virtual bool is_empty(const abs_address_t& addr) const override;
    virtual celltype_t get_celltype(const abs_address_t& addr) const override;
    virtual double get_numeric_value(const abs_address_t& addr) const override;
    virtual bool get_boolean_value(const abs_address_t& addr) const override;
    virtual string_id_t get_string_identifier(const abs_address_t& addr) const override;
    virtual const std::string* get_string_value(const abs_address_t& addr) const override;
    virtual const formula_cell* get_formula_cell(const abs_address_t& addr) const override;
    virtual formula_cell* get_formula_cell(const abs_address_t& addr) override;

    virtual formula_result get_formula_result(const abs_address_t& addr) const override;

    virtual const named_expression_t* get_named_expression(sheet_t sheet, std::string_view name) const override;

    virtual double count_range(const abs_range_t& range, const values_t& values_type) const override;
    virtual matrix get_range_value(const abs_range_t& range) const override;
    virtual std::unique_ptr<iface::session_handler> create_session_handler() override;
    virtual iface::table_handler* get_table_handler() override;
    virtual const iface::table_handler* get_table_handler() const override;

    virtual string_id_t add_string(std::string_view s) override;
    virtual const std::string* get_string(string_id_t identifier) const override;
    virtual sheet_t get_sheet_index(std::string_view name) const override;
    virtual std::string get_sheet_name(sheet_t sheet) const override;
    virtual rc_size_t get_sheet_size() const override;
    virtual size_t get_sheet_count() const override;

    string_id_t append_string(std::string_view s);

    void set_sheet_size(const rc_size_t& sheet_size);
    void set_config(const config& cfg);

    IXION_DEPRECATED void erase_cell(const abs_address_t& addr);
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
     * @param p pointer to the string buffer that contains the name of the
     *          expression.
     * @param n length of the buffer containing the name.
     * @param expr formula tokens to use for the named expression.
     */
    void set_named_expression(const char* p, size_t n, formula_tokens_t expr);

    /**
     * Set a named expression associated with a string name in the global
     * scope.
     *
     * @param p pointer to the string buffer that contains the name of the
     *          expression.
     * @param n length of the buffer containing the name.
     * @param origin position of the origin cell.  Origin cell is relevant
     *               only when you need to convert the tokens into a string
     *               representation.
     * @param expr formula tokens to use for the named expression.
     */
    void set_named_expression(const char* p, size_t n, const abs_address_t& origin, formula_tokens_t expr);

    /**
     * Set a named expression associated with a string name in a sheet-local
     * scope.
     *
     * @param sheet 0-based index of the sheet to register this expression
     *              with.
     * @param p pointer to the string buffer that contains the name of the
     *          expression.
     * @param n length of the buffer containing the name.
     * @param expr formula tokens to use for the named expression.
     */
    void set_named_expression(sheet_t sheet, const char* p, size_t n, formula_tokens_t expr);

    /**
     * Set a named expression associated with a string name in a sheet-local
     * scope.
     *
     * @param sheet 0-based index of the sheet to register this expression
     *              with.
     * @param p pointer to the string buffer that contains the name of the
     *          expression.
     * @param n length of the buffer containing the name.
     * @param origin position of the origin cell.  Origin cell is relevant
     *               only when you need to convert the tokens into a string
     *               representation.
     * @param expr formula tokens to use for the named expression.
     */
    void set_named_expression(sheet_t sheet, const char* p, size_t n, const abs_address_t& origin, formula_tokens_t expr);

    /**
     * Append a new sheet to the model.  The caller must ensure that the name
     * of the new sheet is unique within the model context.  When the name
     * being used for the new sheet already exists, it throws a
     * model_context_error exception.
     *
     * @param p pointer to the char array storing the name of the inserted
     *          sheet.
     * @param n size of the sheet name char array.
     *
     * @return sheet index of the inserted sheet.
     *
     * @throw model_context_error
     */
    sheet_t append_sheet(const char* p, size_t n);

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

    string_id_t get_identifier_from_string(const char* p, size_t n) const;

    /**
     * Get an immutable iterator that lets you iterate cell values in one
     * sheet one at a time.  <i>The caller has to ensure that the model
     * content does not change for the duration of the iteration.</i>
     *
     * @param sheet sheet index.
     * @param dir direction of the iteration.  Currently, only horizontal
     *            direction is supported.
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

    /**
     * @deprecated This is not generic enough and should be replaced.  This
     *             functionality should be realized via model_iterator in the
     *             future.
     */
    abs_address_set_t get_all_formula_cells() const;

    bool empty() const;
};

}

#endif
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
