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
#include <ixion/formula_name_resolver.hpp>
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
};

model_context_loader::model_context_loader(model_context& cxt) :
    mp_impl(std::make_unique<impl>(cxt))
{
}

model_context_loader::~model_context_loader() = default;

std::unique_ptr<formula_name_resolver> model_context_loader::create_name_resolver(
    formula_name_resolver_t type) const
{
    return formula_name_resolver::get(type, &mp_impl->cxt);
}

formula_tokens_t model_context_loader::parse_formula_string(
    const abs_address_t& pos, const formula_name_resolver& resolver, std::string_view formula)
{
    return ixion::parse_formula_string(mp_impl->cxt, pos, resolver, formula);
}

void model_context_loader::set_sheet_size(const rc_size_t& sheet_size)
{
    mp_impl->cxt.set_sheet_size(sheet_size);
}

sheet_t model_context_loader::append_sheet(std::string name)
{
    return mp_impl->cxt.append_sheet(std::move(name));
}

void model_context_loader::set_named_expression(std::string name, formula_tokens_t expr)
{
    mp_impl->cxt.set_named_expression(std::move(name), std::move(expr));
}

void model_context_loader::set_named_expression(
    std::string name, const abs_address_t& origin, formula_tokens_t expr)
{
    mp_impl->cxt.set_named_expression(std::move(name), origin, std::move(expr));
}

void model_context_loader::set_named_expression(
    sheet_t sheet, std::string name, formula_tokens_t expr)
{
    mp_impl->cxt.set_named_expression(sheet, std::move(name), std::move(expr));
}

void model_context_loader::set_named_expression(
    sheet_t sheet, std::string name, const abs_address_t& origin, formula_tokens_t expr)
{
    mp_impl->cxt.set_named_expression(sheet, std::move(name), origin, std::move(expr));
}

void model_context_loader::set_table(table_t tab)
{
    mp_impl->cxt.set_table(std::move(tab));
}

string_id_t model_context_loader::append_string(std::string_view s)
{
    return mp_impl->cxt.append_string(s);
}

string_id_t model_context_loader::add_string(std::string_view s)
{
    return mp_impl->cxt.add_string(s);
}

void model_context_loader::set_numeric_cell(const abs_address_t& addr, double val)
{
#ifdef IXION_DEBUG_UTILS
    mp_impl->cxt.mp_impl->ensure_empty_or_throw(addr);
#endif
    mp_impl->cxt.mp_impl->write_numeric_cell(addr, val);
}

void model_context_loader::set_boolean_cell(const abs_address_t& addr, bool val)
{
#ifdef IXION_DEBUG_UTILS
    mp_impl->cxt.mp_impl->ensure_empty_or_throw(addr);
#endif
    mp_impl->cxt.mp_impl->write_boolean_cell(addr, val);
}

void model_context_loader::set_string_cell(const abs_address_t& addr, std::string_view s)
{
#ifdef IXION_DEBUG_UTILS
    mp_impl->cxt.mp_impl->ensure_empty_or_throw(addr);
#endif
    mp_impl->cxt.mp_impl->write_string_cell(addr, s);
}

void model_context_loader::set_string_cell(const abs_address_t& addr, string_id_t identifier)
{
#ifdef IXION_DEBUG_UTILS
    mp_impl->cxt.mp_impl->ensure_empty_or_throw(addr);
#endif
    mp_impl->cxt.mp_impl->write_string_cell(addr, identifier);
}

void model_context_loader::fill_down_cells(const abs_address_t& src, std::size_t n_dst)
{
#ifdef IXION_DEBUG_UTILS
    mp_impl->cxt.mp_impl->ensure_empty_or_throw(
        abs_range_t(src.sheet, src.row + 1, src.column, row_t(n_dst), 1));
#endif
    mp_impl->cxt.mp_impl->write_fill_down_cells(src, n_dst);
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
    model_context& cxt = mp_impl->cxt;
#ifdef IXION_DEBUG_UTILS
    cxt.mp_impl->ensure_empty_or_throw(addr);
#endif
    formula_cell* p = cxt.mp_impl->write_formula_cell(addr, std::make_unique<formula_cell>(tokens));
    mp_impl->formula_cells_to_register.push_back(addr);
    return p;
}

formula_cell* model_context_loader::set_formula_cell(
    const abs_address_t& addr, const formula_tokens_store_ptr_t& tokens, formula_result result)
{
    model_context& cxt = mp_impl->cxt;
    std::unique_ptr<formula_cell> fcell = std::make_unique<formula_cell>(tokens);
    fcell->set_result_cache(std::move(result));
#ifdef IXION_DEBUG_UTILS
    cxt.mp_impl->ensure_empty_or_throw(addr);
#endif
    formula_cell* p = cxt.mp_impl->write_formula_cell(addr, std::move(fcell));
    mp_impl->formula_cells_to_register.push_back(addr);
    return p;
}

void model_context_loader::set_grouped_formula_cells(
    const abs_range_t& group_range, formula_tokens_t tokens)
{
    model_context& cxt = mp_impl->cxt;
    formula_tokens_store_ptr_t ts = formula_tokens_store::create(std::move(tokens));
    calc_status_ptr_t cs = detail::model_context_impl::create_group_status(group_range);
#ifdef IXION_DEBUG_UTILS
    cxt.mp_impl->ensure_empty_or_throw(group_range);
#endif
    cxt.mp_impl->write_formula_group(group_range, cs, ts);
    mp_impl->formula_cells_to_register.push_back(group_range.first);
}

void model_context_loader::set_grouped_formula_cells(
    const abs_range_t& group_range, formula_tokens_t tokens, formula_result result)
{
    model_context& cxt = mp_impl->cxt;
    formula_tokens_store_ptr_t ts = formula_tokens_store::create(std::move(tokens));
    calc_status_ptr_t cs =
        detail::model_context_impl::create_group_status(group_range, std::move(result));
#ifdef IXION_DEBUG_UTILS
    cxt.mp_impl->ensure_empty_or_throw(group_range);
#endif
    cxt.mp_impl->write_formula_group(group_range, cs, ts);
    mp_impl->formula_cells_to_register.push_back(group_range.first);
}

void model_context_loader::finalize()
{
    if (mp_impl->finalized)
        throw model_context_error(
            "finalize() can only be called once", model_context_error::loader_already_finalized);

    mp_impl->finalized = true;
    model_context& cxt = mp_impl->cxt;

    // A loader never overwrites cells, so every position still holds the
    // formula cell it got.
    std::vector<std::pair<abs_address_t, const formula_cell*>> cells;
    cells.reserve(mp_impl->formula_cells_to_register.size());

    for (const abs_address_t& pos : mp_impl->formula_cells_to_register)
    {
        const formula_cell* fc = std::as_const(cxt).get_formula_cell(pos);
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
