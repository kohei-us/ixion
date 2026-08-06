/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/address.hpp"

#include <format>
#include <limits>
#include <sstream>

namespace ixion {

static const row_t row_max = std::numeric_limits<row_t>::max();
static const col_t column_max = std::numeric_limits<col_t>::max();

const row_t row_unset = row_max - 9;
const row_t row_upper_bound = row_max - 10;

const col_t column_unset = column_max - 9;
const col_t column_upper_bound = column_max / 26 - 26;

abs_address_t::abs_address_t() : sheet(0), row(0), column(0) {}
abs_address_t::abs_address_t(init_invalid) : sheet(-1), row(-1), column(-1) {}

abs_address_t::abs_address_t(sheet_t _sheet, row_t _row, col_t _column) :
    sheet(_sheet), row(_row), column(_column) {}

abs_address_t::abs_address_t(const abs_address_t& r) :
    sheet(r.sheet), row(r.row), column(r.column) {}

bool abs_address_t::valid() const
{
    return sheet >= 0 && row >= 0 && column >= 0 && row <= row_unset && column <= column_unset;
}

std::string abs_address_t::get_name() const
{
    return std::format("(sheet={}; row={}; column={})", sheet, row, column);
}

std::size_t abs_address_t::hash::operator()(const abs_address_t& addr) const
{
    return addr.sheet + addr.row + addr.column;
}

std::strong_ordering abs_address_t::operator<=>(const abs_address_t& r) const = default;
bool abs_address_t::operator==(const abs_address_t& r) const = default;

address_t::address_t() :
    sheet(0), row(0), column(0), abs_sheet(true), abs_row(true), abs_column(true) {}

address_t::address_t(sheet_t _sheet, row_t _row, col_t _column, bool _abs_sheet, bool _abs_row, bool _abs_column) :
    sheet(_sheet), row(_row), column(_column),
    abs_sheet(_abs_sheet), abs_row(_abs_row), abs_column(_abs_column) {}

address_t::address_t(const address_t& r) :
    sheet(r.sheet), row(r.row), column(r.column),
    abs_sheet(r.abs_sheet), abs_row(r.abs_row), abs_column(r.abs_column) {}

address_t::address_t(const abs_address_t& r) :
    sheet(r.sheet), row(r.row), column(r.column),
    abs_sheet(true), abs_row(true), abs_column(true) {}

bool address_t::valid() const
{
    if (abs_sheet)
    {
        if (sheet < 0)
            return false;
    }

    if (row > row_unset)
        return false;

    if (abs_row)
    {
        if (row < 0)
            return false;
    }
    else
    {
        if (row < -row_upper_bound)
            return false;
    }

    if (column > column_unset)
        return false;

    if (abs_column)
    {
        if (column < 0)
            return false;
    }
    else
    {
        if (column < -column_upper_bound)
            return false;
    }

    return true;
}

abs_address_t address_t::to_abs(const abs_address_t& origin) const
{
    abs_address_t abs_addr;
    abs_addr.sheet = sheet;
    abs_addr.row = row;
    abs_addr.column = column;

    if (!is_valid_sheet(origin.sheet))
        // If the origin sheet is invalid, then any sheet position relative to that should be invalid.
        abs_addr.sheet = origin.sheet;
    else if (!abs_sheet)
        abs_addr.sheet += origin.sheet;

    if (!abs_row && row <= row_upper_bound)
        abs_addr.row += origin.row;

    if (!abs_column && column <= column_upper_bound)
        abs_addr.column += origin.column;

    return abs_addr;
}

std::string address_t::get_name() const
{
    return std::format("(row={} [{}]; column={} [{}])",
        row, abs_row ? "abs" : "rel",
        column, abs_column ? "abs" : "rel");
}

void address_t::set_absolute(bool abs)
{
    abs_sheet = abs;
    abs_row = abs;
    abs_column = abs;
}

std::size_t address_t::hash::operator()(const address_t& addr) const
{
    return 0;
}

std::strong_ordering address_t::operator<=>(const address_t& r) const = default;
bool address_t::operator==(const address_t& r) const = default;

abs_rc_address_t::abs_rc_address_t() : row(0), column(0)
{
}

abs_rc_address_t::abs_rc_address_t(init_invalid) :
    row(-1), column(-1) {}

abs_rc_address_t::abs_rc_address_t(row_t _row, col_t _column) :
    row(_row), column(_column) {}

abs_rc_address_t::abs_rc_address_t(const abs_rc_address_t& r) :
    row(r.row), column(r.column) {}

abs_rc_address_t::abs_rc_address_t(const abs_address_t& r) :
    row(r.row), column(r.column) {}

bool abs_rc_address_t::valid() const
{
    return row >= 0 && column >= 0 && row <= row_unset && column <= column_unset;
}

std::size_t abs_rc_address_t::hash::operator() (const abs_rc_address_t& addr) const
{
    size_t hv = addr.column;
    hv <<= 16;
    hv += addr.row;
    return hv;
}

std::strong_ordering abs_rc_address_t::operator<=>(const abs_rc_address_t& r) const = default;
bool abs_rc_address_t::operator==(const abs_rc_address_t& r) const = default;

rc_address_t::rc_address_t() :
    row(0), column(0), abs_row(true), abs_column(true) {}

rc_address_t::rc_address_t(row_t _row, col_t _column, bool _abs_row, bool _abs_column) :
    row(_row), column(_column), abs_row(_abs_row), abs_column(_abs_column) {}

rc_address_t::rc_address_t(const rc_address_t& r) :
    row(r.row), column(r.column), abs_row(r.abs_row), abs_column(r.abs_column) {}

rc_address_t::rc_address_t(const abs_rc_address_t& r) :
    row(r.row), column(r.column), abs_row(true), abs_column(true) {}

std::size_t rc_address_t::hash::operator()(const rc_address_t& addr) const
{
    std::size_t hv = addr.column;
    hv <<= 16;
    hv += addr.row;
    return hv;
}

std::strong_ordering rc_address_t::operator<=>(const rc_address_t& r) const = default;
bool rc_address_t::operator==(const rc_address_t& r) const = default;

abs_range_t::abs_range_t() {}
abs_range_t::abs_range_t(init_invalid) :
    first(abs_address_t::invalid), last(abs_address_t::invalid) {}

abs_range_t::abs_range_t(sheet_t _sheet, row_t _row, col_t _col) :
    first(_sheet, _row, _col), last(_sheet, _row, _col) {}

abs_range_t::abs_range_t(sheet_t _sheet, row_t _row, col_t _col, row_t _row_span, col_t _col_span) :
    first(_sheet, _row, _col), last(_sheet, _row + _row_span - 1, _col + _col_span - 1)
{
    if (_row_span < 1 || _col_span < 1)
    {
        throw std::range_error(
            std::format("abs_range_t: invalid span (row={}; col={})", _row_span, _col_span));
    }
}

abs_range_t::abs_range_t(const abs_address_t& addr, row_t row_span, col_t col_span) :
    first(addr), last(addr)
{
    if (row_span > 0)
        last.row += row_span - 1;
    if (col_span > 0)
        last.column += col_span - 1;
}

abs_range_t::abs_range_t(const abs_address_t& addr) :
    first(addr), last(addr) {}

abs_range_t::abs_range_t(const abs_address_t& _first, const abs_address_t& _last) :
    first(_first), last(_last)
{
    if (last.row < first.row || last.column < first.column)
    {
        std::ostringstream os;
        os << "abs_range_t: the last position (" << last << ") precedes the first position (" << first << ")";
        throw std::range_error(os.str());
    }
}

std::size_t abs_range_t::hash::operator() (const abs_range_t& range) const
{
    abs_address_t::hash adr_hash;
    return adr_hash(range.first) + 65536*adr_hash(range.last);
}

bool abs_range_t::valid() const
{
    return first.valid() && last.valid() &&
        first.sheet <= last.sheet &&
        first.column <= last.column &&
        first.row <= last.row;
}

void abs_range_t::set_all_columns()
{
    first.column = column_unset;
    last.column = column_unset;
}

void abs_range_t::set_all_rows()
{
    first.row = row_unset;
    last.row = row_unset;
}

bool abs_range_t::all_columns() const
{
    return first.column == column_unset && last.column == column_unset;
}

bool abs_range_t::all_rows() const
{
    return first.row == row_unset && last.row == row_unset;
}

bool abs_range_t::contains(const abs_address_t& addr) const
{
    return first.sheet <= addr.sheet && addr.sheet <= last.sheet &&
        first.row <= addr.row && addr.row <= last.row &&
        first.column <= addr.column && addr.column <= last.column;
}

void abs_range_t::reorder()
{
    if (first.sheet > last.sheet)
        std::swap(first.sheet, last.sheet);

    if (first.row > last.row)
        std::swap(first.row, last.row);

    if (first.column > last.column)
        std::swap(first.column, last.column);
}

std::strong_ordering abs_range_t::operator<=>(const abs_range_t& r) const = default;
bool abs_range_t::operator==(const abs_range_t& r) const = default;

abs_rc_range_t::abs_rc_range_t() {}
abs_rc_range_t::abs_rc_range_t(init_invalid) :
    first(abs_rc_address_t::invalid), last(abs_rc_address_t::invalid) {}

abs_rc_range_t::abs_rc_range_t(const abs_rc_range_t& other) :
    first(other.first), last(other.last) {}

abs_rc_range_t::abs_rc_range_t(const abs_range_t& other) :
    first(other.first), last(other.last) {}

size_t abs_rc_range_t::hash::operator() (const abs_rc_range_t& range) const
{
    abs_rc_address_t::hash adr_hash;
    return adr_hash(range.first) + 65536*adr_hash(range.last);
}

bool abs_rc_range_t::valid() const
{
    if (!first.valid() || !last.valid())
        return false;

    if (first.row != row_unset && last.row != row_unset)
    {
        if (first.row > last.row)
            return false;
    }

    if (first.column != column_unset && last.column != column_unset)
    {
        if (first.column > last.column)
            return false;
    }

    return true;
}

void abs_rc_range_t::set_all_columns()
{
    first.column = column_unset;
    last.column = column_unset;
}

void abs_rc_range_t::set_all_rows()
{
    first.row = row_unset;
    last.row = row_unset;
}

bool abs_rc_range_t::all_columns() const
{
    return first.column == column_unset && last.column == column_unset;
}

bool abs_rc_range_t::all_rows() const
{
    return first.row == row_unset && last.row == row_unset;
}

bool abs_rc_range_t::contains(const abs_rc_address_t& addr) const
{
    return first.row <= addr.row && addr.row <= last.row &&
        first.column <= addr.column && addr.column <= last.column;
}

std::strong_ordering abs_rc_range_t::operator<=>(const abs_rc_range_t& r) const = default;
bool abs_rc_range_t::operator==(const abs_rc_range_t& r) const = default;

range_t::range_t() {}
range_t::range_t(const address_t& _first, const address_t& _last) :
    first(_first), last(_last) {}

range_t::range_t(const range_t& r) : first(r.first), last(r.last) {}
range_t::range_t(const abs_range_t& r) : first(r.first), last(r.last) {}

bool range_t::valid() const
{
    return first.valid() && last.valid();
}

void range_t::set_all_columns()
{
    first.column = column_unset;
    last.column = column_unset;
}

void range_t::set_all_rows()
{
    first.row = row_unset;
    last.row = row_unset;
}

bool range_t::all_columns() const
{
    return first.column == column_unset && last.column == column_unset;
}

bool range_t::all_rows() const
{
    return first.row == row_unset && last.row == row_unset;
}

abs_range_t range_t::to_abs(const abs_address_t& origin) const
{
    abs_range_t ret;
    ret.first = first.to_abs(origin);
    ret.last = last.to_abs(origin);
    return ret;
}

void range_t::set_absolute(bool abs)
{
    first.set_absolute(abs);
    last.set_absolute(abs);
}

std::size_t range_t::hash::operator() (const range_t& range) const
{
    address_t::hash adr_hash;
    return adr_hash(range.first) + 65536*adr_hash(range.last);
}

bool operator==(const range_t& left, const range_t& right)
{
    return left.first == right.first && left.last == right.last;
}

rc_range_t::rc_range_t() {}
rc_range_t::rc_range_t(const rc_address_t& _first, const rc_address_t& _last) :
    first(_first), last(_last) {}

rc_range_t::rc_range_t(const rc_range_t& r) : first(r.first), last(r.last) {}
rc_range_t::rc_range_t(const abs_rc_range_t& r) : first(r.first), last(r.last) {}

void rc_range_t::set_all_columns()
{
    first.column = column_unset;
    last.column = column_unset;
}

void rc_range_t::set_all_rows()
{
    first.row = row_unset;
    last.row = row_unset;
}

bool rc_range_t::all_columns() const
{
    return first.column == column_unset && last.column == column_unset;
}

bool rc_range_t::all_rows() const
{
    return first.row == row_unset && last.row == row_unset;
}

std::size_t rc_range_t::hash::operator() (const rc_range_t& range) const
{
    rc_address_t::hash adr_hash;
    return adr_hash(range.first) + 65536*adr_hash(range.last);
}

bool operator==(const rc_range_t& left, const rc_range_t& right)
{
    return left.first == right.first && left.last == right.last;
}

std::ostream& operator<<(std::ostream& os, const abs_address_t& addr)
{
    os << "(sheet:" << addr.sheet << "; row:" << addr.row << "; column:" << addr.column << ")";
    return os;
}

std::ostream& operator<<(std::ostream& os, const abs_rc_address_t& addr)
{
    os << "(row:" << addr.row << "; column:" << addr.column << ")";
    return os;
}

std::ostream& operator<<(std::ostream& os, const address_t& addr)
{
    os << "(sheet:" << addr.sheet << " " << (addr.abs_sheet?"abs":"rel")
        << "; row:" << addr.row << " " << (addr.abs_row?"abs":"rel")
        <<"; column:" << addr.column << " " << (addr.abs_column?"abs":"rel") << ")";
    return os;
}

std::ostream& operator<<(std::ostream& os, const rc_address_t& addr)
{
    os << "(row:" << addr.row << " " << (addr.abs_row?"abs":"rel")
       <<"; column:" << addr.column << " " << (addr.abs_column?"abs":"rel") << ")";
    return os;
}

std::ostream& operator<<(std::ostream& os, const abs_range_t& range)
{
    os << range.first << "-" << range.last;
    return os;
}

std::ostream& operator<<(std::ostream& os, const abs_rc_range_t& range)
{
    os << range.first << "-" << range.last;
    return os;
}

std::ostream& operator<<(std::ostream& os, const range_t& range)
{
    os << range.first << "-" << range.last;
    return os;
}

std::ostream& operator<<(std::ostream& os, const rc_range_t& range)
{
    os << range.first << "-" << range.last;
    return os;
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
