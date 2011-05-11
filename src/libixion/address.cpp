/*************************************************************************
 *
 * Copyright (c) 2011 Kohei Yoshida
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 ************************************************************************/

#include "ixion/address.hpp"

#include <sstream>

using namespace std;

namespace ixion {

abs_address_t::abs_address_t() : 
    sheet(0), row(0), column(0) {}

abs_address_t::abs_address_t(sheet_t _sheet, row_t _row, col_t _column) : 
    sheet(_sheet), row(_row), column(_column) {}

abs_address_t::abs_address_t(const abs_address_t& r) : 
    sheet(r.sheet), row(r.row), column(r.column) {}

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

abs_range_t::abs_range_t() {}

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

range_t::range_t() {}
range_t::range_t(const address_t& _first, const address_t& _last) :
    first(_first), last(_last) {}

bool operator==(const range_t& left, const range_t& right)
{
    return left.first == right.first && left.last == right.last;
}

bool operator!=(const range_t& left, const range_t& right)
{
    return !operator==(left, right);
}

}
