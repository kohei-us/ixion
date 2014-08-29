/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "table_handler.hpp"

namespace ixion {

table_handler::entry::entry() :
    name(empty_string_id), range(abs_range_t::invalid), totals_row_count(0) {}

table_handler::~table_handler() {}

abs_range_t table_handler::get_range(
    const abs_address_t& pos, string_id_t column_first, string_id_t column_last,
    table_areas_t areas) const
{
    entries_type::const_iterator it = m_entries.begin(), it_end = m_entries.end();
    for (; it != it_end; ++it)
    {
        const entry& e = *it->second;
        if (!e.range.contains(pos))
            continue;

        return get_column_range(e, column_first, column_last, areas);
    }

    return abs_range_t(abs_range_t::invalid);
}

abs_range_t table_handler::get_range(
    string_id_t table, string_id_t column_first, string_id_t column_last,
    table_areas_t areas) const
{
    entries_type::const_iterator it = m_entries.find(table);
    if (it == m_entries.end())
        // Table name not found.
        return abs_range_t(abs_range_t::invalid);

    const entry& e = *it->second;
    return get_column_range(e, column_first, column_last, areas);
}

void table_handler::insert(entry* p)
{
    if (!p)
        return;

    unique_ptr<entry> px(p);
    string_id_t name = p->name;
    m_entries.insert(name, px.release());
}

abs_range_t table_handler::get_column_range(
    const entry& e, string_id_t column_first, string_id_t column_last, table_areas_t areas) const
{
    if (column_first == empty_string_id)
    {
        // Area specifiers only.
        bool headers = (areas & table_area_headers);
        bool data = (areas & table_area_data);
        bool totals = (areas & table_area_totals);

        if (headers)
        {
            if (data)
            {
                if (totals)
                {
                    // All areas.
                    return e.range;
                }

                // Headers + data areas
                abs_range_t ret = e.range;
                ret.last.row -= e.totals_row_count;
                return ret;
            }

            // Headers only.
            abs_range_t ret = e.range;
            ret.last.row = ret.first.row;
            return ret;
        }

        abs_range_t ret = e.range;
        --ret.first.row;

        if (data)
        {
            if (totals)
            {
                // Data + totals areas
                return ret;
            }

            // Data area only.
            ret.last.row -= e.totals_row_count;
            return ret;
        }

        // Totals area only.
        if (e.totals_row_count <= 0)
            return abs_range_t(abs_range_t::invalid);

        ret.first.row = ret.last.row;
        ret.first.row -= e.totals_row_count - 1;
        return ret;
    }

    for (size_t i = 0, n = e.columns.size(); i < n; ++i)
    {
        if (e.columns[i] == column_first)
        {
            // Matching column name found.
            abs_range_t ret = e.range;
            col_t col = e.range.first.column + i;
            ret.first.column = col;
            ret.first.row += 1;
            ret.last.column = col;
            ret.last.row -= 1;
            return ret;
        }
    }

    return abs_range_t(abs_range_t::invalid);
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
