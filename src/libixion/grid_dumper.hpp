/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <ixion/types.hpp>

#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

namespace ixion {

class formula_cell;
class formula_name_resolver;
class model_context;
class sheet_view;
struct abs_address_t;

namespace detail {

/**
 * Dumps the content of a sheet to an output stream as a human-readable
 * text grid.
 */
class grid_dumper
{
    const model_context& m_cxt;
    std::unique_ptr<formula_name_resolver> m_own_resolver;
    const formula_name_resolver& m_resolver;

public:
    /**
     * @param cxt Model context to dump sheets from.
     * @param resolver Name resolver that determines the column label style
     *                 as well as the way formula expressions get printed in
     *                 verbose mode.  When null, an Excel A1 resolver gets
     *                 created and used internally.
     */
    grid_dumper(const model_context& cxt, const formula_name_resolver* resolver);
    ~grid_dumper();

    /**
     * Dump the data area of a sheet as a text grid with column and row
     * headers.  An empty sheet produces no output at all.
     *
     * @param os Output stream to dump the sheet content to.
     * @param sheet Index of the sheet to dump.
     * @param mode Amount of detail to include in the output.
     */
    void dump(std::ostream& os, sheet_t sheet, sheet_dump_mode_t mode) const;

    /**
     * Dump the data area of a sheet view as a text grid with column and row
     * headers, plus a column showing the base sheet row of each row.  An
     * empty view produces no output at all.
     *
     * @param os Output stream to dump the view content to.
     * @param view View to dump.
     * @param mode Amount of detail to include in the output.
     */
    void dump(std::ostream& os, const sheet_view& view, sheet_dump_mode_t mode) const;

private:
    /** Cell strings of a rectangular area, in row-major order. */
    struct grid
    {
        std::size_t row_count = 0;
        std::size_t col_count = 0;
        std::vector<std::string> cells;

        // base sheet row labels, one per row; empty when not dumping a view
        std::vector<std::string> base_row_labels;

        std::string& at(std::size_t row, std::size_t col)
        {
            return cells[col_count * row + col];
        }

        const std::string& at(std::size_t row, std::size_t col) const
        {
            return cells[col_count * row + col];
        }
    };

    std::string format_formula_cell(
        const formula_cell& cell, const abs_address_t& parent, row_t group_offset,
        sheet_dump_mode_t mode) const;

    void print_grid(std::ostream& os, const grid& g) const;
};

}}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
