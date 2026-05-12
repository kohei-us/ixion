/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once
#include "../types.hpp"

#include <string_view>

namespace ixion {

struct abs_address_t;
struct abs_range_t;

namespace iface {

class IXION_DLLPUBLIC table_handler
{
public:
    virtual ~table_handler();

    /**
     * Get the data range associated with a given column name.  The current
     * position is used to infer which table to use.
     *
     * The string views passed in are valid only for the duration of the
     * call; implementations must copy if they need to retain them.
     *
     * @param pos current cell position.
     * @param column_first name of the starting column within the table.
     * @param column_last name of the ending column within the table, or
     *                    an empty view if it's a single column.
     * @param areas area specifier value, which may consist of one or more
     *              values of table_area_t.
     *
     * @return referenced data range.
     */
    virtual abs_range_t get_range(
        const abs_address_t& pos, std::string_view column_first, std::string_view column_last,
        table_areas_t areas) const = 0;

    /**
     * Get the data range associated with given table and column names.
     *
     * The string views passed in are valid only for the duration of the
     * call; implementations must copy if they need to retain them.
     *
     * @param table name of the table.
     * @param column_first name of the starting column within the table.
     * @param column_last name of the ending column within the table, or
     *                    an empty view if it's a single column.
     * @param areas area specifier value, which may consist of one or more
     *              values of table_area_t.
     *
     * @return referenced data range.
     */
    virtual abs_range_t get_range(
        std::string_view table, std::string_view column_first, std::string_view column_last,
        table_areas_t areas) const = 0;
};

}}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
