/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/formula.hpp"
#include "ixion/formula_lexer.hpp"
#include "ixion/formula_name_resolver.hpp"
#include "ixion/formula_parser.hpp"
#include "ixion/formula_functions.hpp"
#include "ixion/formula_function_opcode.hpp"
#include "ixion/function_objects.hpp"
#include "ixion/cell.hpp"
#include "ixion/depends_tracker.hpp"
#include "ixion/cell_listener_tracker.hpp"
#include "ixion/types.hpp"

#define DEBUG_FORMULA_API 0

#include <sstream>
#include <boost/scoped_ptr.hpp>

#if DEBUG_FORMULA_API
#include <iostream>
using namespace std;
#endif

namespace ixion {

void parse_formula_string(
    iface::model_context& cxt, const abs_address_t& pos,
    const formula_name_resolver& resolver,
    const char* p, size_t n, formula_tokens_t& tokens)
{
    lexer_tokens_t lxr_tokens;
    formula_lexer lexer(p, n);
    lexer.tokenize();
    lexer.swap_tokens(lxr_tokens);

    formula_parser parser(lxr_tokens, cxt);
    parser.set_name_resolver(&resolver);
    parser.set_origin(pos);
    parser.parse();
    parser.get_tokens().swap(tokens);
}

namespace {

class print_formula_token : std::unary_function<formula_token_base, void>
{
    const iface::model_context& m_cxt;
    const abs_address_t& m_pos;
    const formula_name_resolver& m_resolver;
    std::ostringstream& m_os;
public:
    print_formula_token(
        const iface::model_context& cxt, const abs_address_t& pos,
        const formula_name_resolver& resolver, std::ostringstream& os) :
        m_cxt(cxt),
        m_pos(pos),
        m_resolver(resolver),
        m_os(os) {}

    void operator() (const formula_token_base& token)
    {
        switch (token.get_opcode())
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
                m_os << token.get_value();
                break;
            case fop_sep:
                m_os << ",";
            break;
            case fop_function:
            {
                formula_function_t fop = static_cast<formula_function_t>(token.get_index());
                m_os << formula_functions::get_function_name(fop);
            }
            break;
            case fop_single_ref:
            {
                address_t addr = token.get_single_ref();
                m_os << m_resolver.get_name(addr, m_pos, false);
            }
            break;
            case fop_range_ref:
            {
                range_t range = token.get_range_ref();
                m_os << m_resolver.get_name(range, m_pos, false);
            }
            break;
            case fop_string:
            {
                const std::string* p = m_cxt.get_string(token.get_index());
                if (p)
                    m_os << "\"" << *p << "\"";
            }
            break;
            case fop_equal:
                m_os << "=";
            break;
            case fop_err_no_ref:
            case fop_named_expression:
            case fop_unknown:
            default:
                ;
        }
    }
};

}

void print_formula_tokens(
    const iface::model_context& cxt, const abs_address_t& pos,
    const formula_name_resolver& resolver, const formula_tokens_t& tokens,
    std::string& str)
{
    std::ostringstream os;
    std::for_each(tokens.begin(), tokens.end(), print_formula_token(cxt, pos, resolver, os));
    str = os.str();
}

namespace {

bool has_volatile(const formula_tokens_t& tokens)
{
    formula_tokens_t::const_iterator i = tokens.begin(), iend = tokens.end();
    for (; i != iend; ++i)
    {
        const formula_token_base& t = *i;
        if (t.get_opcode() != fop_function)
            continue;

        formula_function_t func = static_cast<formula_function_t>(t.get_index());
        if (is_volatile(func))
            return true;
    }
    return false;
}

}

void register_formula_cell(iface::model_context& cxt, const abs_address_t& pos)
{
    formula_cell* cell = cxt.get_formula_cell(pos);
    if (!cell)
        // Not a formula cell. Bail out.
        return;

    std::vector<const formula_token_base*> ref_tokens;
    cell->get_ref_tokens(cxt, pos, ref_tokens);
    std::for_each(ref_tokens.begin(), ref_tokens.end(),
             formula_cell_listener_handler(cxt,
                 pos, formula_cell_listener_handler::mode_add));

    // Check if the cell is volatile.
    const formula_tokens_t* tokens = cxt.get_formula_tokens(pos.sheet, cell->get_identifier());
    if (tokens && has_volatile(*tokens))
        cxt.get_cell_listener_tracker().add_volatile(pos);
}

void unregister_formula_cell(iface::model_context& cxt, const abs_address_t& pos)
{
    // When there is a formula cell at this position, unregister it from
    // the dependency tree.
    formula_cell* fcell = cxt.get_formula_cell(pos);
    if (!fcell)
        // Not a formula cell. Bail out.
        return;

    cell_listener_tracker& tracker = cxt.get_cell_listener_tracker();
    tracker.remove_volatile(pos);

    // Go through all its existing references, and remove
    // itself as their listener.  This step is important
    // especially during partial re-calculation.
    std::vector<const formula_token_base*> ref_tokens;
    fcell->get_ref_tokens(cxt, pos, ref_tokens);
    for_each(ref_tokens.begin(), ref_tokens.end(),
             formula_cell_listener_handler(cxt,
                 pos, formula_cell_listener_handler::mode_remove));
}

void get_all_dirty_cells(
    iface::model_context& cxt, modified_cells_t& addrs, dirty_formula_cells_t& cells)
{
#if DEBUG_FORMULA_API
    __IXION_DEBUG_OUT__ << "number of modified cells: " << addrs.size() << endl;
#endif

    cell_listener_tracker& tracker = cxt.get_cell_listener_tracker();

    // Volatile cells are always included.
    const cell_listener_tracker::address_set_type& vcells = tracker.get_volatile_cells();
    {
        cell_listener_tracker::address_set_type::const_iterator itr = vcells.begin(), itr_end = vcells.end();
        for (; itr != itr_end; ++itr)
        {
            if (cxt.get_celltype(*itr) != celltype_formula)
                continue;

            addrs.push_back(*itr);
            cells.insert(*itr);
        }
    }

    {
        modified_cells_t::const_iterator itr = addrs.begin(), itr_end = addrs.end();
        for (; itr != itr_end; ++itr)
        {
            tracker.get_all_range_listeners(*itr, cells);
            tracker.get_all_cell_listeners(*itr, cells);
        }
    }
}

void calculate_cells(iface::model_context& cxt, dirty_formula_cells_t& cells, size_t thread_count)
{
    dependency_tracker deptracker(cells, cxt);
    std::for_each(cells.begin(), cells.end(),
                  cell_dependency_handler(cxt, deptracker, cells));
    deptracker.interpret_all_cells(thread_count);
}

}
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
