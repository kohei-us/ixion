/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/formula.hpp"
#include "ixion/formula_name_resolver.hpp"
#include "ixion/formula_function_opcode.hpp"
#include "ixion/cell.hpp"
#include "ixion/dirty_cell_tracker.hpp"
#include "ixion/types.hpp"

#include "formula_lexer.hpp"
#include "formula_parser.hpp"
#include "formula_functions.hpp"
#include "topo_sort_calculator.hpp"

#define DEBUG_FORMULA_API 0

#include <sstream>
#include <algorithm>

#if DEBUG_FORMULA_API
#include <iostream>
using namespace std;
#endif

namespace ixion {

formula_tokens_t parse_formula_string(
    iface::formula_model_access& cxt, const abs_address_t& pos,
    const formula_name_resolver& resolver, const char* p, size_t n)
{
    lexer_tokens_t lxr_tokens;
    formula_lexer lexer(cxt.get_config(), p, n);
    lexer.tokenize();
    lexer.swap_tokens(lxr_tokens);

    formula_tokens_t tokens;
    formula_parser parser(lxr_tokens, cxt, resolver);
    parser.set_origin(pos);
    parser.parse();
    parser.get_tokens().swap(tokens);

    return tokens;
}

namespace {

class print_formula_token : std::unary_function<formula_tokens_t::value_type, void>
{
    const iface::formula_model_access& m_cxt;
    const abs_address_t& m_pos;
    const formula_name_resolver& m_resolver;
    std::ostringstream& m_os;
public:
    print_formula_token(
        const iface::formula_model_access& cxt, const abs_address_t& pos,
        const formula_name_resolver& resolver, std::ostringstream& os) :
        m_cxt(cxt),
        m_pos(pos),
        m_resolver(resolver),
        m_os(os) {}

