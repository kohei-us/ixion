/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/global.hpp"

#include "workbook.hpp"

namespace ixion {

worksheet::worksheet() {}

worksheet::worksheet(size_t row_size, size_t col_size)
{
    m_pos_hints.reserve(col_size);
    for (size_t i = 0; i < col_size; ++i)
    {
        m_columns.emplace_back(row_size);
        m_pos_hints.push_back(m_columns.back().begin());
    }
}

worksheet::~worksheet() {}

workbook::workbook() {}

workbook::workbook(size_t sheet_size, size_t row_size, size_t col_size)
{
    for (size_t i = 0; i < sheet_size; ++i)
        m_sheets.emplace_back(row_size, col_size);
}

workbook::~workbook() {}

void workbook::push_back(size_t row_size, size_t col_size)
{
    m_sheets.emplace_back(row_size, col_size);
}

size_t workbook::size() const
{
    return m_sheets.size();
}

bool workbook::empty() const
{
    return m_sheets.empty();
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
