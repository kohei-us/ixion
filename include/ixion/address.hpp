/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once
#include "types.hpp"

#include <compare>
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
    /** Tag type for constructing an invalid address. */
    enum init_invalid { invalid };

    /** 0-based sheet index. */
    sheet_t sheet;
    /** 0-based row position. */
    row_t   row;
    /** 0-based column position. */
    col_t   column;

    /** Construct the address of the top-left cell of the first sheet. */
    abs_address_t();
    /** Construct an invalid address, with all components set to -1. */
    abs_address_t(init_invalid);
    /** Construct an address from its components. */
    abs_address_t(sheet_t _sheet, row_t _row, col_t _column);
    /** Copy constructor. */
    abs_address_t(const abs_address_t& r);

    /**
     * Check whether the address is valid, that is, no component is negative
     * and the row and column positions are within their upper bounds.
     */
    bool valid() const;

    /**
     * Get a string representation of the address, for debugging.
     */
    ::std::string get_name() const;

    /** Order addresses by sheet, then row, then column. */
    std::strong_ordering operator<=>(const abs_address_t& r) const;
    /** Compare all the components of two addresses. */
    bool operator==(const abs_address_t& r) const;

    /** Hash function for use in unordered containers. */
    struct hash
    {
        /** Compute the hash value of an address. */
        IXION_DLLPUBLIC size_t operator() (const abs_address_t& addr) const;
    };
};

/**
 * Stores either absolute or relative address.  Each component is absolute
 * or relative on its own.  An absolute component is a position, whereas a
 * relative component is an offset from the position of the cell the address
 * is used in.
 */
struct IXION_DLLPUBLIC address_t
{
    /** Sheet index, or sheet offset when relative. */
    sheet_t sheet;
    /** Row position, or row offset when relative. */
    row_t   row;
    /** Column position, or column offset when relative. */
    col_t   column;
    /** Whether the sheet component is absolute. */
    bool    abs_sheet:1;
    /** Whether the row component is absolute. */
    bool    abs_row:1;
    /** Whether the column component is absolute. */
    bool    abs_column:1;

    /** Construct the absolute address of the top-left cell of the first sheet. */
    address_t();
    /** Construct an address from its components and their absolute flags. */
    address_t(sheet_t _sheet, row_t _row, col_t _column,
              bool _abs_sheet=true, bool _abs_row=true, bool _abs_column=true);
    /** Copy constructor. */
    address_t(const address_t& r);
    /** Construct an address with all components absolute. */
    address_t(const abs_address_t& r);

    /**
     * Check whether the address is valid.  An absolute sheet component must
     * not be negative, and the row and column components must be within
     * their upper bounds.
     */
    bool valid() const;

    /**
     * Convert the address to an absolute address.  Each relative component
     * gets added to the corresponding component of the origin.
     *
     * @param origin Position of the cell the address is used in.
     *
     * @return Absolute address.
     */
    abs_address_t to_abs(const abs_address_t& origin) const;

    /**
     * Get a string representation of the address, for debugging.
     */
    ::std::string get_name() const;

    /** Set all three components absolute or relative. */
    void set_absolute(bool abs);

    /** Order addresses by their components and flags, in declaration order. */
    std::strong_ordering operator<=>(const address_t& r) const;
    /** Compare all the components and flags of two addresses. */
    bool operator==(const address_t& r) const;

    /** Hash function for use in unordered containers. */
    struct hash
    {
        /** Compute the hash value of an address. */
        IXION_DLLPUBLIC size_t operator() (const address_t& addr) const;
    };
};

/**
 * Stores absolute address, and absolute address only, but unlike the
 * abs_address_t counterpart, this struct only stores row and column
 * positions.  The "rc" in the name stands for row and column.
 */
struct IXION_DLLPUBLIC abs_rc_address_t
{
    /** Tag type for constructing an invalid address. */
    enum init_invalid { invalid };