    void operator() (const formula_tokens_t::value_type& token)
    {
        switch (token->get_opcode())
        {
            case fop_close:
                m_os << ")";
                break;
            case fop_divide:
                m_os << "/";
                break;
            case fop_minus:
                m_os << "-";
                break;
            case fop_multiply:
                m_os << "*";
                break;
            case fop_open:
                m_os << "(";
                break;
            case fop_plus:
                m_os << "+";
                break;
            case fop_value:
                m_os << token->get_value();
                break;
            case fop_sep:
                m_os << m_cxt.get_config().sep_function_arg;
                break;
            case fop_function:
            {
                formula_function_t fop = static_cast<formula_function_t>(token->get_index());
                m_os << formula_functions::get_function_name(fop);
                break;
            }
            case fop_single_ref:
            {
                address_t addr = token->get_single_ref();
                m_os << m_resolver.get_name(addr, m_pos, false);
                break;
            }
            case fop_range_ref:
            {
                range_t range = token->get_range_ref();
                m_os << m_resolver.get_name(range, m_pos, false);
                break;
            }
            case fop_table_ref:
            {
                table_t tbl = token->get_table_ref();
                m_os << m_resolver.get_name(tbl);
                break;
            }
            case fop_string:
            {
                const std::string* p = m_cxt.get_string(token->get_index());
                if (p)
                    m_os << "\"" << *p << "\"";
                break;
            }
            case fop_equal:
                m_os << "=";
                break;
            case fop_named_expression:
                m_os << token->get_name();
                break;
            case fop_err_no_ref:
                m_os << "#REF!";
                break;
            case fop_err_no_name:
                m_os << "#NAME?";
                break;
            case fop_unknown:
            default:
                ;
        }
    }
};

}

std::string print_formula_tokens(
    const iface::formula_model_access& cxt, const abs_address_t& pos,
    const formula_name_resolver& resolver, const formula_tokens_t& tokens)
{
    std::ostringstream os;
    std::for_each(tokens.begin(), tokens.end(), print_formula_token(cxt, pos, resolver, os));
    return os.str();
}

namespace {

bool is_volatile(formula_function_t func)
{
    switch (func)
    {
        case formula_function_t::func_now:
            return true;
        default:
            ;
    }
    return false;
}

bool has_volatile(const formula_tokens_t& tokens)
{
    formula_tokens_t::const_iterator i = tokens.begin(), iend = tokens.end();
    for (; i != iend; ++i)
    {
        const formula_token& t = **i;
        if (t.get_opcode() != fop_function)
            continue;

        formula_function_t func = static_cast<formula_function_t>(t.get_index());
        if (is_volatile(func))
            return true;
    }
    return false;
}

}

void register_formula_cell(iface::formula_model_access& cxt, const abs_address_t& pos)
{
    formula_cell* cell = cxt.get_formula_cell(pos);
    if (!cell)
        // Not a formula cell. Bail out.
        return;

    formula_group_t fg_props = cell->get_group_properties();
    dirty_cell_tracker& tracker = cxt.get_cell_tracker();

    abs_range_t src_pos = pos;
    if (fg_props.grouped)
    {
        // Expand the source range for grouped formula cells.
        src_pos.last.column += fg_props.size.column - 1;
        src_pos.last.row += fg_props.size.row - 1;
    }

    std::vector<const formula_token*> ref_tokens = cell->get_ref_tokens(cxt, pos);

    for (const formula_token* p : ref_tokens)
    {
        switch (p->get_opcode())
        {
            case fop_single_ref:
            {
                abs_address_t addr = p->get_single_ref().to_abs(pos);
                tracker.add(src_pos, addr);
                break;
            }
            case fop_range_ref:
            {
                abs_range_t range = p->get_range_ref().to_abs(pos);
                tracker.add(src_pos, range);
                break;
            }
            default:
                ; // ignore the rest.
        }
    }

    // Check if the cell is volatile.
    const formula_tokens_store_ptr_t& ts = cell->get_tokens();
    if (ts && has_volatile(ts->get()))
        tracker.add_volatile(pos);
}

void unregister_formula_cell(iface::formula_model_access& cxt, const abs_address_t& pos)
{
    // When there is a formula cell at this position, unregister it from
    // the dependency tree.
    formula_cell* fcell = cxt.get_formula_cell(pos);
    if (!fcell)
        // Not a formula cell. Bail out.
        return;

    dirty_cell_tracker& tracker = cxt.get_cell_tracker();
    tracker.remove_volatile(pos);

    // Go through all its existing references, and remove
    // itself as their listener.  This step is important
    // especially during partial re-calculation.
    std::vector<const formula_token*> ref_tokens = fcell->get_ref_tokens(cxt, pos);

    for (const formula_token* p : ref_tokens)
    {

        switch (p->get_opcode())
        {
            case fop_single_ref:
            {
                abs_address_t addr = p->get_single_ref().to_abs(pos);
                tracker.remove(pos, addr);
                break;
            }
            case fop_range_ref:
            {
                abs_range_t range = p->get_range_ref().to_abs(pos);
                tracker.remove(pos, range);
                break;
            }
            default:
                ; // ignore the rest.
        }
    }
}

abs_address_set_t query_dirty_cells(iface::formula_model_access& cxt, const abs_address_set_t& modified_cells)
{
    abs_range_set_t modified_ranges;
    for (const abs_address_t& mc : modified_cells)
        modified_ranges.insert(mc);

    const dirty_cell_tracker& tracker = cxt.get_cell_tracker();
    abs_range_set_t dirty_ranges = tracker.query_dirty_cells(modified_ranges);

    // Convert a set of ranges to a set of addresses.
    abs_address_set_t dirty_cells;
    std::for_each(dirty_ranges.begin(), dirty_ranges.end(),
        [&dirty_cells](const abs_range_t& r)
        {
            dirty_cells.insert(r.first);
        }
    );
    return dirty_cells;
}

std::vector<abs_range_t> query_and_sort_dirty_cells(
    iface::formula_model_access& cxt, const abs_range_set_t& modified_cells,
    const abs_range_set_t* dirty_formula_cells)
{
    const dirty_cell_tracker& tracker = cxt.get_cell_tracker();
    return tracker.query_and_sort_dirty_cells(modified_cells, dirty_formula_cells);
}

void calculate_cells(iface::formula_model_access& cxt, abs_address_set_t& formula_cells, size_t thread_count)
{
    topo_sort_calculator deptracker(formula_cells, cxt);

    for (const abs_address_t& fcell : formula_cells)
    {
        // Register cell dependencies.
        formula_cell* fp = cxt.get_formula_cell(fcell);
        assert(fp);
        std::vector<const formula_token*> ref_tokens = fp->get_ref_tokens(cxt, fcell);

        // Pick up the referenced cells from the ref tokens.  I should
        // probably combine this with the above get_ref_tokens() call above
        // for efficiency.
        std::vector<abs_address_t> deps;

        for (const formula_token* p : ref_tokens)
        {
            switch (p->get_opcode())
            {
                case fop_single_ref:
                {
                    abs_address_t addr = p->get_single_ref().to_abs(fcell);
                    if (cxt.get_celltype(addr) != celltype_t::formula)
                        break;

                    deps.push_back(addr);
                    break;
                }
                case fop_range_ref:
                {
                    abs_range_t range = p->get_range_ref().to_abs(fcell);
                    for (sheet_t sheet = range.first.sheet; sheet <= range.last.sheet; ++sheet)
                    {
                        for (col_t col = range.first.column; col <= range.last.column; ++col)
                        {
                            for (row_t row = range.first.row; row <= range.last.row; ++row)
                            {
                                abs_address_t addr(sheet, row, col);
                                if (cxt.get_celltype(addr) != celltype_t::formula)
                                    continue;

                                deps.push_back(addr);
                            }
                        }
                    }
                    break;
                }
                default:
                    ; // ignore the rest.
            }
        }

        // Register dependency information.  Only dirty cells should be
        // registered as precedent cells since non-dirty cells are equivalent
        // to constants.
        for (const abs_address_t& cell : deps)
        {
            if (formula_cells.count(cell) > 0)
                deptracker.set_reference_relation(fcell, cell);
        }
    }

    deptracker.interpret_all_cells(thread_count);
}

void calculate_sorted_cells(
    iface::formula_model_access& cxt, const std::vector<abs_range_t>& formula_cells, size_t thread_count)
{
    // Convert an array of ranges to an array of cells.  Be reminded that a
    // range represents a grouped fromula cells whose top-left cell represents
    // the whole group.
    std::vector<abs_address_t> fcs;
    for (const abs_range_t& r : formula_cells)
        fcs.push_back(r.first);

    topo_sort_calculator::calculate_sorted_cells(cxt, std::move(fcs), thread_count);
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
