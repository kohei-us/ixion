/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_ADDRESS_HPP
#define INCLUDED_IXION_ADDRESS_HPP

#include "ixion/global.hpp"

#include <string>
#include <vector>
#include <ostream>
#include <unordered_set>

namespace ixion {

/**
 * Row address not specified. This is used to reference an entire column
 * when a specific column address is given.
 */
IXION_DLLPUBLIC_VAR const row_t row_unset;

/**
 * Highest number that can be used to reference a row address. Numbers
 * higher than this number are all used as special indices.
 */
IXION_DLLPUBLIC_VAR const row_t row_upper_bound;

/**
 * Column address not specified. This is used to reference an entire row
 * when a specific row address is given.
 */
IXION_DLLPUBLIC_VAR const col_t column_unset;

/**
 * Highest number that can be used to reference a column address. Numbers
 * higher than this number are all used as special indices.
 */
IXION_DLLPUBLIC_VAR const col_t column_upper_bound;

/**
 * Stores absolute address, and absolute address only.
 */
struct IXION_DLLPUBLIC abs_address_t
{
    enum init_invalid { invalid };

    sheet_t sheet;
    row_t   row;
    col_t   column;

    abs_address_t();
    abs_address_t(init_invalid);
    abs_address_t(sheet_t _sheet, row_t _row, col_t _column);
    abs_address_t(const abs_address_t& r);

    bool valid() const;
    ::std::string get_name() const;

    struct hash
    {
        IXION_DLLPUBLIC size_t operator() (const abs_address_t& addr) const;
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

    bool valid() const;
    abs_address_t to_abs(const abs_address_t& origin) const;
    ::std::string get_name() const;

    void set_absolute(bool abs);

    struct hash
    {
        IXION_DLLPUBLIC size_t operator() (const address_t& addr) const;
    };
};

IXION_DLLPUBLIC bool operator==(const address_t& left, const address_t& right);
IXION_DLLPUBLIC bool operator!=(const address_t& left, const address_t& right);
IXION_DLLPUBLIC bool operator<(const address_t& left, const address_t& right);

struct IXION_DLLPUBLIC abs_rc_address_t
{
    enum init_invalid { invalid };

    row_t row;
    col_t column;

    abs_rc_address_t();
    abs_rc_address_t(init_invalid);
    abs_rc_address_t(row_t _row, col_t _column);
    abs_rc_address_t(const abs_rc_address_t& r);

    bool valid() const;

    struct hash
    {
        IXION_DLLPUBLIC size_t operator() (const abs_rc_address_t& addr) const;
    };
};

IXION_DLLPUBLIC bool operator==(const abs_rc_address_t& left, const abs_rc_address_t& right);
IXION_DLLPUBLIC bool operator!=(const abs_rc_address_t& left, const abs_rc_address_t& right);
IXION_DLLPUBLIC bool operator<(const abs_rc_address_t& left, const abs_rc_address_t& right);

/**
 * Stores either absolute or relative address, but unlike the {@link
 * address_t} counterpart, this struct only stores row and column positions.
 */
struct IXION_DLLPUBLIC rc_address_t
{
    row_t row;
    col_t column;
    bool  abs_row:1;
    bool  abs_column:1;

    rc_address_t();
    rc_address_t(row_t _row, col_t _column, bool _abs_row=true, bool _abs_column=true);
    rc_address_t(const rc_address_t& r);

    struct hash
    {
        IXION_DLLPUBLIC size_t operator() (const rc_address_t& addr) const;
    };
};

/**
 * Stores absolute range address.
 */
struct IXION_DLLPUBLIC abs_range_t
{
    enum init_invalid { invalid };

    abs_address_t first;
    abs_address_t last;

    abs_range_t();
    abs_range_t(init_invalid);
    abs_range_t(sheet_t _sheet, row_t _row, col_t _col);

    /**
     * @param _sheet 0-based sheet index.
     * @param _row 0-based row position of the top-left cell of the range.
     * @param _col 0-based column position of the top-left cell of the range.
     * @param _row_span row length of the range. It must be 1 or greater.
     * @param _col_span column length of the range.  It must be 1 or greater.
     */
    abs_range_t(sheet_t _sheet, row_t _row, col_t _col, row_t _row_span, col_t _col_span);
    abs_range_t(const abs_address_t& addr);

    struct hash
    {
        IXION_DLLPUBLIC size_t operator() (const abs_range_t& range) const;
    };

    bool valid() const;

