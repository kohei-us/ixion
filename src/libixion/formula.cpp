/*************************************************************************
 *
 * Copyright (c) 2011 Kohei Yoshida
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

#include "ixion/formula.hpp"
#include "ixion/formula_lexer.hpp"
#include "ixion/formula_name_resolver.hpp"
#include "ixion/formula_parser.hpp"
#include "ixion/formula_functions.hpp"
#include "ixion/function_objects.hpp"
#include "ixion/cell.hpp"
#include "ixion/depends_tracker.hpp"
#include "ixion/cell_listener_tracker.hpp"

#define DEBUG_FORMULA_API 0

#include <sstream>

#if DEBUG_FORMULA_API
#include <iostream>
using namespace std;
#endif


namespace ixion {

void parse_formula_string(
    const iface::model_context& cxt, const abs_address_t& pos, const char* p, size_t n,
    formula_tokens_t& tokens)
{
    lexer_tokens_t lxr_tokens;
    formula_lexer lexer(p, n);
    lexer.tokenize();
    lexer.swap_tokens(lxr_tokens);

    formula_parser parser(lxr_tokens, cxt);
    parser.set_origin(pos);
    parser.parse();
    parser.get_tokens().swap(tokens);
}

namespace {

class print_formula_token : std::unary_function<formula_token_base, void>
{
    const iface::model_context& m_cxt;
    const abs_address_t& m_pos;
    std::ostringstream& m_os;
public:
    print_formula_token(const iface::model_context& cxt, const abs_address_t& pos, std::ostringstream& os) :
        m_cxt(cxt), m_pos(pos), m_os(os) {}

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
                switch (fop)
                {
                    case func_average:
                        m_os << "AVERAGE";
                        break;
                    case func_max:
                        m_os << "MAX";
                        break;
                    case func_min:
                        m_os << "MIN";
                        break;
                    case func_sum:
                        m_os << "SUM";
                        break;
                    case func_wait:
                        m_os << "WAIT";
                        break;
                    case func_unknown:
                    default:
                        m_os << "<unknown function>";
                }
            }
            break;
            case fop_single_ref:
            {
                abs_address_t addr = token.get_single_ref().to_abs(m_pos);
                const formula_name_resolver& resolver = m_cxt.get_name_resolver();
                m_os << resolver.get_name(addr, false);
            }
            break;
            case fop_range_ref:
            {
                abs_range_t range = token.get_range_ref().to_abs(m_pos);
                const formula_name_resolver& resolver = m_cxt.get_name_resolver();
                m_os << resolver.get_name(range, false);
            }
            break;
            case fop_err_no_ref:
            case fop_named_expression:
            case fop_string:
            case fop_unknown:
            default:
                ;
        }
    }
};

}

void print_formula_tokens(
    const iface::model_context& cxt, const abs_address_t& pos,
    const formula_tokens_t& tokens, std::string& str)
{
    std::ostringstream os;
    std::for_each(tokens.begin(), tokens.end(), print_formula_token(cxt, pos, os));
    str = os.str();
}

void register_formula_cell(
    iface::model_context& cxt, const abs_address_t& pos, formula_cell* cell)
{
    // TODO: Check for an existing formula cell at the specified position, and
    // if there is one, unregister it first.

    std::vector<const formula_token_base*> ref_tokens;
    cell->get_ref_tokens(cxt, ref_tokens);
    std::for_each(ref_tokens.begin(), ref_tokens.end(),
             formula_cell_listener_handler(cxt,
                 pos, formula_cell_listener_handler::mode_add));
}

void get_all_dirty_cells(
    iface::model_context& cxt, const dirty_cell_addrs_t& addrs, dirty_cells_t& cells)
{
#if DEBUG_FORMULA_API
    __IXION_DEBUG_OUT__ << "number of modified cells: " << addrs.size() << endl;
#endif
    // single, cell-to-cell listeners.
    dirty_cell_addrs_t::const_iterator itr = addrs.begin(), itr_end = addrs.end();
    for (; itr != itr_end; ++itr)
    {
        cell_listener_tracker& tracker = cxt.get_cell_listener_tracker();
        tracker.get_all_range_listeners(*itr, cells);
        tracker.get_all_cell_listeners(*itr, cells);
    }
}

void calculate_cells(iface::model_context& cxt, dirty_cells_t& cells, size_t thread_count)
{
    dependency_tracker deptracker(cells, cxt);
    std::for_each(cells.begin(), cells.end(),
                  cell_dependency_handler(cxt, deptracker, cells));
    deptracker.interpret_all_cells(thread_count);
}

}
