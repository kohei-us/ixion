/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/document.hpp"
#include "ixion/global.hpp"
#include "ixion/model_context.hpp"
#include "ixion/formula_name_resolver.hpp"
#include "ixion/formula.hpp"
#include "ixion/cell_access.hpp"

#include <cstring>
#include <sstream>

namespace ixion {

namespace {

abs_address_t to_address(
    const model_context& cxt, const formula_name_resolver& resolver, const document::cell_pos& pos)
{
    switch (pos.type)
    {
        case document::cell_pos::cp_type::string:
        {
            std::string_view s = std::get<std::string_view>(pos.value);
            formula_name_t name = resolver.resolve(s, abs_address_t());
            if (name.type != formula_name_t::cell_reference)
            {
                std::ostringstream os;
                os << "invalid cell address: " << s;
                throw std::invalid_argument(os.str());
            }

            return std::get<address_t>(name.value).to_abs(abs_address_t());
        }
        case document::cell_pos::cp_type::address:
        {
            return std::get<abs_address_t>(pos.value);
        }
    }

    throw std::logic_error("unrecognized cell position type.");
}

} // anonymous namespace

document::cell_pos::cell_pos(const char* p) :
    type(cp_type::string),
    value(p)
{
}

document::cell_pos::cell_pos(std::string_view s) :
    type(cp_type::string),
    value(s)
{
}

document::cell_pos::cell_pos(const std::string& s) :
    type(cp_type::string),
    value(s)
{
}

document::cell_pos::cell_pos(const abs_address_t& addr) :
    type(cp_type::address),
    value(addr)
{
}

document::cell_pos::cell_pos(const cell_pos& other) = default;

document::cell_pos& document::cell_pos::operator=(const cell_pos& other) = default;

struct document::impl
{
    model_context cxt;
    std::unique_ptr<formula_name_resolver> resolver;

    abs_range_set_t modified_cells;
    abs_range_set_t modified_formula_cells;

    impl() :
        cxt(),
        resolver(formula_name_resolver::get(formula_name_resolver_t::excel_a1, &cxt))
    {}

    impl(formula_name_resolver_t cell_address_type) :
        cxt(),
        resolver(formula_name_resolver::get(cell_address_type, &cxt))
    {}

    void append_sheet(std::string name)
    {
        cxt.append_sheet(std::move(name));
    }

    void set_sheet_name(sheet_t sheet, std::string name)
    {
        cxt.set_sheet_name(sheet, std::move(name));
    }

    cell_access get_cell_access(cell_pos pos) const
    {
        abs_address_t addr = to_address(cxt, *resolver, pos);
        return cxt.get_cell_access(addr);
    }

    void set_numeric_cell(cell_pos pos, double val)
    {
        abs_address_t addr = to_address(cxt, *resolver, pos);
        unregister_formula_cell(cxt, addr);
        cxt.set_numeric_cell(addr, val);
        modified_cells.insert(addr);
    }

    void set_string_cell(cell_pos pos, std::string_view s)
    {
        abs_address_t addr = to_address(cxt, *resolver, pos);
        unregister_formula_cell(cxt, addr);
        cxt.set_string_cell(addr, s);
        modified_cells.insert(addr);
    }

    void set_boolean_cell(cell_pos pos, bool val)
    {
        abs_address_t addr = to_address(cxt, *resolver, pos);
        unregister_formula_cell(cxt, addr);
        cxt.set_boolean_cell(addr, val);
        modified_cells.insert(addr);
    }

    void empty_cell(cell_pos pos)
    {
        abs_address_t addr = to_address(cxt, *resolver, pos);
        unregister_formula_cell(cxt, addr);
        cxt.empty_cell(addr);
        modified_cells.insert(addr);
    }

    double get_numeric_value(cell_pos pos) const
    {
        abs_address_t addr = to_address(cxt, *resolver, pos);
        return cxt.get_numeric_value(addr);
    }

    std::string_view get_string_value(cell_pos pos) const
    {
        abs_address_t addr = to_address(cxt, *resolver, pos);
        return cxt.get_string_value(addr);
    }

    void set_formula_cell(cell_pos pos, std::string_view formula)
    {
        abs_address_t addr = to_address(cxt, *resolver, pos);
        unregister_formula_cell(cxt, addr);
        auto tokens = parse_formula_string(cxt, addr, *resolver, formula);
        formula_cell* fc = cxt.set_formula_cell(addr, std::move(tokens));
        register_formula_cell(cxt, addr, fc);
        modified_formula_cells.insert(addr);
    }

    void calculate(size_t thread_count)
    {
        auto sorted_cells = query_and_sort_dirty_cells(cxt, modified_cells, &modified_formula_cells);
        calculate_sorted_cells(cxt, sorted_cells, thread_count);
        modified_cells.clear();
        modified_formula_cells.clear();
    }
};

document::document() :
    mp_impl(std::make_unique<impl>()) {}

document::document(formula_name_resolver_t cell_address_type) :
    mp_impl(std::make_unique<impl>(cell_address_type)) {}

document::~document() {}

void document::append_sheet(std::string name)
{
    mp_impl->append_sheet(std::move(name));
}

void document::set_sheet_name(sheet_t sheet, std::string name)
{
    mp_impl->set_sheet_name(sheet, std::move(name));
}

cell_access document::get_cell_access(cell_pos pos) const
{
    return mp_impl->get_cell_access(pos);
}

void document::set_numeric_cell(cell_pos pos, double val)
{
    mp_impl->set_numeric_cell(pos, val);
}

void document::set_string_cell(cell_pos pos, std::string_view s)
{
    mp_impl->set_string_cell(pos, s);
}

void document::set_boolean_cell(cell_pos pos, bool val)
{
    mp_impl->set_boolean_cell(pos, val);
}

void document::empty_cell(cell_pos pos)
{
    mp_impl->empty_cell(pos);
}

double document::get_numeric_value(cell_pos pos) const
{
    return mp_impl->get_numeric_value(pos);
}

std::string_view document::get_string_value(cell_pos pos) const
{
    return mp_impl->get_string_value(pos);
}

void document::set_formula_cell(cell_pos pos, std::string_view formula)
{
    mp_impl->set_formula_cell(pos, formula);
}

void document::calculate(size_t thread_count)
{
    mp_impl->calculate(thread_count);
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
