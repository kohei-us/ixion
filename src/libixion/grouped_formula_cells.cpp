/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "grouped_formula_cells.hpp"
#include "calc_status.hpp"
#include "ixion/formula_tokens.hpp"

namespace ixion {

size_t grouped_formula_cells::to_pos(row_t row, col_t col) const
{
    return row * m_cols + col;
}

grouped_formula_cells::grouped_formula_cells(row_t rows, col_t cols, formula_tokens_t tokens) :
    m_cells(rows*cols), m_cols(cols)
{
    formula_tokens_store_ptr_t ts = formula_tokens_store::create();
    ts->get() = std::move(tokens);

    calc_status_ptr_t cs(new calc_status);

    for (row_t row = 0; row < rows; ++row)
    {
        for (col_t col = 0; col < cols; ++col)
        {
            size_t pos = to_pos(row, col);
            m_cells[pos] = ixion::make_unique<formula_cell>(row, col, cs, ts);
        }
    }
}

std::unique_ptr<formula_cell> grouped_formula_cells::release(row_t row, col_t col)
{
    size_t pos = to_pos(row, col);
    return std::move(m_cells[pos]);
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
