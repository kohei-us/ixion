/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_GROUPED_FORMULA_CELLS_HPP
#define INCLUDED_IXION_GROUPED_FORMULA_CELLS_HPP

#include "ixion/cell.hpp"
#include "ixion/formula_tokens_fwd.hpp"

#include <vector>
#include <memory>

namespace ixion {

/**
 * Manages creation of grouped cells that share the same series of tokens.
 */
class grouped_formula_cells
{
    std::vector<std::unique_ptr<formula_cell>> m_cells;
    col_t m_cols;

    size_t to_pos(row_t row, col_t col) const;
public:
    grouped_formula_cells(row_t rows, col_t cols, formula_tokens_t tokens);

    std::unique_ptr<formula_cell> release(row_t row, col_t col);
};

}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
