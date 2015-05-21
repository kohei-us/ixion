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

    std::unique_ptr<entry> px(p);
    string_id_t name = p->name;
    m_entries.insert(name, px.release());
}

void adjust_table_area(abs_range_t& range, const table_handler::entry& e, table_areas_t areas)
{
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
                return;
            }

            // Headers + data areas
            range.last.row -= e.totals_row_count;
            return;
        }

        // Headers only.
        range.last.row = range.first.row;
        return;
    }

    // No header row.
    --range.first.row;

    if (data)
    {
        if (totals)
        {
            // Data + totals areas
            return;
        }

        // Data area only.
        range.last.row -= e.totals_row_count;
        return;
    }

    // Totals area only.
    if (e.totals_row_count <= 0)
    {
        range = abs_range_t(abs_range_t::invalid);
        return;
    }

    range.first.row = range.last.row;
    range.first.row -= e.totals_row_count - 1;
}

abs_range_t table_handler::get_column_range(
    const entry& e, string_id_t column_first, string_id_t column_last, table_areas_t areas) const
{
    if (column_first == empty_string_id)
    {
        // Area specifiers only.
        abs_range_t ret = e.range;
        adjust_table_area(ret, e, areas);
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
            ret.last.column = col;
            adjust_table_area(ret, e, areas);
            return ret;
        }
    }

    return abs_range_t(abs_range_t::invalid);
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
