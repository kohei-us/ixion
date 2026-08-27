/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <ixion/sheet_view.hpp>
#include <ixion/address.hpp>

#include "model_context_impl.hpp"
#include "sheet_store.hpp"
#include "utils.hpp"

namespace ixion {

struct sheet_view::impl
{
    const detail::model_context_impl& cxt;
    sheet_t sheet;
    std::string name;

    // Snapshot of the base sheet taken when the view was created.  It is cloned
    // from the base sheet, so if COW is enabled this view and the base sheet
    // share their content.
    detail::sheet_store store;

    impl(const detail::model_context_impl& _cxt, sheet_t _sheet, std::string _name,
         const detail::sheet_store& base) :
        cxt(_cxt), sheet(_sheet), name(std::move(_name)), store(base.clone()) {}

    column_store_t::const_position_type get_cell_position(const abs_rc_address_t& pos) const
    {
        return store.at(pos.column).position(pos.row);
    }
};

sheet_view::sheet_view(
    const detail::model_context_impl& cxt, sheet_t sheet, std::string name,
    const detail::sheet_store& base) :
    mp_impl(std::make_unique<impl>(cxt, sheet, std::move(name), base))
{
}

sheet_view::~sheet_view() = default;

sheet_t sheet_view::get_sheet() const
{
    return mp_impl->sheet;
}

std::string_view sheet_view::get_name() const
{
    return mp_impl->name;
}

cell_t sheet_view::get_celltype(const abs_rc_address_t& pos) const
{
    auto cell_pos = mp_impl->get_cell_position(pos);
    return detail::to_celltype(cell_pos.first->type);
}

double sheet_view::get_numeric_value(const abs_rc_address_t& pos) const
{
    auto cell_pos = mp_impl->get_cell_position(pos);
    return mp_impl->cxt.get_numeric_value(cell_pos);
}

bool sheet_view::get_boolean_value(const abs_rc_address_t& pos) const
{
    auto cell_pos = mp_impl->get_cell_position(pos);
    return mp_impl->cxt.get_boolean_value(cell_pos);
}

std::string_view sheet_view::get_string_value(const abs_rc_address_t& pos) const
{
    auto cell_pos = mp_impl->get_cell_position(pos);
    return mp_impl->cxt.get_string_value(cell_pos);
}

const formula_cell* sheet_view::get_formula_cell(const abs_rc_address_t& pos) const
{
    auto cell_pos = mp_impl->get_cell_position(pos);
    return detail::model_context_impl::get_formula_cell(cell_pos);
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