    /** 0-based row position. */
    row_t row;
    /** 0-based column position. */
    col_t column;

    /** Construct the address of the top-left cell. */
    abs_rc_address_t();
    /** Construct an invalid address, with both components set to -1. */
    abs_rc_address_t(init_invalid);
    /** Construct an address from its components. */
    abs_rc_address_t(row_t _row, col_t _column);
    /** Copy constructor. */
    abs_rc_address_t(const abs_rc_address_t& r);
    /** Construct an address from the row and column of an address with a sheet. */
    abs_rc_address_t(const abs_address_t& r);

    /**
     * Check whether the address is valid, that is, neither component is
     * negative and both are within their upper bounds.
     */
    bool valid() const;

    /** Order addresses by row, then column. */
    std::strong_ordering operator<=>(const abs_rc_address_t& r) const;
    /** Compare both components of two addresses. */
    bool operator==(const abs_rc_address_t& r) const;

    /** Hash function for use in unordered containers. */
    struct hash
    {
        /** Compute the hash value of an address. */
        IXION_DLLPUBLIC size_t operator() (const abs_rc_address_t& addr) const;
    };
};

/**
 * Stores either absolute or relative address, but unlike the address_t
 * counterpart, this struct only stores row and column positions.  The "rc"
 * in the name stands for row and column.
 */
struct IXION_DLLPUBLIC rc_address_t
{
    /** Row position, or row offset when relative. */
    row_t row;
    /** Column position, or column offset when relative. */
    col_t column;
    /** Whether the row component is absolute. */
    bool  abs_row:1;
    /** Whether the column component is absolute. */
    bool  abs_column:1;

    /** Construct the absolute address of the top-left cell. */
    rc_address_t();
    /** Construct an address from its components and their absolute flags. */
    rc_address_t(row_t _row, col_t _column, bool _abs_row=true, bool _abs_column=true);
    /** Copy constructor. */
    rc_address_t(const rc_address_t& r);
    /** Construct an address with both components absolute. */
    rc_address_t(const abs_rc_address_t& r);

    /** Order addresses by their components and flags, in declaration order. */
    std::strong_ordering operator<=>(const rc_address_t& r) const;
    /** Compare both components and flags of two addresses. */
    bool operator==(const rc_address_t& r) const;

    /** Hash function for use in unordered containers. */
    struct hash
    {
        /** Compute the hash value of an address. */
        IXION_DLLPUBLIC size_t operator() (const rc_address_t& addr) const;
    };
};

struct abs_rc_range_t;

/**
 * Stores absolute range address.
 */
struct IXION_DLLPUBLIC abs_range_t
{
    /** Tag type for constructing an invalid range. */
    enum init_invalid { invalid };

    /** Position of the top-left cell of the range. */
    abs_address_t first;
    /** Position of the bottom-right cell of the range. */
    abs_address_t last;

    /** Construct a range covering the top-left cell of the first sheet. */
    abs_range_t();
    /** Construct an invalid range, with both positions invalid. */
    abs_range_t(init_invalid);
    /** Construct a range covering a single cell. */
    abs_range_t(sheet_t _sheet, row_t _row, col_t _col);

    /**
     * Construct a range from its top-left cell and its size.
     *
     * @param _sheet 0-based sheet index.
     * @param _row 0-based row position of the top-left cell of the range.
     * @param _col 0-based column position of the top-left cell of the range.
     * @param _row_span row length of the range. It must be 1 or greater.
     * @param _col_span column length of the range.  It must be 1 or greater.
     */
    abs_range_t(sheet_t _sheet, row_t _row, col_t _col, row_t _row_span, col_t _col_span);
    /** Construct a range covering a single cell. */
    abs_range_t(const abs_address_t& addr);

    /**
     * Construct a range from its top-left cell and its size.
     *
     * @param addr Position of the top-left cell of the range.
     * @param row_span Row length of the range.  A value less than 1 counts
     *                 as 1.
     * @param col_span Column length of the range.  A value less than 1
     *                 counts as 1.
     */
    abs_range_t(const abs_address_t& addr, row_t row_span, col_t col_span);

