/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <ixion/global.hpp>
#include "sheet_store.hpp"

namespace ixion { namespace detail {

sheet_store::sheet_store() = default;

sheet_store::sheet_store(sheet_store&& other) = default;

sheet_store::sheet_store(size_t row_size, size_t col_size)
{
    for (size_t i = 0; i < col_size; ++i)
        m_columns.emplace_back(row_size);

    m_pos_hints.resize(col_size); // default-constructed hints
}

sheet_store::~sheet_store() = default;

abs_rc_range_t sheet_store::get_data_range() const
{
    size_t col_size = m_columns.size();
    if (!col_size)
        return abs_rc_range_t(abs_rc_range_t::invalid);

    row_t row_size = m_columns[0].size();
    if (!row_size)
        return abs_rc_range_t(abs_rc_range_t::invalid);

    abs_rc_range_t range;
    range.first.column = 0;
    range.first.row = row_size-1;
    range.last.column = -1; // if this stays -1 all columns are empty.
    range.last.row = 0;

    for (size_t i = 0; i < col_size; ++i)
    {
        const column_store_t& col = m_columns[i];
        if (col.empty())
        {
            if (range.last.column < 0)
                ++range.first.column;
            continue;
        }

        if (range.first.row > 0)
        {
            // First non-empty row.

            column_store_t::const_iterator it = col.begin(), it_end = col.end();
            assert(it != it_end);
            if (it->type == element_type_empty)
            {
                // First block is empty.
                row_t offset = it->size;
                ++it;
                if (it == it_end)
                {
                    // The whole column is empty.
                    if (range.last.column < 0)
                        ++range.first.column;
                    continue;
                }

                assert(it->type != element_type_empty);
                if (range.first.row > offset)
                    range.first.row = offset;
            }
            else
                // Set the first row to 0, and lock it.
                range.first.row = 0;
        }

        if (range.last.row < (row_size-1))
        {
            // Last non-empty row.

            column_store_t::const_reverse_iterator it = col.rbegin(), it_end = col.rend();
            assert(it != it_end);
            if (it->type == element_type_empty)
            {
                // Last block is empty.
                size_t size_last_block = it->size;
                ++it;
                if (it == it_end)
                {
                    // The whole column is empty.
                    if (range.last.column < 0)
                        ++range.first.column;
                    continue;
                }

                assert(it->type != element_type_empty);
                row_t last_data_row = static_cast<row_t>(col.size() - size_last_block - 1);
                if (range.last.row < last_data_row)
                    range.last.row = last_data_row;
            }
            else
                // Last block is not empty.
                range.last.row = row_size - 1;
        }

        // Check if the column contains at least one non-empty cell.
        if (col.block_size() > 1 || !col.is_empty(0))
            range.last.column = i;
    }

    if (range.last.column < 0)
        // No data column found.  The whole sheet is empty.
        return abs_rc_range_t(abs_rc_range_t::invalid);

    return range;
}

sheet_store sheet_store::clone() const
{
    sheet_store cloned;

    for (const column_store_t& col : m_columns)
        cloned.m_columns.push_back(col.clone());

    cloned.m_pos_hints.resize(m_columns.size()); // default-constructed hints

    // named_expression_t is move-only; reconstruct each entry with a copy of
    // its tokens.
    for (const auto& [name, exp] : m_named_expressions)
        cloned.m_named_expressions.emplace(name, named_expression_t(exp.origin, exp.tokens));

    return cloned;
}

}}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
