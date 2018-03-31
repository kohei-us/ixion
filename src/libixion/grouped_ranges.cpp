/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "grouped_ranges.hpp"

#include "ixion/global.hpp"
#include "ixion/model_context.hpp"

namespace ixion {

grouped_range_error::grouped_range_error(const std::string& msg) : general_error(msg) {}
grouped_range_error::~grouped_range_error() throw() {}

grouped_ranges::sheet_type::sheet_type(rc_size_t ss) :
    rows(0, ss.row, 0), columns(0, ss.column, 0) {}

void grouped_ranges::sheet_type::build_trees() const
{
    if (!rows.is_tree_valid())
        rows.build_tree();

    if (!columns.is_tree_valid())
        columns.build_tree();
}

grouped_ranges::grouped_ranges(const model_context& cxt) :
    m_cxt(cxt)
{
}

grouped_ranges::~grouped_ranges()
{
}

void grouped_ranges::add(sheet_t sheet, const abs_rc_range_t& range, uintptr_t identifier)
{
    sheet_type& store = fetch_sheet_store(sheet);
    store.rows.insert_back(range.first.row, range.last.row+1, identifier);
    store.columns.insert_back(range.first.column, range.last.column+1, identifier);
}

uintptr_t grouped_ranges::remove(sheet_t sheet, const abs_rc_range_t& range)
{
    if (size_t(sheet) >= m_sheets.size())
        throw grouped_range_error("No grouped ranges stored on specified sheet.");

    sheet_type& store = *m_sheets[sheet];
    store.build_trees();

    uintptr_t row_identifier = 0;
    rc_t row_start = -1, row_end = -1;
    auto row_res = store.rows.search_tree(range.first.row, row_identifier, &row_start, &row_end);
    if (!row_res.second)
        throw grouped_range_error("Row search was not successful.");

    uintptr_t col_identifier = 0;
    rc_t col_start = -1, col_end = -1;
    auto col_res = store.columns.search_tree(range.first.column, col_identifier, &col_start, &col_end);
    if (!col_res.second)
        throw grouped_range_error("Column search was not successful.");

    if (row_start != range.first.row || row_end != range.last.row+1)
        throw grouped_range_error("Stored row range does not match the specified row range.");

    if (col_start != range.first.column || col_end != range.last.column+1)
        throw grouped_range_error("Stored column range does not match the specified column range.");

    if (row_identifier != col_identifier)
        throw grouped_range_error("Stored identifiers do not match between row and column ranges.");

    store.rows.insert(row_res.first, range.first.row, range.last.row+1, 0);
    store.columns.insert(col_res.first, range.first.column, range.last.column+1, 0);

    return row_identifier;
}

abs_rc_address_t grouped_ranges::move_to_origin(sheet_t sheet, const abs_rc_address_t& pos) const
{
    if (size_t(sheet) >= m_sheets.size())
        return pos;

    const sheet_type& store = *m_sheets[sheet];
    store.build_trees();

    uintptr_t row_identifier = 0;
    uintptr_t col_identifier = 0;

    rc_t row_start = -1;
    rc_t col_start = -1;

    auto row_res = store.rows.search_tree(pos.row, row_identifier, &row_start);
    if (!row_res.second)
        return pos;

    auto col_res = store.columns.search_tree(pos.column, col_identifier, &col_start);
    if (!col_res.second)
        return pos;

    if (row_identifier != col_identifier)
        return pos;

    return abs_rc_address_t(row_start, col_start);
}

grouped_ranges::sheet_type& grouped_ranges::fetch_sheet_store(sheet_t sheet)
{
    if (size_t(sheet) < m_sheets.size())
        return *m_sheets[sheet];

    // Add enough sheet stores so that the specified sheet index becomes
    // within range.

    size_t new_size = sheet + 1;
    size_t added = new_size - m_sheets.size();
    m_sheets.reserve(new_size);

    for (size_t i = 0; i < added; ++i)
    {
        sheet_t sheet_to_add = m_sheets.size();
        m_sheets.emplace_back(
            ixion::make_unique<sheet_type>(
                m_cxt.get_sheet_size(sheet_to_add)));
    }

    assert(size_t(sheet) < m_sheets.size());
    return *m_sheets[sheet];
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
