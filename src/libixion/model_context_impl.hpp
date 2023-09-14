/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_MODEL_CONTEXT_IMPL_HPP
#define INCLUDED_MODEL_CONTEXT_IMPL_HPP

#include "ixion/model_context.hpp"
#include "ixion/types.hpp"
#include "ixion/config.hpp"
#include "ixion/dirty_cell_tracker.hpp"

#include "workbook.hpp"
#include "column_store_type.hpp"

#include <vector>
#include <string>
#include <unordered_map>
#include <mutex>
#include <deque>

namespace ixion { namespace detail {

class safe_string_pool
{
    using string_pool_type = std::deque<std::string>;
    using string_map_type = std::unordered_map<std::string_view, string_id_t>;

    std::mutex m_mtx;
    string_pool_type m_strings;
    string_map_type m_string_map;
    std::string m_empty_string;

    string_id_t append_string_unsafe(std::string_view s);

public:
    string_id_t append_string(std::string_view s);
    string_id_t add_string(std::string_view s);
    const std::string* get_string(string_id_t identifier) const;

    size_t size() const;
    void dump_strings() const;
    string_id_t get_identifier_from_string(std::string_view s) const;
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

    formula_result_wait_policy_t get_formula_result_wait_policy() const
    {
        return m_formula_res_wait_policy;
    }

    void notify(formula_event_t event);

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

    void empty_cell(const abs_address_t& addr);
    void set_numeric_cell(const abs_address_t& addr, double val);
    void set_boolean_cell(const abs_address_t& addr, bool val);
    void set_string_cell(const abs_address_t& addr, std::string_view s);
    void set_string_cell(const abs_address_t& addr, string_id_t identifier);
    void fill_down_cells(const abs_address_t& src, size_t n_dst);
    formula_cell* set_formula_cell(const abs_address_t& addr, const formula_tokens_store_ptr_t& tokens);
    formula_cell* set_formula_cell(const abs_address_t& addr, const formula_tokens_store_ptr_t& tokens, formula_result result);
    void set_grouped_formula_cells(const abs_range_t& group_range, formula_tokens_t tokens);
    void set_grouped_formula_cells(const abs_range_t& group_range, formula_tokens_t tokens, formula_result result);

    abs_range_t get_data_range(sheet_t sheet) const;

    bool is_empty(const abs_address_t& addr) const;
    bool is_empty(abs_range_t range) const;
    celltype_t get_celltype(const abs_address_t& addr) const;
    cell_value_t get_cell_value_type(const abs_address_t& addr) const;
    double get_numeric_value(const abs_address_t& addr) const;
    bool get_boolean_value(const abs_address_t& addr) const;
    string_id_t get_string_identifier(const abs_address_t& addr) const;
    std::string_view get_string_value(const abs_address_t& addr) const;
    string_id_t get_identifier_from_string(std::string_view s) const;
    const formula_cell* get_formula_cell(const abs_address_t& addr) const;
    formula_cell* get_formula_cell(const abs_address_t& addr);

    formula_result get_formula_result(const abs_address_t& addr) const;

    void set_named_expression(std::string name, const abs_address_t& origin, formula_tokens_t&& expr);
    void set_named_expression(sheet_t sheet, std::string name, const abs_address_t& origin, formula_tokens_t&& expr);

    const named_expression_t* get_named_expression(std::string_view name) const;
    const named_expression_t* get_named_expression(sheet_t sheet, std::string_view name) const;

    sheet_t get_sheet_index(std::string_view name) const;
    std::string get_sheet_name(sheet_t sheet) const;
    void set_sheet_name(sheet_t sheet, std::string name);
    rc_size_t get_sheet_size() const;
    size_t get_sheet_count() const;
    sheet_t append_sheet(std::string&& name);

    void set_cell_values(sheet_t sheet, std::initializer_list<model_context::input_row>&& rows);

    string_id_t append_string(std::string_view s);
    string_id_t add_string(std::string_view s);
    const std::string* get_string(string_id_t identifier) const;
    size_t get_string_count() const;
    void dump_strings() const;

    const column_store_t* get_column(sheet_t sheet, col_t col) const;
    const column_stores_t* get_columns(sheet_t sheet) const;

    double count_range(abs_range_t range, values_t values_type) const;

    void walk(sheet_t sheet, const abs_rc_range_t& range, column_block_callback_t cb) const;

    bool empty() const;

    const worksheet* fetch_sheet(sheet_t sheet_index) const;

    column_store_t::const_position_type get_cell_position(const abs_address_t& addr) const;

    const detail::named_expressions_t& get_named_expressions() const;
    const detail::named_expressions_t& get_named_expressions(sheet_t sheet) const;

    model_iterator get_model_iterator(
        sheet_t sheet, rc_direction_t dir, const abs_rc_range_t& range) const;

private:
    abs_range_t shrink_to_workbook(abs_range_t range) const;

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

    formula_result_wait_policy_t m_formula_res_wait_policy;
};

}}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
