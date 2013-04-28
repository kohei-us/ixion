/*************************************************************************
 *
 * Copyright (c) 2012 Kohei Yoshida
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 ************************************************************************/

#include "ixion/global.hpp"

#include "workbook.hpp"

namespace ixion {

worksheet::worksheet() {}

worksheet::worksheet(size_t row_size, size_t col_size)
{
    m_columns.reserve(col_size);
    m_pos_hints.reserve(col_size);
    for (size_t i = 0; i < col_size; ++i)
    {
        m_columns.push_back(new column_store_t(row_size));
        m_pos_hints.push_back(m_columns.back()->begin());
    }
}

worksheet::~worksheet()
{
    std::for_each(m_columns.begin(), m_columns.end(), default_deleter<column_store_t>());
}

workbook::workbook() {}

workbook::workbook(size_t sheet_size, size_t row_size, size_t col_size)
{
    for (size_t i = 0; i < sheet_size; ++i)
        m_sheets.push_back(new worksheet(row_size, col_size));
}

workbook::~workbook()
{
    std::for_each(m_sheets.begin(), m_sheets.end(), default_deleter<worksheet>());
}

void workbook::push_back(size_t row_size, size_t col_size)
{
    m_sheets.push_back(new worksheet(row_size, col_size));
}

}
