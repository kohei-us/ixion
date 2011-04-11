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

#include "address.hpp"

#include <sstream>

using namespace std;

namespace ixion {

address_t::address_t() : 
    sheet(0), row(0), column(0), abs_sheet(true), abs_row(true), abs_column(true) {}

address_t::address_t(sheet_t _sheet, row_t _row, col_t _column, bool _abs_sheet, bool _abs_row, bool _abs_column) : 
    sheet(_sheet), row(_row), column(_column), 
    abs_sheet(_abs_sheet), abs_row(_abs_row), abs_column(_abs_column) {}

address_t::address_t(const address_t& r) : 
    sheet(r.sheet), row(r.row), column(r.column),
    abs_sheet(r.abs_sheet), abs_row(r.abs_row), abs_column(r.abs_column) {}

address_t address_t::to_abs(const address_t& origin) const
{
    address_t abs_addr(*this);
    if (!abs_addr.abs_sheet)
    {
        abs_addr.sheet += origin.sheet;
        abs_addr.abs_sheet = true;
    }

    if (!abs_addr.abs_row)
    {
        abs_addr.row += origin.row;
        abs_addr.abs_row = true;
    }

    if (!abs_addr.abs_column)
    {
        abs_addr.column += origin.column;
        abs_addr.abs_column = true;
    }
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


}
