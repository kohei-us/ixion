/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_MODEL_CONTEXT_HPP
#define INCLUDED_IXION_MODEL_CONTEXT_HPP

#include "ixion/column_store_type.hpp"
#include "ixion/mem_str_buf.hpp"
#include "ixion/interface/formula_model_access.hpp"
#include "ixion/env.hpp"

#include <string>
#include <memory>

namespace ixion {

struct abs_address_t;
struct abs_rc_range_t;
struct config;
class matrix;
class model_context_impl;
class model_iterator;

/**
 * This class stores all data relevant to current session.  You can think of
 * this like a document model for each formula calculation run.  Note that
 * only those methods called from the formula interpreter are specified in
 * the interface; this explains why accessors for the most part only have
 * the 'get' method not paired with its 'set' counterpart.
 */
class IXION_DLLPUBLIC model_context : public iface::formula_model_access
{
    std::unique_ptr<model_context_impl> mp_impl;

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
        celltype_t type;

        union
        {
            bool boolean;
            double numeric;
            const char* string;

        } value;

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
    virtual ~model_context() override;

    virtual const config& get_config() const override;
    virtual dirty_cell_tracker& get_cell_tracker() override;
    virtual const dirty_cell_tracker& get_cell_tracker() const override;

    virtual bool is_empty(const abs_address_t& addr) const override;
    virtual celltype_t get_celltype(const abs_address_t& addr) const override;
    virtual double get_numeric_value(const abs_address_t& addr) const override;
    virtual bool get_boolean_value(const abs_address_t& addr) const override;
    virtual string_id_t get_string_identifier(const abs_address_t& addr) const override;
    virtual string_id_t get_string_identifier(const char* p, size_t n) const override;
    virtual const formula_cell* get_formula_cell(const abs_address_t& addr) const override;
    virtual formula_cell* get_formula_cell(const abs_address_t& addr) override;

    virtual const formula_tokens_t* get_named_expression(sheet_t sheet, const std::string& name) const override;

    virtual double count_range(const abs_range_t& range, const values_t& values_type) const override;
    virtual matrix get_range_value(const abs_range_t& range) const override;
    virtual std::unique_ptr<iface::session_handler> create_session_handler() override;
    virtual iface::table_handler* get_table_handler() override;
    virtual const iface::table_handler* get_table_handler() const override;

    virtual string_id_t append_string(const char* p, size_t n) override;
    virtual string_id_t add_string(const char* p, size_t n) override;
    virtual const std::string* get_string(string_id_t identifier) const override;
    virtual sheet_t get_sheet_index(const char* p, size_t n) const override;
    virtual std::string get_sheet_name(sheet_t sheet) const override;
    virtual rc_size_t get_sheet_size(sheet_t sheet) const override;
    virtual size_t get_sheet_count() const override;

    void set_config(const config& cfg);

    double get_numeric_value_nowait(const abs_address_t& addr) const;
    string_id_t get_string_identifier_nowait(const abs_address_t& addr) const;

    void erase_cell(const abs_address_t& addr);

    void set_numeric_cell(const abs_address_t& addr, double val);
    void set_boolean_cell(const abs_address_t& adr, bool val);
    void set_string_cell(const abs_address_t& addr, const char* p, size_t n);
    void set_string_cell(const abs_address_t& addr, string_id_t identifier);

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
     */
    void set_formula_cell(const abs_address_t& addr, formula_tokens_t tokens);

    /**
     * Set a formula cell at a specified address.  This variant takes a
     * formula tokens store that can be shared between multiple formula cell
     * instances.
     *
     * @param addr address at which to set a formula cell.
     * @param tokens formula tokens to put into the formula cell.
     */
    void set_formula_cell(const abs_address_t& addr, const formula_tokens_store_ptr_t& tokens);

    /**
     * Set a formula cell at a specified address.  This variant takes a
     * formula tokens store that can be shared between multiple formula cell
     * instances.
     *
     * @param addr address at which to set a formula cell.
     * @param tokens formula tokens to put into the formula cell.
     * @param result cached result of this formula cell.
     */
    void set_formula_cell(const abs_address_t& addr, const formula_tokens_store_ptr_t& tokens, formula_result result);

    void set_grouped_formula_cells(const abs_range_t& group_range, formula_tokens_t tokens);

    void set_grouped_formula_cells(const abs_range_t& group_range, formula_tokens_t tokens, formula_result result);

    abs_range_t get_data_range(sheet_t sheet) const;

    void set_named_expression(const char* p, size_t n, std::unique_ptr<formula_tokens_t>&& expr);
    void set_named_expression(sheet_t sheet, const char* p, size_t n, std::unique_ptr<formula_tokens_t>&& expr);

    /**
     * Append a new sheet to the model.  The caller must ensure that the name
     * of the new sheet is unique within the model context.  When the name
     * being used for the new sheet already exists, it throws a {@link
     * model_context_error} exception.
     *
     * @param p pointer to the char array storing the name of the inserted
     *          sheet.
     * @param n size of the sheet name char array.
     * @param row_size number of rows in the inserted sheet.
     * @param col_size number of columns in the inserted sheet.
     *
     * @return sheet index of the inserted sheet.
     */
    sheet_t append_sheet(const char* p, size_t n, row_t row_size, col_t col_size);

    /**
     * Append a new sheet to the model.  The caller must ensure that the name
     * of the new sheet is unique within the model context.  When the name
     * being used for the new sheet already exists, it throws a {@link
     * model_context_error} exception.
     *
     * @param name name of the sheet to be inserted.
     * @param row_size number of rows in the inserted sheet.
     * @param col_size number of columns in the inserted sheet.
     *
     * @return sheet index of the inserted sheet.
     */
    sheet_t append_sheet(std::string name, row_t row_size, col_t col_size);

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
     * Get column storage.
     *
     * @param sheet sheet index.
     * @param col column index.
     *
     * @return const pointer to column storage, or NULL in case sheet index or
     *         column index is out of bound.
     */
    const column_store_t* get_column(sheet_t sheet, col_t col) const;

    /**
     * Get an array of column stores for the entire sheet.
     *
     *
     * @param sheet sheet index.
     *
     * @return const pointer to an array of column stores, or nullptr in case
     * the sheet index is out of bound.
     */
    const column_stores_t* get_columns(sheet_t sheet) const;

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
