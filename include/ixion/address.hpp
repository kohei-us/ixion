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

#include "ixion/global.hpp"
#include "ixion/hash_container/set.hpp"

#include <string>
#include <vector>
#include <set>

namespace ixion {

/**
 * Stores absolute address, and absolute address only.
 */
struct IXION_DLLPUBLIC abs_address_t
{
    sheet_t sheet;
    row_t   row;
    col_t   column;
    abs_address_t();
    abs_address_t(sheet_t _sheet, row_t _row, col_t _column);
    abs_address_t(const abs_address_t& r);

    ::std::string get_name() const;

    struct hash
    {
        size_t operator() (const abs_address_t& addr) const;
    };
};

IXION_DLLPUBLIC bool operator==(const abs_address_t& left, const abs_address_t& right);
IXION_DLLPUBLIC bool operator!=(const abs_address_t& left, const abs_address_t& right);
IXION_DLLPUBLIC bool operator<(const abs_address_t& left, const abs_address_t& right);

/**
 * Stores either absolute or relative address.
 */
struct IXION_DLLPUBLIC address_t
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
    address_t(const address_t& r);
    address_t(const abs_address_t& r);

    abs_address_t to_abs(const abs_address_t& origin) const;
    ::std::string get_name() const;

    struct hash
    {
        size_t operator() (const address_t& addr) const;
    };
};

IXION_DLLPUBLIC bool operator==(const address_t& left, const address_t& right);
IXION_DLLPUBLIC bool operator!=(const address_t& left, const address_t& right);
IXION_DLLPUBLIC bool operator<(const address_t& left, const address_t& right);

/**
 * Stores absolute range address.
 */
struct IXION_DLLPUBLIC abs_range_t
{
    abs_address_t first;
    abs_address_t last;

    abs_range_t();

    struct hash
    {
        size_t operator() (const abs_range_t& range) const;
    };

    /**
     * Check whether or not a given address is contained within this range.
     */
    bool contains(const abs_address_t& addr) const;
};

IXION_DLLPUBLIC bool operator==(const abs_range_t& left, const abs_range_t& right);
IXION_DLLPUBLIC bool operator!=(const abs_range_t& left, const abs_range_t& right);
IXION_DLLPUBLIC bool operator<(const abs_range_t& left, const abs_range_t& right);

/**
 * Stores range whose component may be relative or absolute.
 */
struct IXION_DLLPUBLIC range_t
{
    address_t first;
    address_t last;

    range_t();
    range_t(const address_t& _first, const address_t& _last);

    abs_range_t to_abs(const abs_address_t& origin) const;
};

bool operator==(const range_t& left, const range_t& right);
bool operator!=(const range_t& left, const range_t& right);

/**
 * Dirty cells are those formula cells that have been modified or formula
 * cells that reference other modified cells.
 */
typedef _ixion_unordered_set_type<abs_address_t, abs_address_t::hash> dirty_cells_t;
typedef std::vector<abs_address_t> dirty_cell_addrs_t;

}

#endif
