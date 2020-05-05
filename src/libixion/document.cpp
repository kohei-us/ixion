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
            formula_name_t name = resolver.resolve(pos.value.str, pos.value.n, abs_address_t());
            if (name.type != formula_name_t::cell_reference)
            {
                std::ostringstream os;
                os << "invalid cell address: " << std::string(pos.value.str, pos.value.n);
                throw std::invalid_argument(os.str());
            }

            return to_address(name.address).to_abs(abs_address_t());
        }
        case document::cell_pos::cp_type::address:
            return abs_address_t(pos.value.sheet, pos.value.row, pos.value.column);
    }

    throw std::logic_error("unrecognized cell position type.");
}

} // anonymous namespace

document::cell_pos::cell_pos(const char* p) :
    type(cp_type::string)
{
    value.str = p;
    value.n = std::strlen(p);
}

document::cell_pos::cell_pos(const char* p, size_t n) :
    type(cp_type::string)
{
    value.str = p;
    value.n = n;
}

document::cell_pos::cell_pos(const abs_address_t& addr) :
    type(cp_type::address)
{
    value.sheet = addr.sheet;
    value.row = addr.row;
    value.column = addr.column;
}

struct document::impl
{
    model_context cxt;
    std::unique_ptr<formula_name_resolver> resolver;

    impl() :
        cxt(),
        resolver(formula_name_resolver::get(formula_name_resolver_t::excel_a1, &cxt))
    {}

    void append_sheet(std::string name)
    {
        cxt.append_sheet(std::move(name));
    }

    void set_numeric_cell(cell_pos pos, double val)
    {
        abs_address_t addr = to_address(cxt, *resolver, pos);
        cxt.set_numeric_cell(addr, val);
    }

    double get_numeric_value(cell_pos pos) const
    {
        abs_address_t addr = to_address(cxt, *resolver, pos);
        return cxt.get_numeric_value(addr);
    }
};

document::document() :
    mp_impl(ixion::make_unique<impl>()) {}

document::~document() {}

void document::append_sheet(std::string name)
{
    mp_impl->append_sheet(std::move(name));
}

void document::set_numeric_cell(cell_pos pos, double val)
{
    mp_impl->set_numeric_cell(pos, val);
}

double document::get_numeric_value(cell_pos pos) const
{
    return mp_impl->get_numeric_value(pos);
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
