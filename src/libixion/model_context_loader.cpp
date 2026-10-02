/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <ixion/model_context_loader.hpp>
#include <ixion/model_context.hpp>
#include <ixion/cell.hpp>
#include <ixion/formula_result.hpp>
#include <ixion/formula.hpp>
#include <ixion/exceptions.hpp>
#include <ixion/formula_tokens.hpp>
#include <ixion/table.hpp>

#include "model_context_impl.hpp"
#include "formula_registration.hpp"
#include "calc_status.hpp"

#include <cassert>
#include <utility>
#include <vector>

namespace ixion {

struct model_context_loader::impl
{
    model_context& cxt;

    /** Top-left positions of the formula cells, which wait to be registered. */
    std::vector<abs_address_t> formula_cells_to_register;

    bool finalized = false;

    impl(model_context& _cxt) : cxt(_cxt) {}

    void ensure_not_finalized() const
    {
        if (finalized)
            throw model_context_error(
                "the loader has already been finalized",
                model_context_error::loader_already_finalized);
    }

    /** Model being loaded; off limits once finalize() has run. */
    const model_context& get_model() const
    {
        ensure_not_finalized();
        return cxt;
    }

    model_context& get_model()
    {
        ensure_not_finalized();
        return cxt;
    }

    detail::model_context_impl& get_model_impl()
    {
        return *get_model().mp_impl;
    }
};

model_context_loader::model_context_loader(model_context& cxt) :
    mp_impl(std::make_unique<impl>(cxt))
{
}

model_context_loader::~model_context_loader() = default;

sheet_t model_context_loader::append_sheet(std::string name)
{
    return mp_impl->get_model().append_sheet(std::move(name));
}

void model_context_loader::set_named_expression(std::string name, formula_tokens_t expr)
{
    mp_impl->get_model().set_named_expression(std::move(name), std::move(expr));
}

void model_context_loader::set_named_expression(
    std::string name, const abs_address_t& origin, formula_tokens_t expr)
{
    mp_impl->get_model().set_named_expression(std::move(name), origin, std::move(expr));
}

void model_context_loader::set_named_expression(
    sheet_t sheet, std::string name, formula_tokens_t expr)
{
    mp_impl->get_model().set_named_expression(sheet, std::move(name), std::move(expr));
}

void model_context_loader::set_named_expression(
    sheet_t sheet, std::string name, const abs_address_t& origin, formula_tokens_t expr)
{
    mp_impl->get_model().set_named_expression(sheet, std::move(name), origin, std::move(expr));
}

void model_context_loader::set_table(table_t tab)
{
    mp_impl->get_model().set_table(std::move(tab));
}

string_id_t model_context_loader::append_string(std::string_view s)
{
    return mp_impl->get_model().append_string(s);
}

string_id_t model_context_loader::add_string(std::string_view s)
{
    return mp_impl->get_model().add_string(s);
}

void model_context_loader::set_numeric_cell(const abs_address_t& addr, double val)
{
    detail::model_context_impl& cxt = mp_impl->get_model_impl();
    mdds::mtv::position_hint hint = cxt.ensure_empty_or_throw(addr);
    cxt.write_numeric_cell(hint, addr, val);
}

void model_context_loader::set_boolean_cell(const abs_address_t& addr, bool val)
{
    detail::model_context_impl& cxt = mp_impl->get_model_impl();
    mdds::mtv::position_hint hint = cxt.ensure_empty_or_throw(addr);
    cxt.write_boolean_cell(hint, addr, val);
}

void model_context_loader::set_string_cell(const abs_address_t& addr, std::string_view s)
{
    detail::model_context_impl& cxt = mp_impl->get_model_impl();
    mdds::mtv::position_hint hint = cxt.ensure_empty_or_throw(addr);
    cxt.write_string_cell(hint, addr, s);
}

void model_context_loader::set_string_cell(const abs_address_t& addr, string_id_t identifier)
{
    detail::model_context_impl& cxt = mp_impl->get_model_impl();
    mdds::mtv::position_hint hint = cxt.ensure_empty_or_throw(addr);
    cxt.write_string_cell(hint, addr, identifier);
}

void model_context_loader::fill_down_cells(const abs_address_t& src, std::size_t n_dst)
{
    detail::model_context_impl& cxt = mp_impl->get_model_impl();
    cxt.ensure_empty_or_throw(abs_range_t(src.sheet, src.row + 1, src.column, row_t(n_dst), 1));
    cxt.write_fill_down_cells(src, n_dst);
}

void model_context_loader::set_cell_values(
    sheet_t sheet, std::initializer_list<model_context::input_row> rows)
{
    abs_address_t pos(sheet, 0, 0);

    for (const model_context::input_row& row : rows)
    {
        pos.column = 0;

        for (const model_context::input_cell& c : row.cells())
        {
            switch (c.type)
            {
                case cell_t::numeric:
                    set_numeric_cell(pos, std::get<double>(c.value));
                    break;
                case cell_t::string:
                    set_string_cell(pos, std::get<std::string_view>(c.value));
                    break;
                case cell_t::boolean:
                    set_boolean_cell(pos, std::get<bool>(c.value));
                    break;
                default:
                    ;
            }

            ++pos.column;
        }

        ++pos.row;
    }
}

formula_cell* model_context_loader::set_formula_cell(
    const abs_address_t& addr, formula_tokens_t tokens)
{
    formula_tokens_store_ptr_t ts = formula_tokens_store::create(std::move(tokens));
    return set_formula_cell(addr, ts);
}

formula_cell* model_context_loader::set_formula_cell(
    const abs_address_t& addr, const formula_tokens_store_ptr_t& tokens)
{
    detail::model_context_impl& cxt = mp_impl->get_model_impl();
    mdds::mtv::position_hint hint = cxt.ensure_empty_or_throw(addr);
    formula_cell* p = cxt.write_formula_cell(hint, addr, std::make_unique<formula_cell>(tokens));
    mp_impl->formula_cells_to_register.push_back(addr);
    return p;
}

formula_cell* model_context_loader::set_formula_cell(
    const abs_address_t& addr, const formula_tokens_store_ptr_t& tokens, formula_result result)
{
    detail::model_context_impl& cxt = mp_impl->get_model_impl();
    std::unique_ptr<formula_cell> fcell = std::make_unique<formula_cell>(tokens);
    fcell->set_result_cache(std::move(result));
    mdds::mtv::position_hint hint = cxt.ensure_empty_or_throw(addr);
    formula_cell* p = cxt.write_formula_cell(hint, addr, std::move(fcell));
    mp_impl->formula_cells_to_register.push_back(addr);
    return p;
}

void model_context_loader::set_grouped_formula_cells(
    const abs_range_t& group_range, formula_tokens_t tokens)
{
    detail::model_context_impl& cxt = mp_impl->get_model_impl();
    formula_tokens_store_ptr_t ts = formula_tokens_store::create(std::move(tokens));
    calc_status_ptr_t cs = detail::model_context_impl::create_group_status(group_range);
    cxt.ensure_empty_or_throw(group_range);
    cxt.write_formula_group(group_range, cs, ts);
    mp_impl->formula_cells_to_register.push_back(group_range.first);
}

void model_context_loader::set_grouped_formula_cells(
    const abs_range_t& group_range, formula_tokens_t tokens, formula_result result)
{
    detail::model_context_impl& cxt = mp_impl->get_model_impl();
    formula_tokens_store_ptr_t ts = formula_tokens_store::create(std::move(tokens));
    calc_status_ptr_t cs =
        detail::model_context_impl::create_group_status(group_range, std::move(result));
    cxt.ensure_empty_or_throw(group_range);
    cxt.write_formula_group(group_range, cs, ts);
    mp_impl->formula_cells_to_register.push_back(group_range.first);
}

void model_context_loader::finalize()
{
    detail::model_context_impl& cxt = mp_impl->get_model_impl();
    mp_impl->finalized = true;

    // A loader never overwrites cells, so every position still holds the
    // formula cell it got.
    std::vector<std::pair<abs_address_t, const formula_cell*>> cells;
    cells.reserve(mp_impl->formula_cells_to_register.size());

    for (const abs_address_t& pos : mp_impl->formula_cells_to_register)
    {
        const formula_cell* fc = cxt.get_formula_cell(pos);
        assert(fc);
        assert(fc->get_parent_position(pos) == pos);
        cells.emplace_back(pos, fc);
    }

    // Validate every cell before registering any of them, so that a
    // rejected cell leaves the tracker unchanged.
    std::vector<std::vector<const formula_token*>> ref_tokens;
    ref_tokens.reserve(cells.size());

    for (const auto& [pos, fc] : cells)
        ref_tokens.push_back(detail::validate_formula_registration(cxt, pos, *fc));

    for (std::size_t i = 0; i < cells.size(); ++i)
    {
        const auto& [pos, fc] = cells[i];
        detail::apply_formula_registration(cxt, pos, *fc, ref_tokens[i]);
    }
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
