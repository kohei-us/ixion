/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef IXION_INTERFACE_TABLE_HANDLER_HPP
#define IXION_INTERFACE_TABLE_HANDLER_HPP

#include "../types.hpp"

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
     * @param pos current cell position.
     * @param column_first name of the starting column within the table.
     * @param column_last name of the ending column within the table, or
     *                    empty_string_id if it's a single column.
     * @param areas area specifiter value, which may consist of one or more
     *              values of table_area_t.
     *
     * @return referenced data range.
     */
    virtual abs_range_t get_range(
        const abs_address_t& pos, string_id_t column_first, string_id_t column_last,
        table_areas_t areas) const = 0;

    /**
     * Get the data range associated with given table and column names.
     *
     * @param table string identifier representing the table name.
     * @param column_first name of the starting column within the table.
     * @param column_last name of the ending column within the table, or
     *                    empty_string_id if it's a single column.
     * @param areas area specifiter value, which may consist of one or more
     *              values of table_area_t.
     *
     * @return referenced data range.
     */
    virtual abs_range_t get_range(
        string_id_t table, string_id_t column_first, string_id_t column_last,
        table_areas_t areas) const = 0;
};

}}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
