/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <ixion/types.hpp>
#include <ixion/macros.hpp>

#include "column_store_type.hpp"

#include <limits>
#include <vector>

namespace ixion {

namespace {

constexpr std::string_view formula_error_names[] = {
    "",        // 0: no error
    "#REF!",   // 1: result not available
    "#DIV/0!", // 2: division by zero
    "#NUM!",   // 3: invalid expression
    "#NAME?",  // 4: name not found
    "#NULL!",  // 5: no range intersection
    "#VALUE!", // 6: invalid value type
    "#N/A",    // 7: no value available
};

} // anonymous namespace

const sheet_t global_scope = -1;
const sheet_t invalid_sheet = -2;

const string_id_t empty_string_id = std::numeric_limits<string_id_t>::max();

bool is_valid_sheet(sheet_t sheet)
{
    return sheet >= 0;
}

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

std::string_view get_formula_error_name(formula_error_t fe)
{
    constexpr std::string_view default_err_name = "#ERR!";

    if (std::size_t(fe) < std::size(formula_error_names))
        return formula_error_names[std::size_t(fe)];

    return default_err_name;
}

formula_error_t to_formula_error_type(std::string_view s)
{
    const auto* p = formula_error_names;
    const auto* p_end = p + std::size(formula_error_names);

    p = std::find(p, p_end, s);

    if (p == p_end)
        return formula_error_t::no_error;

    return formula_error_t(std::distance(formula_error_names, p));
}

column_block_shape_t::column_block_shape_t() :
    position(0), size(0), offset(0), type(column_block_t::unknown), data(nullptr)
{
}

column_block_shape_t::column_block_shape_t(
    std::size_t _position, std::size_t _size, std::size_t _offset,
    column_block_t _type, column_block_handle _data) :
    position(_position), size(_size), offset(_offset), type(_type), data(_data)
{
}

column_block_shape_t::column_block_shape_t(const column_block_shape_t& other) :
    position(other.position), size(other.size), offset(other.offset),
    type(other.type), data(other.data)
{
}

column_block_shape_t& column_block_shape_t::operator=(const column_block_shape_t& other)
{
    position = other.position;
    size = other.size;
    offset = other.offset;
    type = other.type;
    data = other.data;
    return *this;
}

std::ostream& operator<< (std::ostream& os, const column_block_shape_t& v)
{
    os << "(column_block_shape_t: position=" << v.position << "; size=" << v.size << "; offset=" << v.offset
        << "; type=" << int(v.type) << "; data=" << v.data << ")";
    return os;
}

print_config::print_config() = default;
print_config::print_config(const print_config&) = default;
print_config::~print_config() = default;

} // namespace ixion

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
