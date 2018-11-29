/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/address.hpp"

#include <sstream>
#include <limits>

using namespace std;

namespace ixion {

static const row_t row_max = numeric_limits<row_t>::max();
static const col_t column_max = numeric_limits<col_t>::max();

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

string abs_address_t::get_name() const
{
    ostringstream os;
    os << "(sheet=" << sheet << "; row=" << row << "; column=" << column << ")";
    return os.str();
}

size_t abs_address_t::hash::operator()(const abs_address_t& addr) const
{
    return addr.sheet + addr.row + addr.column;
}

bool operator== (const abs_address_t& left, const abs_address_t& right)
{
    return left.sheet == right.sheet &&
        left.row == right.row &&
        left.column == right.column;
}

bool operator!= (const abs_address_t& left, const abs_address_t& right)
{
    return !operator==(left, right);
}

bool operator< (const abs_address_t& left, const abs_address_t& right)
{
    if (left.sheet != right.sheet)
        return left.sheet < right.sheet;

    if (left.row != right.row)
        return left.row < right.row;

    return left.column < right.column;
}

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

    if (!abs_sheet)
        abs_addr.sheet += origin.sheet;

    if (!abs_row)
        abs_addr.row += origin.row;

    if (!abs_column)
        abs_addr.column += origin.column;

    return abs_addr;
}

string address_t::get_name() const
{
    ostringstream os;
    os << "(row=" << row << " [";
    if (abs_row)
        os << "abs";
    else
        os << "rel";
    os << "]; column=" << column << " [";
    if (abs_column)
        os << "abs";
    else
        os << "rel";
    os << "])";
    return os.str();
}

void address_t::set_absolute(bool abs)
{
    abs_sheet = abs;
    abs_row = abs;
    abs_column = abs;
}

size_t address_t::hash::operator()(const address_t& addr) const
{
    return 0;
}

bool operator== (const address_t& left, const address_t& right)
{
    return left.sheet == right.sheet &&
        left.row == right.row &&
        left.column == right.column &&
        left.abs_sheet == right.abs_sheet &&
        left.abs_row == right.abs_row &&
        left.abs_column == right.abs_column;
}

bool operator!=(const address_t& left, const address_t& right)
{
    return !operator==(left, right);
}

bool operator< (const address_t& left, const address_t& right)
{
    // Not sure how to compare absolute and relative addresses, but let's make
    // absolute address always greater than relative one until we find a
    // better way.

    if (left.abs_sheet != right.abs_sheet)
        return left.abs_sheet < right.abs_sheet;

    if (left.abs_row != right.abs_row)
        return left.abs_row < right.abs_row;

    if (left.abs_column != right.abs_column)
        return left.abs_column < right.abs_column;

    if (left.sheet != right.sheet)
        return left.sheet < right.sheet;

    if (left.row != right.row)
        return left.row < right.row;

    return left.column < right.column;
}

abs_rc_address_t::abs_rc_address_t() : row(0), column(0)
{
}

abs_rc_address_t::abs_rc_address_t(init_invalid) :
    row(-1), column(-1) {}

abs_rc_address_t::abs_rc_address_t(row_t _row, col_t _column) :
    row(_row), column(_column) {}

abs_rc_address_t::abs_rc_address_t(const abs_rc_address_t& r) :
    row(r.row), column(r.column) {}

bool abs_rc_address_t::valid() const
{
    return row >= 0 && column >= 0 && row <= row_unset && column <= column_unset;
}

size_t abs_rc_address_t::hash::operator() (const abs_rc_address_t& addr) const
{
    size_t hv = addr.column;
    hv <<= 16;
    hv += addr.row;
    return hv;
}

bool operator== (const abs_rc_address_t& left, const abs_rc_address_t& right)
{
    return left.row == right.row && left.column == right.column;
}

bool operator!= (const abs_rc_address_t& left, const abs_rc_address_t& right)
{
    return !operator==(left, right);
}

bool operator< (const abs_rc_address_t& left, const abs_rc_address_t& right)
{
    if (left.row != right.row)
        return left.row < right.row;

    return left.column < right.column;
}

rc_address_t::rc_address_t() :
    row(0), column(0), abs_row(true), abs_column(true) {}

rc_address_t::rc_address_t(row_t _row, col_t _column, bool _abs_row, bool _abs_column) :
    row(_row), column(_column), abs_row(_abs_row), abs_column(_abs_column) {}

rc_address_t::rc_address_t(const rc_address_t& r) :
    row(r.row), column(r.column), abs_row(r.abs_row), abs_column(r.abs_column) {}

size_t rc_address_t::hash::operator()(const rc_address_t& addr) const
{
    size_t hv = addr.column;
    hv <<= 16;
    hv += addr.row;
    return hv;
}

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
        std::ostringstream os;
        os << "abs_range_t: invalid span (row=" << _row_span << "; col=" << _col_span << ")";
        throw std::range_error(os.str());
    }
}

abs_range_t::abs_range_t(const abs_address_t& addr) :
    first(addr), last(addr) {}

size_t abs_range_t::hash::operator() (const abs_range_t& range) const
{
    abs_address_t::hash adr_hash;
    return adr_hash(range.first) + 65536*adr_hash(range.last);
}

bool abs_range_t::valid() const
{
    return first.valid() && last.valid() &&
        first.sheet == last.sheet &&
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

bool operator==(const abs_range_t& left, const abs_range_t& right)
{
    return left.first == right.first && left.last == right.last;
}

bool operator!=(const abs_range_t& left, const abs_range_t& right)
{
    return !operator==(left, right);
}

bool operator<(const abs_range_t& left, const abs_range_t& right)
{
    if (left.first != right.first)
        return left.first < right.first;
    return left.last < right.last;
}

abs_rc_range_t::abs_rc_range_t() {}
abs_rc_range_t::abs_rc_range_t(init_invalid) :
    first(abs_rc_address_t::invalid), last(abs_rc_address_t::invalid) {}

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

bool operator==(const abs_rc_range_t& left, const abs_rc_range_t& right)
{
    return left.first == right.first && left.last == right.last;
}

bool operator!=(const abs_rc_range_t& left, const abs_rc_range_t& right)
{
    return !operator==(left, right);
}

bool operator<(const abs_rc_range_t& left, const abs_rc_range_t& right)
{
    if (left.first != right.first)
        return left.first < right.first;
    return left.last < right.last;
}

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

size_t range_t::hash::operator() (const range_t& range) const
{
    address_t::hash adr_hash;
    return adr_hash(range.first) + 65536*adr_hash(range.last);
}

bool operator==(const range_t& left, const range_t& right)
{
    return left.first == right.first && left.last == right.last;
}

bool operator!=(const range_t& left, const range_t& right)
{
    return !operator==(left, right);
}

std::ostream& operator<<(std::ostream& os, const abs_address_t& addr)
{
    os << "(sheet:" << addr.sheet << "; row:" << addr.row << "; column:" << addr.column << ")";
    return os;
}

std::ostream& operator<<(std::ostream& os, const address_t& addr)
{
    os << "(sheet:" << addr.sheet << " " << (addr.abs_sheet?"abs":"rel")
        << "; row:" << addr.row << " " << (addr.abs_row?"abs":"rel")
        <<"; column:" << addr.column << " " << (addr.abs_column?"abs":"rel") << ")";
    return os;
}

std::ostream& operator<<(std::ostream& os, const abs_range_t& range)
{
    os << range.first << "-" << range.last;
    return os;
}

std::ostream& operator<<(std::ostream& os, const range_t& range)
{
    os << range.first << "-" << range.last;
    return os;
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