    /**
     * Expand the range horizontally to include all columns.  The row range
     * will remain unchanged.
     */
    void set_all_columns();

    /**
     * Expand the range vertically to include all rows.  The column range will
     * remain unchanged.
     */
    void set_all_rows();

    /**
     * @return true if the range is unspecified in the horizontal direction
     *         i.e. all columns are selected, false otherwise.
     */
    bool all_columns() const;

    /**
     * @return true if the range is unspecified in the vertical direction i.e.
     *         all rows are selected, false otherwise.
     */
    bool all_rows() const;

    /**
     * Check whether or not a given address is contained within this range.
     */
    bool contains(const abs_address_t& addr) const;
};

IXION_DLLPUBLIC bool operator==(const abs_range_t& left, const abs_range_t& right);
IXION_DLLPUBLIC bool operator!=(const abs_range_t& left, const abs_range_t& right);
IXION_DLLPUBLIC bool operator<(const abs_range_t& left, const abs_range_t& right);

struct IXION_DLLPUBLIC abs_rc_range_t
{
    enum init_invalid { invalid };

    abs_rc_address_t first;
    abs_rc_address_t last;

    abs_rc_range_t();
    abs_rc_range_t(init_invalid);

    struct hash
    {
        IXION_DLLPUBLIC size_t operator() (const abs_rc_range_t& range) const;
    };

    bool valid() const;

    /**
     * Expand the range horizontally to include all columns.  The row range
     * will remain unchanged.
     */
    void set_all_columns();

    /**
     * Expand the range vertically to include all rows.  The column range will
     * remain unchanged.
     */
    void set_all_rows();

    /**
     * @return true if the range is unspecified in the horizontal direction
     *         i.e. all columns are selected, false otherwise.
     */
    bool all_columns() const;

    /**
     * @return true if the range is unspecified in the vertical direction i.e.
     *         all rows are selected, false otherwise.
     */
    bool all_rows() const;

    /**
     * Check whether or not a given address is contained within this range.
     */
    bool contains(const abs_rc_address_t& addr) const;
};

IXION_DLLPUBLIC bool operator==(const abs_rc_range_t& left, const abs_rc_range_t& right);
IXION_DLLPUBLIC bool operator!=(const abs_rc_range_t& left, const abs_rc_range_t& right);
IXION_DLLPUBLIC bool operator<(const abs_rc_range_t& left, const abs_rc_range_t& right);

/**
 * Stores range whose component may be relative or absolute.
 */
struct IXION_DLLPUBLIC range_t
{
    address_t first;
    address_t last;

    range_t();
    range_t(const address_t& _first, const address_t& _last);
    range_t(const range_t& r);
    range_t(const abs_range_t& r);

    bool valid() const;

    /**
     * Expand the range horizontally to include all columns.  The row range
     * will remain unchanged.
     */
    void set_all_columns();

    /**
     * Expand the range vertically to include all rows.  The column range will
     * remain unchanged.
     */
    void set_all_rows();

    /**
     * @return true if the range is unspecified in the horizontal direction
     *         i.e. all columns are selected, false otherwise.
     */
    bool all_columns() const;

    /**
     * @return true if the range is unspecified in the vertical direction i.e.
     *         all rows are selected, false otherwise.
     */
    bool all_rows() const;

    abs_range_t to_abs(const abs_address_t& origin) const;

    void set_absolute(bool abs);

    struct hash
    {
        IXION_DLLPUBLIC size_t operator() (const range_t& range) const;
    };
};

IXION_DLLPUBLIC bool operator==(const range_t& left, const range_t& right);
IXION_DLLPUBLIC bool operator!=(const range_t& left, const range_t& right);

IXION_DLLPUBLIC std::ostream& operator<<(std::ostream& os, const abs_address_t& addr);
IXION_DLLPUBLIC std::ostream& operator<<(std::ostream& os, const address_t& addr);
IXION_DLLPUBLIC std::ostream& operator<<(std::ostream& os, const abs_range_t& range);
IXION_DLLPUBLIC std::ostream& operator<<(std::ostream& os, const range_t& range);

/**
 * Type that represents a collection of multiple cell addresses.
 */
using abs_address_set_t = std::unordered_set<abs_address_t, abs_address_t::hash>;
using abs_range_set_t = std::unordered_set<abs_range_t, abs_range_t::hash>;
using abs_rc_range_set_t = std::unordered_set<abs_rc_range_t, abs_rc_range_t::hash>;

}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
