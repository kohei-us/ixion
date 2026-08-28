/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "grid_dumper.hpp"
#include "utf8.hpp"

#include <ixion/address.hpp>
#include <ixion/cell.hpp>
#include <ixion/formula.hpp>
#include <ixion/formula_name_resolver.hpp>
#include <ixion/formula_result.hpp>
#include <ixion/model_cell_range.hpp>
#include <ixion/model_context.hpp>
#include <ixion/sheet_view.hpp>

#include <algorithm>
#include <cassert>
#include <iomanip>
#include <limits>
#include <memory>
#include <ostream>
#include <span>
#include <sstream>
#include <string>
#include <vector>

namespace ixion { namespace detail {

namespace {

std::size_t display_width(std::string_view s)
{
    // Logical character count; CJK double-width is not accounted for.
    return calc_utf8_byte_positions(s).size();
}

std::string format_number(double v)
{
    std::ostringstream os;
    os << std::setprecision(std::numeric_limits<double>::digits10 + 1) << v;
    return os.str();
}

std::string format_number_cell(double v, sheet_dump_mode_t mode)
{
    std::string s = format_number(v);
    if (mode == sheet_dump_mode_t::verbose)
        s += " [v]";
    return s;
}

std::string format_boolean_cell(bool b, sheet_dump_mode_t mode)
{
    std::string s = b ? "true" : "false";
    if (mode == sheet_dump_mode_t::verbose)
        s += " [b]";
    return s;
}

std::string print_formula_cell_value(const model_context& cxt, const formula_cell& cell)
{
    try
    {
        formula_result res = cell.get_result_cache(formula_result_wait_policy_t::throw_exception);
        if (res.get_type() == formula_result::result_type::value)
            return format_number(res.get_value());

        return res.str(cxt);
    }
    catch (const std::exception&)
    {
        return "#RES!";
    }
}

} // anonymous namespace

grid_dumper::grid_dumper(const model_context& cxt, const formula_name_resolver* resolver) :
    m_cxt(cxt),
    m_own_resolver(resolver ? nullptr : formula_name_resolver::get(formula_name_resolver_t::excel_a1, &cxt)),
    m_resolver(resolver ? *resolver : *m_own_resolver)
{}

grid_dumper::~grid_dumper() = default;

std::string grid_dumper::format_formula_cell(
    const formula_cell& cell, const abs_address_t& parent, row_t group_offset,
    sheet_dump_mode_t mode) const
{
    if (mode != sheet_dump_mode_t::verbose)
        return print_formula_cell_value(m_cxt, cell);

    const formula_tokens_store_ptr_t& ts = cell.get_tokens();
    if (!ts)
        return std::string{};

    std::ostringstream os;

    std::string formula = print_formula_tokens(m_cxt, parent, m_resolver, ts->get());

    formula_group_t group = cell.get_group_properties();
    if (group.grouped)
        os << '{' << formula << "}@" << group_offset << '/' << group.size.row;
    else
        os << formula;

    try
    {
        formula_result res = cell.get_result_cache(formula_result_wait_policy_t::throw_exception);
        os << " (" << res.str(m_cxt) << ")";
    }
    catch (const std::exception&)
    {
        os << " (#RES!)";
    }

    return os.str();
}

void grid_dumper::dump(std::ostream& os, sheet_t sheet, sheet_dump_mode_t mode) const
{
    abs_range_t data_range = m_cxt.get_data_range(sheet);
    if (!data_range.valid())
        // The sheet has no content; empty in, empty out.
        return;

    // Always start at the top-left corner of the sheet.
    grid g;
    g.row_count = data_range.last.row + 1;
    g.col_count = data_range.last.column + 1;
    g.cells.resize(g.row_count * g.col_count);

    abs_rc_range_t range;
    range.first.row = 0;
    range.first.column = 0;
    range.last.row = data_range.last.row;
    range.last.column = data_range.last.column;

    // Collect all cell values to display.
    for (const auto& c : m_cxt.iterate_cells(sheet, rc_direction_t::vertical, range))
    {
        std::string s;

        switch (c.type)
        {
            case cell_t::string:
                s = std::get<std::string_view>(c.value);
                break;
            case cell_t::numeric:
                s = format_number_cell(std::get<double>(c.value), mode);
                break;
            case cell_t::boolean:
                s = format_boolean_cell(std::get<bool>(c.value), mode);
                break;
            case cell_t::formula:
            {
                const formula_cell* fc = std::get<const formula_cell*>(c.value);
                assert(fc);

                abs_address_t pos(sheet, c.row, c.col);
                abs_address_t parent = fc->get_parent_position(pos);
                s = format_formula_cell(*fc, parent, pos.row - parent.row, mode);
                break;
            }
            default:
                ;
        }

        if (!s.empty())
            g.at(c.row, c.col) = std::move(s);
    }

    print_grid(os, g);
}

void grid_dumper::dump(std::ostream& os, const sheet_view& view, sheet_dump_mode_t mode) const
{
    abs_rc_range_t data_range = view.get_data_range();
    if (!data_range.valid())
        // The view has no content; empty in, empty out.
        return;

    // Always start at the top-left corner of the view.
    grid g;
    g.row_count = data_range.last.row + 1;
    g.col_count = data_range.last.column + 1;
    g.cells.resize(g.row_count * g.col_count);
    g.base_row_labels.reserve(g.row_count);

    sheet_t sheet = view.get_sheet();

    for (row_t row = 0; row < row_t(g.row_count); ++row)
    {
        // Base sheet row numbers are 1-based like the row labels.
        g.base_row_labels.push_back(std::to_string(view.to_base_row(row) + 1));

        for (col_t col = 0; col < col_t(g.col_count); ++col)
        {
            abs_rc_address_t pos(row, col);
            std::string s;

            switch (view.get_celltype(pos))
            {
                case cell_t::string:
                    s = view.get_string_value(pos);
                    break;
                case cell_t::numeric:
                    s = format_number_cell(view.get_numeric_value(pos), mode);
                    break;
                case cell_t::boolean:
                    s = format_boolean_cell(view.get_boolean_value(pos), mode);
                    break;
                case cell_t::formula:
                {
                    const formula_cell* fc = view.get_formula_cell(pos);
                    assert(fc);

                    // Print the formula as it reads on the base sheet: anchor
                    // it at the base row of the group head (the cell itself
                    // when not grouped) rather than at the row it landed on.
                    abs_address_t view_pos(sheet, row, col);
                    row_t group_offset = row - fc->get_parent_position(view_pos).row;
                    abs_address_t parent(sheet, view.to_base_row(row - group_offset), col);
                    s = format_formula_cell(*fc, parent, group_offset, mode);
                    break;
                }
                default:
                    ;
            }

            if (!s.empty())
                g.at(row, col) = std::move(s);
        }
    }

    print_grid(os, g);
}

void grid_dumper::print_grid(std::ostream& os, const grid& g) const
{
    // Row numbers are 1-based to match the A1 references in formulas.
    std::size_t row_label_width = std::to_string(g.row_count).size();

    // The base row column, when present, is right-aligned like the row labels.
    bool has_base_rows = !g.base_row_labels.empty();
    std::size_t base_label_width = 0;

    if (has_base_rows)
    {
        base_label_width = display_width("base");

        for (const std::string& label : g.base_row_labels)
            base_label_width = std::max(base_label_width, label.size());
    }

    std::vector<std::string> col_labels(g.col_count);
    std::vector<std::size_t> col_widths(g.col_count);

    for (std::size_t col = 0; col < g.col_count; ++col)
    {
        col_labels[col] = m_resolver.get_column_name(col);
        col_widths[col] = display_width(col_labels[col]);
    }

    for (std::size_t row = 0; row < g.row_count; ++row)
    {
        for (std::size_t col = 0; col < g.col_count; ++col)
        {
            std::size_t w = display_width(g.at(row, col));
            if (col_widths[col] < w)
                col_widths[col] = w;
        }
    }

    auto print_border = [&os, &col_widths, row_label_width, has_base_rows, base_label_width]()
    {
        os << '+' << std::string(row_label_width + 2, '-');
        if (has_base_rows)
            os << '+' << std::string(base_label_width + 2, '-');
        for (std::size_t w : col_widths)
            os << '+' << std::string(w + 2, '-');
        os << '+';
    };

    auto print_label = [&os](std::string_view label, std::size_t width)
    {
        os << ' ' << std::string(width - label.size(), ' ') << label << " |";
    };

    auto print_grid_row =
        [&os, &col_widths, &print_label, row_label_width, has_base_rows, base_label_width](
            std::string_view label, std::string_view base_label,
            std::span<const std::string> row_cells)
    {
        os << '|';
        print_label(label, row_label_width);
        if (has_base_rows)
            print_label(base_label, base_label_width);

        for (std::size_t col = 0; col < row_cells.size(); ++col)
        {
            const std::string& s = row_cells[col];
            os << ' ' << s << std::string(col_widths[col] - display_width(s), ' ') << " |";
        }
    };

    // Each line break precedes its line so that the output does not end
    // with one.
    print_border();

    os << '\n';
    print_grid_row(std::string_view{}, "base", col_labels);

    os << '\n';
    print_border();

    for (std::size_t row = 0; row < g.row_count; ++row)
    {
        os << '\n';
        const auto* p0 = &g.at(row, 0); // pointer to the first cell on this row
        std::span<const std::string> row_cells{p0, g.col_count};
        std::string_view base_label = has_base_rows ? g.base_row_labels[row] : std::string_view{};
        print_grid_row(std::to_string(row + 1), base_label, row_cells);
    }

    os << '\n';
    print_border();
}

}}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
