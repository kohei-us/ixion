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

#ifndef __IXION_ADDRESS_HPP__
#define __IXION_ADDRESS_HPP__

#include "global.hpp"

#include <string>

namespace ixion {

/**
 * Stores absolute address, and absolute address only.
 */
struct abs_address_t
{
    sheet_t sheet;
    row_t   row;
    col_t   column;
    abs_address_t();
    abs_address_t(sheet_t _sheet, row_t _row, col_t _column);
    abs_address_t(const abs_address_t& r);
};

struct address_t
{
    sheet_t sheet;
    row_t   row;
    col_t   column;
    bool    abs_sheet:1;
    bool    abs_row:1;
    bool    abs_column:1;

    address_t();
    address_t(sheet_t _sheet, row_t _row, col_t _column, 
              bool _abs_sheet=true, bool _abs_row=true, bool _abs_column=true);
    address_t(const address_t& addr);

    address_t to_abs(const address_t& origin) const;
    ::std::string get_name() const;

    struct hash
    {
        size_t operator() (const address_t& addr) const;
    };
};

bool operator==(const address_t& left, const address_t& right);
bool operator<(const address_t& left, const address_t& right);

}

#endif
