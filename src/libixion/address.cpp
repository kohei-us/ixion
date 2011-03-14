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
    sheet(0), row(0), column(0) {}

address_t::address_t(sheet_t _sheet, row_t _row, col_t _column) : 
    sheet(_sheet), row(_row), column(_column) {}

address_t::address_t(const address_t& r) : 
    sheet(r.sheet), row(r.row), column(r.column) {}

string address_t::get_name() const
{
    ostringstream os;
    os << "(row=" << row << ",column=" << column << ")";
    return os.str();
}

size_t address_t::hash::operator()(const address_t& addr) const
{
    return 0;
}

bool operator== (const address_t& left, const address_t& right)
{
    return left.sheet == right.sheet && left.row == right.row && left.column == right.column;
}

bool operator< (const address_t& left, const address_t& right)
{
    if (left.sheet != right.sheet)
        return left.sheet < right.sheet;

    if (left.row != right.row)
        return left.row < right.row;

    return left.column < right.column;
}


}
