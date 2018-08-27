/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/types.hpp"

#include <limits>
#include <vector>

namespace ixion {

const sheet_t global_scope = -1;
const sheet_t invalid_sheet = -2;

const string_id_t empty_string_id = std::numeric_limits<string_id_t>::max();

rc_size_t::rc_size_t() : row(0), column(0) {}
rc_size_t::rc_size_t(const rc_size_t& other) : row(other.row), column(other.column) {}
rc_size_t::rc_size_t(row_t _row, col_t _column) : row(_row), column(_column) {}
rc_size_t::~rc_size_t() {}

rc_size_t& rc_size_t::operator= (const rc_size_t& other)
{
    row = other.row;
    column = other.column;
    return *this;
}

formula_group_t::formula_group_t() : size(), identity(0), grouped(false) {}
formula_group_t::formula_group_t(const formula_group_t& r) :
    size(r.size), identity(r.identity), grouped(r.grouped) {}
formula_group_t::formula_group_t(const rc_size_t& _group_size, uintptr_t _identity, bool _grouped) :
    size(_group_size), identity(_identity), grouped(_grouped) {}
formula_group_t::~formula_group_t() {}

formula_group_t& formula_group_t::operator= (const formula_group_t& other)
{
    size = other.size;
    identity = other.identity;
    return *this;
}

const char* get_formula_error_name(formula_error_t fe)
{
    static const char* default_err_name = "#ERR!";

    static const std::vector<const char*> names = {
        "",        // 0: no error
        "#REF!",   // 1: result not available
        "#DIV/0!", // 2: division by zero
        "#NUM!",   // 3: invalid expression
        "#NAME?",  // 4: name not found
        "#NULL!",  // 5: no range intersection
        "#VALUE!", // 6: invalid value type
    };

    if (static_cast<size_t>(fe) < names.size())
        return names[static_cast<size_t>(fe)];

    return default_err_name;
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
