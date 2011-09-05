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
#include "ixion/formula_parser.hpp"
#include "ixion/formula_functions.hpp"

#include <sstream>

namespace ixion {

void parse_formula_string(
    const interface::model_context& cxt, const char* p, size_t n, const abs_address_t& pos,
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

void print_formula_tokens(
    const interface::model_context& cxt, const formula_tokens_t& tokens, const abs_address_t& pos,
    std::string& str)
{
    std::ostringstream os;
    formula_tokens_t::const_iterator itr = tokens.begin(), itr_end = tokens.end();
    for (; itr != itr_end; ++itr)
    {
        switch (itr->get_opcode())
        {
            case fop_close:
                os << ")";
                break;
            case fop_divide:
                os << "/";
                break;
            case fop_minus:
                os << "-";
                break;
            case fop_multiply:
                os << "*";
                break;
            case fop_open:
                os << "(";
                break;
            case fop_plus:
                os << "+";
                break;
            case fop_value:
                os << itr->get_value();
                break;
            case fop_sep:
                os << ",";
            break;
            case fop_function:
            {
                formula_function_t fop = static_cast<formula_function_t>(itr->get_index());
                switch (fop)
                {
                    case func_average:
                        os << "AVERAGE";
                        break;
                    case func_max:
                        os << "MAX";
                        break;
                    case func_min:
                        os << "MIN";
                        break;
                    case func_sum:
                        os << "SUM";
                        break;
                    case func_wait:
                        os << "WAIT";
                        break;
                    case func_unknown:
                    default:
                        os << "<unknown function>";
                }
            }
            break;
            case fop_err_no_ref:
            case fop_named_expression:
            case fop_range_ref:
            case fop_single_ref:
            case fop_string:
            case fop_unknown:
            case fop_unresolved_ref:
            default:
                ;
        }
    }
    str = os.str();
}

}