    /**
     * Construct a range from its top-left and bottom-right cells.
     *
     * @param _first Position of the top-left cell of the range.
     * @param _last Position of the bottom-right cell of the range.  It must
     *              not precede the first position either row-wise or
     *              column-wise.
     */
    abs_range_t(const abs_address_t& _first, const abs_address_t& _last);

    /**
     * Construct a range from a sheet index and a range without a sheet.
     *
     * @param _sheet 0-based index of the sheet the range is on.
     * @param _range Range on the sheet, without a sheet index of its own.
     */
    abs_range_t(sheet_t _sheet, const abs_rc_range_t& _range);

    /** Hash function for use in unordered containers. */
    struct hash
    {
        /** Compute the hash value of a range. */
        IXION_DLLPUBLIC size_t operator() (const abs_range_t& range) const;
    };

    /**
     * Check whether the range is valid, that is, both positions are valid
     * and the first position does not follow the last one in any direction.
     */
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

    /**
     * Reorder range values as needed to ensure the range is valid.
     */
    void reorder();

    /** Order ranges by their first position, then their last position. */
    std::strong_ordering operator<=>(const abs_range_t& r) const;
    /** Compare both positions of two ranges. */
    bool operator==(const abs_range_t& r) const;
};

/**
 * Stores absolute range address, but unlike the abs_range_t counterpart,
 * this struct only stores row and column positions.  The "rc" in the name
 * stands for row and column.
 */
struct IXION_DLLPUBLIC abs_rc_range_t
{
    /** Tag type for constructing an invalid range. */
    enum init_invalid { invalid };

    /** Position of the top-left cell of the range. */
    abs_rc_address_t first;
    /** Position of the bottom-right cell of the range. */
    abs_rc_address_t last;

    /** Construct a range covering the top-left cell. */
    abs_rc_range_t();
    /** Construct an invalid range, with both positions invalid. */
    abs_rc_range_t(init_invalid);
    /** Copy constructor. */
    abs_rc_range_t(const abs_rc_range_t& other);
    /** Construct a range from a range with a sheet, dropping the sheet. */
    explicit abs_rc_range_t(const abs_range_t& other);

    /**
     * Construct a range from its top-left cell and its size.
     *
     * @param _row 0-based row position of the top-left cell of the range.
     * @param _col 0-based column position of the top-left cell of the range.
     * @param _row_span Row length of the range.  It must be 1 or greater.
     * @param _col_span Column length of the range.  It must be 1 or greater.
     */
    abs_rc_range_t(row_t _row, col_t _col, row_t _row_span, col_t _col_span);

    /** Hash function for use in unordered containers. */
    struct hash
    {
        /** Compute the hash value of a range. */
        IXION_DLLPUBLIC size_t operator() (const abs_rc_range_t& range) const;
    };

    /**
     * Check whether the range is valid, that is, both positions are valid
     * and the first position does not follow the last one in any direction.
     */
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

    /** Order ranges by their first position, then their last position. */
    std::strong_ordering operator<=>(const abs_rc_range_t& r) const;
    /** Compare both positions of two ranges. */
    bool operator==(const abs_rc_range_t& r) const;
};

/**
 * Stores range whose component may be relative or absolute.
 */
struct IXION_DLLPUBLIC range_t
{
    /** Address of the top-left cell of the range. */
    address_t first;
    /** Address of the bottom-right cell of the range. */
    address_t last;

    /** Construct a range covering the top-left cell of the first sheet. */
    range_t();
    /** Construct a range from its top-left and bottom-right addresses. */
    range_t(const address_t& _first, const address_t& _last);
    /** Copy constructor. */
    range_t(const range_t& r);
    /** Construct a range with all components absolute. */
    range_t(const abs_range_t& r);

