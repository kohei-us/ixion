/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/cell_access.hpp"
#include "ixion/global.hpp"
#include "ixion/model_context.hpp"

#include "model_context_impl.hpp"
#include "workbook.hpp"
#include "utils.hpp"

namespace ixion {

struct cell_access::impl
{
    column_store_t::const_position_type pos;
    impl() {}
};

cell_access::cell_access(const model_context& cxt, const abs_address_t& addr) :
    mp_impl(ixion::make_unique<impl>())
{
    mp_impl->pos = cxt.mp_impl->get_cell_position(addr);
}

cell_access::cell_access(cell_access&& other) :
    mp_impl(std::move(other.mp_impl))
{
    other.mp_impl = ixion::make_unique<impl>();
}

cell_access& cell_access::operator= (cell_access&& other)
{
    mp_impl = std::move(other.mp_impl);
    other.mp_impl = ixion::make_unique<impl>();
    return *this;
}

cell_access::~cell_access() {}

celltype_t cell_access::get_type() const
{
    return detail::to_celltype(mp_impl->pos.first->type);
}

const formula_cell* cell_access::get_formula_cell() const
{
    if (mp_impl->pos.first->type != element_type_formula)
        return nullptr;

    return formula_element_block::at(*mp_impl->pos.first->data, mp_impl->pos.second);
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
            return p->get_value();
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
            return p->get_value() != 0.0 ? true : false;
        }
        default:
            ;
    }
    return false;
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

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
