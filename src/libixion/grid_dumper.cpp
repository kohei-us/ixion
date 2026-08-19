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

std::string print_formula_cell_verbose(
    const model_context& cxt, const abs_address_t& pos,
    const formula_name_resolver& resolver, const formula_cell& cell)
{
    const formula_tokens_store_ptr_t& ts = cell.get_tokens();
    if (!ts)
        return std::string{};

    std::ostringstream os;

    abs_address_t parent = cell.get_parent_position(pos);
    std::string formula = print_formula_tokens(cxt, parent, resolver, ts->get());

    formula_group_t group = cell.get_group_properties();
    if (group.grouped)
        os << '{' << formula << "}@" << (pos.row - parent.row) << '/' << group.size.row;
    else
        os << formula;

    try
    {
        formula_result res = cell.get_result_cache(formula_result_wait_policy_t::throw_exception);
        os << " (" << res.str(cxt) << ")";
    }
    catch (const std::exception&)
    {
        os << " (#RES!)";
    }

    return os.str();
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

void grid_dumper::dump(std::ostream& os, sheet_t sheet, sheet_dump_mode_t mode) const
{
    abs_range_t data_range = m_cxt.get_data_range(sheet);
    if (!data_range.valid())
        // The sheet has no content; empty in, empty out.
        return;

    // Always start at the top-left corner of the sheet.
    std::size_t row_count = data_range.last.row + 1;
    std::size_t col_count = data_range.last.column + 1;

    abs_rc_range_t range;
    range.first.row = 0;
    range.first.column = 0;
    range.last.row = data_range.last.row;
    range.last.column = data_range.last.column;

    std::vector<std::string> cells(row_count * col_count);

    auto to_pos = [col_count](std::size_t row, std::size_t col)
    {
        return col_count * row + col;
    };

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
                s = format_number(std::get<double>(c.value));
                if (mode == sheet_dump_mode_t::verbose)
                    s += " [v]";
                break;
            case cell_t::boolean:
                s = std::get<bool>(c.value) ? "true" : "false";
                if (mode == sheet_dump_mode_t::verbose)
                    s += " [b]";
                break;
            case cell_t::formula:
            {
                const formula_cell* fc = std::get<const formula_cell*>(c.value);
                assert(fc);

                if (mode == sheet_dump_mode_t::verbose)
                    s = print_formula_cell_verbose(
                        m_cxt, abs_address_t(sheet, c.row, c.col), m_resolver, *fc);
                else
                    s = print_formula_cell_value(m_cxt, *fc);
                break;
            }
            default:
                ;
        }

        if (!s.empty())
            cells[to_pos(c.row, c.col)] = std::move(s);
    }

    // Row numbers are 1-based to match the A1 references in formulas.
    std::size_t row_label_width = std::to_string(row_count).size();

    std::vector<std::string> col_labels(col_count);
    std::vector<std::size_t> col_widths(col_count);

    for (std::size_t col = 0; col < col_count; ++col)
    {
        col_labels[col] = m_resolver.get_column_name(col);
        col_widths[col] = display_width(col_labels[col]);
    }

    for (std::size_t row = 0; row < row_count; ++row)
    {
        for (std::size_t col = 0; col < col_count; ++col)
        {
            std::size_t w = display_width(cells[to_pos(row, col)]);
            if (col_widths[col] < w)
                col_widths[col] = w;
        }
    }

    auto print_border = [&os, &col_widths, row_label_width]()
    {
        os << '+' << std::string(row_label_width + 2, '-');
        for (std::size_t w : col_widths)
            os << '+' << std::string(w + 2, '-');
        os << '+';
    };

    auto print_grid_row = [&os, &col_widths, row_label_width](
        std::string_view label, std::span<const std::string> row_cells)
    {
        os << "| " << std::string(row_label_width - label.size(), ' ') << label << " |";

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
    print_grid_row(std::string_view{}, col_labels);

    os << '\n';
    print_border();

    for (std::size_t row = 0; row < row_count; ++row)
    {
        os << '\n';
        const auto* p0 = &cells[to_pos(row, 0)]; // pointer to the first cell on this row
        std::span<const std::string> row_cells{p0, col_count};
        print_grid_row(std::to_string(row + 1), row_cells);
    }

    os << '\n';
    print_border();
}

}}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
