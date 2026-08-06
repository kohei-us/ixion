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

struct IXION_DLLPUBLIC table_t
{
    std::string_view name;
    std::string_view column_first;
    std::string_view column_last;
    table_areas_t areas;

    table_t();

    bool operator== (const table_t& r) const;
};

IXION_DLLPUBLIC std::ostream& operator<<(std::ostream& os, const table_t& table);

/**
 * Stores the data of a single table.  A table is a 2-dimensional range of
 * cells with named columns, whose range may include a header row at the top
 * and one or more totals rows at the bottom.  A formula expression may
 * reference parts of a table via a table reference, represented by
 * ixion::table_t.
 */
struct IXION_DLLPUBLIC table
{
    /**
     * Name of the table.  It must be non-empty and unique within the model
     * the table belongs to.
     */
    std::string name;

    /**
     * Entire range of the table, including the header row and the totals
     * rows if present.  It must be a valid range that does not span
     * multiple sheets.
     */
    abs_range_t range;

    /** Names of the columns of the table in column order. */
    std::vector<std::string> columns;

    /** Number of totals rows at the bottom of the table range. */
    row_t totals_row_count;

    table();
    table(const table& other);
    table(table&& other);
    ~table();

    table& operator=(const table& other);
    table& operator=(table&& other);

    bool operator==(const table& r) const;
};

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
