/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __IXION_MODEL_CONTEXT_HPP__
#define __IXION_MODEL_CONTEXT_HPP__

#include "ixion/column_store_type.hpp"
#include "ixion/mem_str_buf.hpp"
#include "ixion/interface/formula_model_access.hpp"
#include "ixion/env.hpp"

#include <string>
#include <deque>
#include <memory>

namespace ixion {

struct abs_address_t;
struct config;
class matrix;
class model_context_impl;

/**
 * This class stores all data relevant to current session.  You can think of
 * this like a document model for each formula calculation run.  Note that
 * only those methods called from the formula interpreter are specified in
 * the interface; this explains why accessors for the most part only have
 * the 'get' method not paired with its 'set' counterpart.
 */
class IXION_DLLPUBLIC model_context : public iface::formula_model_access
{
public:
    struct shared_tokens
    {
        formula_tokens_t* tokens;
        abs_range_t range;

        shared_tokens();
        shared_tokens(formula_tokens_t* tokens);
        shared_tokens(const shared_tokens& r);

        bool operator== (const shared_tokens& r) const;
    };
    typedef std::vector<shared_tokens> shared_tokens_type;

    model_context();
    virtual ~model_context();

    virtual const config& get_config() const;
    virtual cell_listener_tracker& get_cell_listener_tracker();

    virtual bool is_empty(const abs_address_t& addr) const;
    virtual celltype_t get_celltype(const abs_address_t& addr) const;
    virtual double get_numeric_value(const abs_address_t& addr) const;
    virtual string_id_t get_string_identifier(const abs_address_t& addr) const;
    virtual string_id_t get_string_identifier(const char* p, size_t n) const;
    virtual const formula_cell* get_formula_cell(const abs_address_t& addr) const;
    virtual formula_cell* get_formula_cell(const abs_address_t& addr);

    virtual const formula_cell* get_named_expression(const ::std::string& name) const;
    virtual const ::std::string* get_named_expression_name(const formula_cell* expr) const;
    virtual double count_range(const abs_range_t& range, const values_t& values_type) const;
    virtual matrix get_range_value(const abs_range_t& range) const;
    virtual iface::session_handler* get_session_handler();
    virtual iface::table_handler* get_table_handler();
    virtual const iface::table_handler* get_table_handler() const;
    virtual const formula_tokens_t* get_formula_tokens(sheet_t sheet, size_t identifier) const;
    virtual const formula_tokens_t* get_shared_formula_tokens(sheet_t sheet, size_t identifier) const;
    virtual abs_range_t get_shared_formula_range(sheet_t sheet, size_t identifier) const;

    virtual string_id_t append_string(const char* p, size_t n);
    virtual string_id_t add_string(const char* p, size_t n);
    virtual const std::string* get_string(string_id_t identifier) const;
    virtual sheet_t get_sheet_index(const char* p, size_t n) const;
    virtual std::string get_sheet_name(sheet_t sheet) const;

    double get_numeric_value_nowait(const abs_address_t& addr) const;
    string_id_t get_string_identifier_nowait(const abs_address_t& addr) const;

    size_t add_formula_tokens(sheet_t sheet, formula_tokens_t* p);
    void set_shared_formula_range(sheet_t sheet, size_t identifier, const abs_range_t& range);
    size_t set_formula_tokens_shared(sheet_t sheet, size_t identifier);
    void remove_formula_tokens(sheet_t sheet, size_t identifier);

    void set_shared_formula(
        const abs_address_t& addr, size_t si,
        const char* p_formula, size_t n_formula, const char* p_range, size_t n_range,
        const formula_name_resolver& resolver);
    void set_shared_formula(
        const abs_address_t& addr, size_t si,
        const char* p_formula, size_t n_formula, const formula_name_resolver& resolver);

    void erase_cell(const abs_address_t& addr);

    void set_numeric_cell(const abs_address_t& addr, double val);
    void set_boolean_cell(const abs_address_t& adr, bool val);
    void set_string_cell(const abs_address_t& addr, const char* p, size_t n);
    void set_string_cell(const abs_address_t& addr, string_id_t identifier);
    void set_formula_cell(const abs_address_t& addr, const char* p, size_t n, const formula_name_resolver& resolver);
    void set_formula_cell(const abs_address_t& addr, size_t identifier, bool shared);

    abs_range_t get_data_range(sheet_t sheet) const;

    void set_named_expression(const char* p, size_t n, formula_cell* cell);
    formula_cell* get_named_expression(const ::std::string& name);

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
     * Set new session handler instance.  The client code needs to allocate
     * the new instance and pass it to the model context; the model context
     * will in turn manage the life cycle of the passed instance. Passing NULL
     * here will only delete the existing session handler, which disables
     * session handling altogether.
     *
     * @param handler pointer to the new session handler instance.
     */
    void set_session_handler(iface::session_handler* handler);

    void set_table_handler(iface::table_handler* handler);

    size_t get_string_count() const;

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

    void get_all_formula_cells(dirty_formula_cells_t& cells) const;

private:
    model_context_impl* mp_impl;
};

}

#endif
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
