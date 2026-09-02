/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "table_store.hpp"

#include <ixion/exceptions.hpp>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <optional>
#include <sstream>

namespace ixion { namespace detail {

namespace {

/**
 * Trim the row range of a table to the sub-range the area specifiers
 * select.  The range is set to invalid when the specified areas cannot
 * form a contiguous range or the table has no rows in the specified areas.
 */
void adjust_row_range(abs_rc_range_t& range, const table_t& tab, table_areas_t areas)
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

            // Headers + data
            range.last.row -= tab.totals_row_count;
            return;
        }

        if (totals)
        {
            // Header + total is invalid.
            range = abs_rc_range_t(abs_rc_range_t::invalid);
            return;
        }

        // Headers only.
        range.last.row = range.first.row;
        return;
    }

    if (data)
    {
        // Skip the header row.
        ++range.first.row;

        if (totals)
        {
            // Data + total
            return;
        }

        // Data only
        range.last.row -= tab.totals_row_count;
        return;
    }

    if (totals)
    {
        // Total only
        if (tab.totals_row_count <= 0)
        {
            // This table has no totals rows.
            range = abs_rc_range_t(abs_rc_range_t::invalid);
            return;
        }

        range.first.row = range.last.row - tab.totals_row_count + 1;
        return;
    }

    // No area specified.
    range = abs_rc_range_t(abs_rc_range_t::invalid);
}

/**
 * Find the 0-based position of a named column within a table, starting the
 * search at a given position.
 */
std::optional<std::size_t> find_column(const table_t& tab, std::string_view name, std::size_t offset)
{
    if (offset >= tab.columns.size())
        return {};

    auto it_beg = tab.columns.cbegin() + offset;
    auto it_end = tab.columns.cend();

    auto it = std::find_if(it_beg, it_end, [name](const std::string& col_name)
    {
        return col_name == name;
    });

    if (it == it_end)
        // not found.
        return {};

    return static_cast<std::size_t>(std::distance(tab.columns.cbegin(), it));
}

abs_rc_range_t get_range_from_table(
    const table_t& tab, std::string_view column_first, std::string_view column_last,
    table_areas_t areas)
{
    if (column_first.empty())
    {
        // Area specifiers only.  Use the whole table width.
        abs_rc_range_t range = tab.range;
        adjust_row_range(range, tab, areas);
        return range;
    }

    std::optional<std::size_t> col1_pos = find_column(tab, column_first, 0);
    if (!col1_pos)
        return abs_rc_range_t(abs_rc_range_t::invalid);

    abs_rc_range_t range = tab.range;
    range.first.column = range.last.column = tab.range.first.column + static_cast<col_t>(*col1_pos);

    if (!column_last.empty())
    {
        // column range table reference.  The second column must not
        // precede the first one.
        std::optional<std::size_t> col2_pos = find_column(tab, column_last, *col1_pos);
        if (!col2_pos)
            return abs_rc_range_t(abs_rc_range_t::invalid);

        range.last.column = tab.range.first.column + static_cast<col_t>(*col2_pos);
    }

    adjust_row_range(range, tab, areas);
    return range;
}

/**
 * Generate a table name unique both within the store and among the names
 * generated so far in the current batch.  The trailing digits of the
 * source name, if any, determine the initial counter value, mimicking the
 * way Excel names the tables of a copied sheet.
 */
std::string make_unique_name(
    std::string_view src_name, const table_store::store_type& stored,
    const table_store::cloned_tables_type& batch)
{
    std::size_t pos = src_name.size();
    while (pos > 0 && std::isdigit(static_cast<unsigned char>(src_name[pos-1])))
        --pos;

    std::string_view stem = src_name.substr(0, pos);
    std::string_view digits = src_name.substr(pos);

    std::size_t counter = 1;
    if (!digits.empty())
        std::from_chars(digits.data(), digits.data() + digits.size(), counter);

    for (++counter; true; ++counter)
    {
        std::string name = std::string{stem} + std::to_string(counter);

        if (stored.find(name) != stored.end())
            continue;

        auto it = std::find_if(batch.begin(), batch.end(), [&name](const auto& entry)
        {
            return entry.second.name == name;
        });

        if (it == batch.end())
            return name;
    }
}

} // anonymous namespace

void table_store::insert(table_t tab)
{
    if (tab.name.empty())
        throw std::invalid_argument("table name is empty");

    if (tab.sheet < 0)
    {
        std::ostringstream os;
        os << "table sheet index is invalid: " << tab.sheet;
        throw std::invalid_argument(os.str());
    }

    if (!tab.range.valid())
    {
        std::ostringstream os;
        os << "table range is invalid: " << tab.range;
        throw std::invalid_argument(os.str());
    }

    if (m_tables.find(tab.name) != m_tables.end())
    {
        std::ostringstream os;
        os << "table named '" << tab.name << "' already exists";
        throw model_context_error(os.str(), model_context_error::table_name_conflict);
    }

    std::string name = tab.name;
    m_tables.emplace(std::move(name), std::move(tab));
}

const table_t* table_store::get(std::string_view name) const
{
    auto it = m_tables.find(name);
    if (it == m_tables.end())
        return nullptr;

    return &it->second;
}

std::vector<const table_t*> table_store::get_by_sheet(sheet_t sheet) const
{
    std::vector<const table_t*> ret;

    for (const auto& [name, tab] : m_tables)
    {
        if (tab.sheet == sheet)
            ret.push_back(&tab);
    }

    return ret;
}

abs_range_t table_store::get_range(
    std::string_view name, std::string_view column_first,
    std::string_view column_last, table_areas_t areas) const
{
    if (name.empty())
        // no table name given.
        return abs_range_t(abs_range_t::invalid);

    auto it = m_tables.find(name);
    if (it == m_tables.end())
        // no table by this name found.
        return abs_range_t(abs_range_t::invalid);

    const table_t& tab = it->second;
    abs_rc_range_t range = get_range_from_table(tab, column_first, column_last, areas);
    if (!range.valid())
        return abs_range_t(abs_range_t::invalid);

    return abs_range_t(tab.sheet, range);
}

abs_range_t table_store::get_range(
    const abs_address_t& pos, std::string_view column_first,
    std::string_view column_last, table_areas_t areas) const
{
    for (const auto& [name, tab] : m_tables)
    {
        if (tab.sheet != pos.sheet)
            continue;

        if (!tab.range.contains(pos))
            continue;

        abs_rc_range_t range = get_range_from_table(tab, column_first, column_last, areas);
        if (!range.valid())
            return abs_range_t(abs_range_t::invalid);

        return abs_range_t(tab.sheet, range);
    }

    return abs_range_t(abs_range_t::invalid);
}

table_store::cloned_tables_type table_store::clone_sheet_tables(sheet_t src, sheet_t dst) const
{
    cloned_tables_type cloned;

    for (const auto& [name, tab] : m_tables)
    {
        if (tab.sheet != src)
            continue;

        table_t copied = tab;
        copied.name = make_unique_name(tab.name, m_tables, cloned);
        copied.sheet = dst;
        cloned.emplace_back(name, std::move(copied));
    }

    return cloned;
}

}}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
