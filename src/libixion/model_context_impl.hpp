/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_MODEL_CONTEXT_IMPL_HPP
#define INCLUDED_MODEL_CONTEXT_IMPL_HPP

#include "ixion/model_context.hpp"
#include "ixion/mem_str_buf.hpp"
#include "ixion/types.hpp"
#include "ixion/config.hpp"
#include "ixion/column_store_type.hpp"
#include "ixion/dirty_cell_tracker.hpp"

#include "workbook.hpp"

#include <vector>
#include <string>
#include <unordered_map>
#include <mutex>

namespace ixion { namespace detail {

class safe_string_pool
{
    using string_pool_type = std::vector<std::unique_ptr<std::string>>;
    using string_map_type = std::unordered_map<mem_str_buf, string_id_t, mem_str_buf::hash>;

    std::mutex m_mtx;
    string_pool_type m_strings;
    string_map_type m_string_map;
    std::string m_empty_string;

    string_id_t append_string_unsafe(const char* p, size_t n);

public:
    string_id_t append_string(const char* p, size_t n);
    string_id_t add_string(const char* p, size_t n);
    const std::string* get_string(string_id_t identifier) const;

    size_t size() const;
    void dump_strings() const;
    string_id_t get_identifier_from_string(const char* p, size_t n) const;
};

class model_context_impl
{
    typedef std::vector<std::string> strings_type;

public:
    model_context_impl() = delete;
    model_context_impl(const model_context_impl&) = delete;
    model_context_impl& operator= (model_context_impl) = delete;

    model_context_impl(model_context& parent, const rc_size_t& sheet_size);
    ~model_context_impl();

    const config& get_config() const
    {
        return m_config;
    }

    void set_config(const config& cfg)
    {
        m_config = cfg;
    }

    void set_sheet_size(const rc_size_t& sheet_size);

    dirty_cell_tracker& get_cell_tracker()
    {
        return m_tracker;
    }

    const dirty_cell_tracker& get_cell_tracker() const
    {
        return m_tracker;
    }

    std::unique_ptr<iface::session_handler> create_session_handler();

    void set_session_handler_factory(model_context::session_handler_factory* factory)
    {
        mp_session_factory = factory;
    }

    iface::table_handler* get_table_handler()
    {
        return mp_table_handler;
    }

    const iface::table_handler* get_table_handler() const
    {
        return mp_table_handler;
    }

    void set_table_handler(iface::table_handler* handler)
    {
        mp_table_handler = handler;
    }

    void erase_cell(const abs_address_t& addr);
    void set_numeric_cell(const abs_address_t& addr, double val);
    void set_boolean_cell(const abs_address_t& addr, bool val);
    void set_string_cell(const abs_address_t& addr, const char* p, size_t n);
    void set_string_cell(const abs_address_t& addr, string_id_t identifier);
    void fill_down_cells(const abs_address_t& src, size_t n_dst);
    formula_cell* set_formula_cell(const abs_address_t& addr, const formula_tokens_store_ptr_t& tokens);
    formula_cell* set_formula_cell(const abs_address_t& addr, const formula_tokens_store_ptr_t& tokens, formula_result result);
    void set_grouped_formula_cells(const abs_range_t& group_range, formula_tokens_t tokens);
    void set_grouped_formula_cells(const abs_range_t& group_range, formula_tokens_t tokens, formula_result result);

    abs_range_t get_data_range(sheet_t sheet) const;

    bool is_empty(const abs_address_t& addr) const;
    celltype_t get_celltype(const abs_address_t& addr) const;
    double get_numeric_value(const abs_address_t& addr) const;
    double get_numeric_value_nowait(const abs_address_t& addr) const;
    bool get_boolean_value(const abs_address_t& addr) const;
    string_id_t get_string_identifier(const abs_address_t& addr) const;
    const std::string* get_string_value(const abs_address_t& addr) const;
    string_id_t get_string_identifier_nowait(const abs_address_t& addr) const;
    string_id_t get_identifier_from_string(const char* p, size_t n) const;
    const formula_cell* get_formula_cell(const abs_address_t& addr) const;
    formula_cell* get_formula_cell(const abs_address_t& addr);

    void set_named_expression(const char* p, size_t n, const abs_address_t& origin, formula_tokens_t&& expr);
    void set_named_expression(sheet_t sheet, const char* p, size_t n, const abs_address_t& origin, formula_tokens_t&& expr);

    const named_expression_t* get_named_expression(const std::string& name) const;
    const named_expression_t* get_named_expression(sheet_t sheet, const std::string& name) const;

    sheet_t get_sheet_index(const char* p, size_t n) const;
    std::string get_sheet_name(sheet_t sheet) const;
    rc_size_t get_sheet_size() const;
    size_t get_sheet_count() const;
    sheet_t append_sheet(std::string&& name);

    void set_cell_values(sheet_t sheet, std::initializer_list<model_context::input_row>&& rows);

    string_id_t append_string(const char* p, size_t n);
    string_id_t add_string(const char* p, size_t n);
    const std::string* get_string(string_id_t identifier) const;
    size_t get_string_count() const;
    void dump_strings() const;

    const column_store_t* get_column(sheet_t sheet, col_t col) const;
    const column_stores_t* get_columns(sheet_t sheet) const;

    double count_range(const abs_range_t& range, const values_t& values_type) const;

    abs_address_set_t get_all_formula_cells() const;

    bool empty() const;

    const worksheet* fetch_sheet(sheet_t sheet_index) const;

    const detail::named_expressions_t& get_named_expressions() const;
    const detail::named_expressions_t& get_named_expressions(sheet_t sheet) const;

private:
    model_context& m_parent;

    rc_size_t m_sheet_size;
    workbook m_sheets;

    config m_config;
    dirty_cell_tracker m_tracker;
    iface::table_handler* mp_table_handler;
    detail::named_expressions_t m_named_expressions;

    model_context::session_handler_factory* mp_session_factory;

    strings_type m_sheet_names; ///< index to sheet name map.

    safe_string_pool m_str_pool;
};

}}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
