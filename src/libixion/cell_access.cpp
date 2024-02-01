/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <ixion/cell_access.hpp>
#include <ixion/global.hpp>
#include <ixion/model_context.hpp>
#include <ixion/formula_result.hpp>
#include <ixion/exceptions.hpp>

#include "model_context_impl.hpp"
#include "workbook.hpp"
#include "utils.hpp"
#include "model_types.hpp"

namespace ixion {

struct cell_access::impl
{
    const model_context& cxt;
    column_store_t::const_position_type pos;
    impl(const model_context& _cxt) : cxt(_cxt) {}
};

cell_access::cell_access(const model_context& cxt, const abs_address_t& addr) :
    mp_impl(std::make_unique<impl>(cxt))
{
    mp_impl->pos = cxt.mp_impl->get_cell_position(addr);
}

cell_access::cell_access(cell_access&& other) :
    mp_impl(std::move(other.mp_impl))
{
    other.mp_impl = std::make_unique<impl>(mp_impl->cxt);
}

cell_access& cell_access::operator= (cell_access&& other)
{
    mp_impl = std::move(other.mp_impl);
    other.mp_impl = std::make_unique<impl>(mp_impl->cxt);
    return *this;
}

cell_access::~cell_access() {}

celltype_t cell_access::get_type() const
{
    return detail::to_celltype(mp_impl->pos.first->type);
}

cell_value_t cell_access::get_value_type() const
{
    return detail::to_cell_value_type(
        mp_impl->pos, mp_impl->cxt.get_formula_result_wait_policy());
}

const formula_cell* cell_access::get_formula_cell() const
{
    if (mp_impl->pos.first->type != element_type_formula)
        return nullptr;

    return formula_element_block::at(*mp_impl->pos.first->data, mp_impl->pos.second);
}

formula_result cell_access::get_formula_result() const
{
    const formula_cell* fc = get_formula_cell();
    if (!fc)
        throw general_error("cell is not a formula cell.");

    return fc->get_result_cache(mp_impl->cxt.get_formula_result_wait_policy());
}

double cell_access::get_numeric_value() const
{
    switch (mp_impl->pos.first->type)
    {
        case element_type_numeric:
            return numeric_element_block::at(*mp_impl->pos.first->data, mp_impl->pos.second);
        case element_type_boolean:
        {
            auto it = boolean_element_block::cbegin(*mp_impl->pos.first->data);
            std::advance(it, mp_impl->pos.second);
            return *it ? 1.0 : 0.0;
        }
        case element_type_formula:
        {
            const formula_cell* p = formula_element_block::at(*mp_impl->pos.first->data, mp_impl->pos.second);
            return p->get_value(mp_impl->cxt.get_formula_result_wait_policy());
        }
        default:
            ;
    }
    return 0.0;
}

bool cell_access::get_boolean_value() const
{
    switch (mp_impl->pos.first->type)
    {
        case element_type_numeric:
            return numeric_element_block::at(*mp_impl->pos.first->data, mp_impl->pos.second) != 0.0 ? true : false;
        case element_type_boolean:
        {
            auto it = boolean_element_block::cbegin(*mp_impl->pos.first->data);
            std::advance(it, mp_impl->pos.second);
            return *it;
        }
        case element_type_formula:
        {
            const formula_cell* p = formula_element_block::at(*mp_impl->pos.first->data, mp_impl->pos.second);
            return p->get_value(mp_impl->cxt.get_formula_result_wait_policy()) == 0.0 ? false : true;
        }
        default:
            ;
    }
    return false;
}

std::string_view cell_access::get_string_value() const
{
    switch (mp_impl->pos.first->type)
    {
        case element_type_string:
        {
            string_id_t sid = string_element_block::at(*mp_impl->pos.first->data, mp_impl->pos.second);
            const std::string* p = mp_impl->cxt.get_string(sid);
            return p ? *p : std::string_view{};
        }
        case element_type_formula:
        {
            const formula_cell* p = formula_element_block::at(*mp_impl->pos.first->data, mp_impl->pos.second);
            return p->get_string(mp_impl->cxt.get_formula_result_wait_policy());
        }
        case element_type_empty:
            return detail::empty_string;
        default:
            ;
    }

    return std::string_view{};
}

string_id_t cell_access::get_string_identifier() const
{
    switch (mp_impl->pos.first->type)
    {
        case element_type_string:
            return string_element_block::at(*mp_impl->pos.first->data, mp_impl->pos.second);
        default:
            ;
    }
    return empty_string_id;
}

formula_error_t cell_access::get_error_value() const
{
    if (mp_impl->pos.first->type != element_type_formula)
        // only a formula cell can store an error value; an error value cannot
        // be stored as a normal cell value
        return formula_error_t::no_error;

    const formula_cell* fc = formula_element_block::at(*mp_impl->pos.first->data, mp_impl->pos.second);
    formula_result res = fc->get_result_cache(mp_impl->cxt.get_formula_result_wait_policy());
    if (res.get_type() != formula_result::result_type::error)
        // this formula cell doesn't have an error result.
        return formula_error_t::no_error;

    return res.get_error();
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
