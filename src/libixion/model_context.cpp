/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/model_context.hpp"
#include "ixion/formula_result.hpp"
#include "ixion/matrix.hpp"
#include "ixion/model_iterator.hpp"
#include "ixion/interface/session_handler.hpp"
#include "ixion/named_expressions_iterator.hpp"
#include "ixion/cell_access.hpp"

#include "model_context_impl.hpp"

namespace ixion {

model_context::input_cell::input_cell(std::nullptr_t) : type(celltype_t::empty) {}
model_context::input_cell::input_cell(bool b) : type(celltype_t::boolean)
{
    value = b;
}

model_context::input_cell::input_cell(const char* s) : type(celltype_t::string)
{
    value = std::string_view(s);
}

model_context::input_cell::input_cell(double v) : type(celltype_t::numeric)
{
    value = v;
}

model_context::input_cell::input_cell(const input_cell& other) :
    type(other.type), value(other.value)
{
}

model_context::input_row::input_row(std::initializer_list<input_cell> cells) :
    m_cells(std::move(cells)) { }

const std::initializer_list<model_context::input_cell>& model_context::input_row::cells() const
{
    return m_cells;
}

std::unique_ptr<iface::session_handler> model_context::session_handler_factory::create()
{
    return std::unique_ptr<iface::session_handler>();
}

model_context::session_handler_factory::~session_handler_factory() {}

model_context::model_context() :
    mp_impl(new detail::model_context_impl(*this, {1048576, 16384})) {}

model_context::model_context(const rc_size_t& sheet_size) :
    mp_impl(new detail::model_context_impl(*this, sheet_size)) {}

model_context::~model_context()
{
}

formula_result_wait_policy_t model_context::get_formula_result_wait_policy() const
{
    return mp_impl->get_formula_result_wait_policy();
}

void model_context::notify(formula_event_t event)
{
    mp_impl->notify(event);
}

const config& model_context::get_config() const
{
    return mp_impl->get_config();
}

dirty_cell_tracker& model_context::get_cell_tracker()
{
    return mp_impl->get_cell_tracker();
}

const dirty_cell_tracker& model_context::get_cell_tracker() const
{
    return mp_impl->get_cell_tracker();
}

void model_context::empty_cell(const abs_address_t& addr)
{
    mp_impl->empty_cell(addr);
}

void model_context::set_numeric_cell(const abs_address_t& addr, double val)
{
    mp_impl->set_numeric_cell(addr, val);
}

void model_context::set_boolean_cell(const abs_address_t& addr, bool val)
{
    mp_impl->set_boolean_cell(addr, val);
}

void model_context::set_string_cell(const abs_address_t& addr, std::string_view s)
{
    mp_impl->set_string_cell(addr, s);
}

cell_access model_context::get_cell_access(const abs_address_t& addr) const
{
    return cell_access(*this, addr);
}

void model_context::fill_down_cells(const abs_address_t& src, size_t n_dst)
{
    mp_impl->fill_down_cells(src, n_dst);
}

void model_context::set_string_cell(const abs_address_t& addr, string_id_t identifier)
{
    mp_impl->set_string_cell(addr, identifier);
}

formula_cell* model_context::set_formula_cell(const abs_address_t& addr, formula_tokens_t tokens)
{
    formula_tokens_store_ptr_t ts = formula_tokens_store::create();
    ts->get() = std::move(tokens);

    return mp_impl->set_formula_cell(addr, ts);
}

formula_cell*  model_context::set_formula_cell(
    const abs_address_t& addr, const formula_tokens_store_ptr_t& tokens)
{
    return mp_impl->set_formula_cell(addr, tokens);
}

formula_cell*  model_context::set_formula_cell(
    const abs_address_t& addr, const formula_tokens_store_ptr_t& tokens, formula_result result)
{
    return mp_impl->set_formula_cell(addr, tokens, std::move(result));
}

void model_context::set_grouped_formula_cells(
    const abs_range_t& group_range, formula_tokens_t tokens)
{
    mp_impl->set_grouped_formula_cells(group_range, std::move(tokens));
}

void model_context::set_grouped_formula_cells(
    const abs_range_t& group_range, formula_tokens_t tokens, formula_result result)
{
    mp_impl->set_grouped_formula_cells(group_range, std::move(tokens), std::move(result));
}

abs_range_t model_context::get_data_range(sheet_t sheet) const
{
    return mp_impl->get_data_range(sheet);
}

bool model_context::is_empty(const abs_address_t& addr) const
{
    return mp_impl->is_empty(addr);
}

celltype_t model_context::get_celltype(const abs_address_t& addr) const
{
    return mp_impl->get_celltype(addr);
}

double model_context::get_numeric_value(const abs_address_t& addr) const
{
    return mp_impl->get_numeric_value(addr);
}

bool model_context::get_boolean_value(const abs_address_t& addr) const
{
    return mp_impl->get_boolean_value(addr);
}

void model_context::set_sheet_size(const rc_size_t& sheet_size)
{
    mp_impl->set_sheet_size(sheet_size);
}

void model_context::set_config(const config& cfg)
{
    mp_impl->set_config(cfg);
}

string_id_t model_context::get_string_identifier(const abs_address_t& addr) const
{
    return mp_impl->get_string_identifier(addr);
}

const std::string* model_context::get_string_value(const abs_address_t& addr) const
{
    return mp_impl->get_string_value(addr);
}

const formula_cell* model_context::get_formula_cell(const abs_address_t& addr) const
{
    return mp_impl->get_formula_cell(addr);
}

formula_cell* model_context::get_formula_cell(const abs_address_t& addr)
{
    return mp_impl->get_formula_cell(addr);
}

formula_result model_context::get_formula_result(const abs_address_t& addr) const
{
    return mp_impl->get_formula_result(addr);
}

double model_context::count_range(const abs_range_t& range, const values_t& values_type) const
{
    return mp_impl->count_range(range, values_type);
}

matrix model_context::get_range_value(const abs_range_t& range) const
{
    if (range.first.sheet != range.last.sheet)
        throw general_error("multi-sheet range is not allowed.");

    if (!range.valid())
    {
        std::ostringstream os;
        os << "invalid range: " << range;
        throw std::invalid_argument(os.str());
    }

    rc_size_t sheet_size = get_sheet_size();
    abs_range_t range_clipped = range;
    if (range_clipped.all_rows())
    {
        range_clipped.first.row = 0;
        range_clipped.last.row = sheet_size.row - 1;
    }
    if (range_clipped.all_columns())
    {
        range_clipped.first.column = 0;
        range_clipped.last.column = sheet_size.column - 1;
    }

    row_t rows = range_clipped.last.row - range_clipped.first.row + 1;
    col_t cols = range_clipped.last.column - range_clipped.first.column + 1;

    matrix ret(rows, cols);
    for (row_t i = 0; i < rows; ++i)
    {
        for (col_t j = 0; j < cols; ++j)
        {
            row_t row = i + range_clipped.first.row;
            col_t col = j + range_clipped.first.column;
            double val = get_numeric_value(abs_address_t(range_clipped.first.sheet, row, col));

            // TODO: we need to handle string types when that becomes available.
            ret.set(i, j, val);
        }
    }
    return ret;
}

std::unique_ptr<iface::session_handler> model_context::create_session_handler()
{
    return mp_impl->create_session_handler();
}

iface::table_handler* model_context::get_table_handler()
{
    return mp_impl->get_table_handler();
}

const iface::table_handler* model_context::get_table_handler() const
{
    return mp_impl->get_table_handler();
}

string_id_t model_context::append_string(std::string_view s)
{
    return mp_impl->append_string(s);
}

string_id_t model_context::add_string(std::string_view s)
{
    return mp_impl->add_string(s);
}

const std::string* model_context::get_string(string_id_t identifier) const
{
    return mp_impl->get_string(identifier);
}

sheet_t model_context::get_sheet_index(std::string_view name) const
{
    return mp_impl->get_sheet_index(name);
}

std::string model_context::get_sheet_name(sheet_t sheet) const
{
    return mp_impl->get_sheet_name(sheet);
}

rc_size_t model_context::get_sheet_size() const
{
    return mp_impl->get_sheet_size();
}

size_t model_context::get_sheet_count() const
{
    return mp_impl->get_sheet_count();
}

void model_context::set_named_expression(std::string name, formula_tokens_t expr)
{
    abs_address_t origin(0, 0, 0);
    mp_impl->set_named_expression(std::move(name), origin, std::move(expr));
}

void model_context::set_named_expression(std::string name, const abs_address_t& origin, formula_tokens_t expr)
{
    mp_impl->set_named_expression(std::move(name), origin, std::move(expr));
}

void model_context::set_named_expression(sheet_t sheet, std::string name, formula_tokens_t expr)
{
    abs_address_t origin(0, 0, 0);
    mp_impl->set_named_expression(sheet, std::move(name), origin, std::move(expr));
}

void model_context::set_named_expression(
    sheet_t sheet, std::string name, const abs_address_t& origin, formula_tokens_t expr)
{
    mp_impl->set_named_expression(sheet, std::move(name), origin, std::move(expr));
}

const named_expression_t* model_context::get_named_expression(sheet_t sheet, std::string_view name) const
{
    return mp_impl->get_named_expression(sheet, name);
}

sheet_t model_context::append_sheet(std::string name)
{
    return mp_impl->append_sheet(std::move(name));
}

void model_context::set_cell_values(sheet_t sheet, std::initializer_list<input_row> rows)
{
    mp_impl->set_cell_values(sheet, std::move(rows));
}

void model_context::set_session_handler_factory(session_handler_factory* factory)
{
    mp_impl->set_session_handler_factory(factory);
}

void model_context::set_table_handler(iface::table_handler* handler)
{
    mp_impl->set_table_handler(handler);
}

size_t model_context::get_string_count() const
{
    return mp_impl->get_string_count();
}

void model_context::dump_strings() const
{
    mp_impl->dump_strings();
}

string_id_t model_context::get_identifier_from_string(std::string_view s) const
{
    return mp_impl->get_identifier_from_string(s);
}

model_iterator model_context::get_model_iterator(
    sheet_t sheet, rc_direction_t dir, const abs_rc_range_t& range) const
{
    return mp_impl->get_model_iterator(sheet, dir, range);
}

named_expressions_iterator model_context::get_named_expressions_iterator() const
{
    return named_expressions_iterator(*this, -1);
}

named_expressions_iterator model_context::get_named_expressions_iterator(sheet_t sheet) const
{
    return named_expressions_iterator(*this, sheet);
}

bool model_context::empty() const
{
    return mp_impl->empty();
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