    /** Check whether both addresses of the range are valid. */
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
     * Convert the range to an absolute range.  Each relative component of
     * either address gets added to the corresponding component of the
     * origin.
     *
     * @param origin Position of the cell the range is used in.
     *
     * @return Absolute range.
     */
    abs_range_t to_abs(const abs_address_t& origin) const;

    /** Set all components of both addresses absolute or relative. */
    void set_absolute(bool abs);

    /** Hash function for use in unordered containers. */
    struct hash
    {
        /** Compute the hash value of a range. */
        IXION_DLLPUBLIC size_t operator() (const range_t& range) const;
    };
};

/** Compare both addresses of two ranges. */
IXION_DLLPUBLIC bool operator==(const range_t& left, const range_t& right);

/**
 * Stores range whose component may be relative or absolute, but unlike the
 * range_t counterpart, this struct only stores row and column positions.
 * The "rc" in the name stands for row and column.
 */
struct IXION_DLLPUBLIC rc_range_t
{
    /** Address of the top-left cell of the range. */
    rc_address_t first;
    /** Address of the bottom-right cell of the range. */
    rc_address_t last;

    /** Construct a range covering the top-left cell. */
    rc_range_t();
    /** Construct a range from its top-left and bottom-right addresses. */
    rc_range_t(const rc_address_t& _first, const rc_address_t& _last);
    /** Copy constructor. */
    rc_range_t(const rc_range_t& r);
    /** Construct a range with all components absolute. */
    rc_range_t(const abs_rc_range_t& r);

    /** Check whether the range is valid. */
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

    /** Hash function for use in unordered containers. */
    struct hash
    {
        /** Compute the hash value of a range. */
        IXION_DLLPUBLIC size_t operator() (const rc_range_t& range) const;
    };
};

/** Compare both addresses of two ranges. */
IXION_DLLPUBLIC bool operator==(const rc_range_t& left, const rc_range_t& right);

/** Print the components of an address or range, for debugging. */
IXION_DLLPUBLIC std::ostream& operator<<(std::ostream& os, const abs_address_t& addr);
/** @copydoc operator<<(std::ostream&, const abs_address_t&) */
IXION_DLLPUBLIC std::ostream& operator<<(std::ostream& os, const abs_rc_address_t& addr);
/** @copydoc operator<<(std::ostream&, const abs_address_t&) */
IXION_DLLPUBLIC std::ostream& operator<<(std::ostream& os, const address_t& addr);
/** @copydoc operator<<(std::ostream&, const abs_address_t&) */
IXION_DLLPUBLIC std::ostream& operator<<(std::ostream& os, const rc_address_t& addr);
/** @copydoc operator<<(std::ostream&, const abs_address_t&) */
IXION_DLLPUBLIC std::ostream& operator<<(std::ostream& os, const abs_range_t& range);
/** @copydoc operator<<(std::ostream&, const abs_address_t&) */
IXION_DLLPUBLIC std::ostream& operator<<(std::ostream& os, const abs_rc_range_t& range);
/** @copydoc operator<<(std::ostream&, const abs_address_t&) */
IXION_DLLPUBLIC std::ostream& operator<<(std::ostream& os, const range_t& range);
/** @copydoc operator<<(std::ostream&, const abs_address_t&) */
IXION_DLLPUBLIC std::ostream& operator<<(std::ostream& os, const rc_range_t& range);

/**
 * Type that represents a collection of multiple absolute cell addresses.
 */
using abs_address_set_t = std::unordered_set<abs_address_t, abs_address_t::hash>;
/** Type that represents a collection of multiple absolute ranges. */
using abs_range_set_t = std::unordered_set<abs_range_t, abs_range_t::hash>;
/** Type that represents a collection of multiple absolute ranges without sheets. */
using abs_rc_range_set_t = std::unordered_set<abs_rc_range_t, abs_rc_range_t::hash>;

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
