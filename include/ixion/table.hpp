/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "types.hpp"
#include "address.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace ixion {

/**
 * Reference to a part of a table in a formula expression, such as
 * <code>Table1[[\#Headers],[Amount]]</code>, commonly known as a structured
 * reference in spreadsheet applications.  The string views point into the
 * storage of the formula token the reference belongs to.
 */
struct IXION_DLLPUBLIC table_ref_t
{
    /**
     * Name of the table.  It is empty when the reference does not name the
     * table, in which case the table is the one containing the referencing
     * cell.
     */
    std::string_view name;
    /**
     * Name of the first column of the reference.  It is empty when the
     * reference only contains area specifiers.
     */
    std::string_view column_first;
    /**
     * Name of the last column of the reference.  It is empty for a
     * single-column reference.
     */
    std::string_view column_last;
    /**
     * Areas of the table the reference covers, as a combination of
     * table_area_t flags.
     */
    table_areas_t areas;

    /** Construct an empty reference with no areas. */
    table_ref_t();

    /** Compare all the fields of two references. */
    bool operator== (const table_ref_t& r) const;
};

/** Print the fields of a table reference, for debugging. */
IXION_DLLPUBLIC std::ostream& operator<<(std::ostream& os, const table_ref_t& table);

/**
 * Stores the data of a single table.  A table is a 2-dimensional range of
 * cells with named columns, whose range may include a header row at the top
 * and one or more totals rows at the bottom.  A formula expression may
 * reference parts of a table via a table reference, represented by
 * ixion::table_ref_t.
 */
struct IXION_DLLPUBLIC table_t
{
    /**
     * Name of the table.  It must be non-empty and unique within the model
     * the table belongs to.
     */
    std::string name;

    /** 0-based index of the sheet the table is on. */
    sheet_t sheet;

    /**
     * Entire range of the table on its sheet, including the header row and
     * the totals rows if present.  It must be a valid range.
     */
    abs_rc_range_t range;

    /** Names of the columns of the table in column order. */
    std::vector<std::string> columns;

    /** Number of totals rows at the bottom of the table range. */
    row_t totals_row_count;

    /** Construct a table with no name, an invalid sheet and an invalid range. */
    table_t();
    /** Copy constructor. */
    table_t(const table_t& other);
    /** Move constructor. */
    table_t(table_t&& other);
    ~table_t();

    /** Copy assignment. */
    table_t& operator=(const table_t& other);
    /** Move assignment. */
    table_t& operator=(table_t&& other);

    /** Compare all the fields of two tables. */
    bool operator==(const table_t& r) const;
};

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
