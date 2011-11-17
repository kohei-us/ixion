/*************************************************************************
 *
 * Copyright (c) 2010, 2011 Kohei Yoshida
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

#include "ixion/formula_parser.hpp"
#include "ixion/formula_functions.hpp"
#include "ixion/formula_name_resolver.hpp"
#include "ixion/interface/model_context.hpp"

#include "ixion/hash_container/map.hpp"

#include <iostream>
#include <sstream>

#define DEBUG_FORMULA_PARSER 0

using namespace std;

namespace ixion {

namespace {

class ref_error : public general_error
{
public:
    ref_error(const string& msg) :
        general_error(msg) {}
};

class formula_token_printer : public unary_function<formula_token_base, void>
{
public:
    formula_token_printer()
    {
    }

    void operator() (const formula_token_base& token) const
    {
        fopcode_t oc = token.get_opcode();
        ostringstream os;
        os << " ";
        os << "<" << get_opcode_name(oc) << ">'";

        switch (oc)
        {
            case fop_close:
                os << ")";
                break;
            case fop_divide:
                os << "/";
                break;
            case fop_err_no_ref:
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
            case fop_sep:
                os << ",";
                break;
            case fop_single_ref:
            {
                address_t addr = token.get_single_ref();
                os << "(s=" << addr.sheet << ",r=" << addr.row << ",c=" << addr.column << ")";
            }
            break;
            case fop_range_ref:
            {
                range_t range = token.get_range_ref();
                os << "(s=" << range.first.sheet << ":" << range.last.sheet
                    << ",r=" << range.first.row << ":" << range.last.row
                    << ",c=" << range.first.column << ":" << range.last.column << ")";
            }
            break;
            case fop_named_expression:
                os << token.get_name();
                break;
            case fop_string:
                break;
            case fop_value:
                os << token.get_value();
                break;
            case fop_function:
            {
                formula_function_t func_oc = formula_functions::get_function_opcode(token);
                os << formula_functions::get_function_name(func_oc);
            }
            break;
            default:
                ;
        }
        os << "'";
#if DEBUG_FORMULA_PARSER
        cout << os.str();
#endif
    }
};

}

// ----------------------------------------------------------------------------

formula_parser::parse_error::parse_error(const string& msg) :
    general_error(msg) {}

// ----------------------------------------------------------------------------

formula_parser::formula_parser(const lexer_tokens_t& tokens, iface::model_context& cxt) :
    m_tokens(tokens),
    m_context(cxt)
{
}

formula_parser::~formula_parser()
{
}

void formula_parser::set_origin(const abs_address_t& pos)
{
    m_pos = pos;
}

void formula_parser::parse()
{
    try
    {
        size_t n = m_tokens.size();
        for (size_t i = 0; i < n; ++i)
        {
            const lexer_token_base& t = m_tokens[i];
            lexer_opcode_t oc = t.get_opcode();
            switch (oc)
            {
                case op_open:
                case op_close:
                case op_plus:
                case op_minus:
                case op_multiply:
                case op_divide:
                case op_sep:
                    primitive(oc);
                    break;
                case op_name:
                    name(t);
                    break;
                case op_string:
                    literal(t);
                    break;
                case op_value:
                    value(t);
                    break;
                default:
                    ;
            }
        }
    }
#if DEBUG_FORMULA_PARSER
    catch (const ref_error& e)
    {
        cout << "reference error: " << e.what() << endl;
    }
    catch (const parse_error& e)
    {
        cout << "parse error: " << e.what() << endl;
    }
#else
    catch (const general_error&) {}
#endif
}

void formula_parser::print_tokens() const
{
#if DEBUG_FORMULA_PARSER
    cout << "formula tokens:";
    for_each(m_formula_tokens.begin(), m_formula_tokens.end(), formula_token_printer());
    cout << " (size=" << m_formula_tokens.size() << ")";
    cout << endl;
#endif
}

formula_tokens_t& formula_parser::get_tokens()
{
    return m_formula_tokens;
}

void formula_parser::primitive(lexer_opcode_t oc)
{
    fopcode_t foc = fop_unknown;
    switch (oc)
    {
        case op_close:
            foc = fop_close;
            break;
        case op_divide:
            foc = fop_divide;
            break;
        case op_minus:
            foc = fop_minus;
            break;
        case op_multiply:
            foc = fop_multiply;
            break;
        case op_open:
            foc = fop_open;
            break;
        case op_plus:
            foc = fop_plus;
            break;
        case op_sep:
            foc = fop_sep;
            break;
        default:
            throw parse_error("unknown primitive token received");
    }
    m_formula_tokens.push_back(new opcode_token(foc));
}

void formula_parser::name(const lexer_token_base& t)
{
    mem_str_buf name = t.get_string();
    formula_name_type fn = m_context.get_name_resolver().resolve(name.get(), name.size(), m_pos);
#if DEBUG_FORMULA_PARSER
            __IXION_DEBUG_OUT__ << "name = '" << name << "' - " << fn.to_string() << endl;
#endif
    switch (fn.type)
    {
        case formula_name_type::cell_reference:
            m_formula_tokens.push_back(
                new single_ref_token(
                    address_t(fn.address.sheet, fn.address.row, fn.address.col,
                              fn.address.abs_sheet, fn.address.abs_row, fn.address.abs_col)));
        break;
        case formula_name_type::range_reference:
        {
            address_t first(fn.range.first.sheet, fn.range.first.row, fn.range.first.col,
                            fn.range.first.abs_sheet, fn.range.first.abs_row, fn.range.first.abs_col);
            address_t last(fn.range.last.sheet, fn.range.last.row, fn.range.last.col,
                           fn.range.last.abs_sheet, fn.range.last.abs_row, fn.range.last.abs_col);
            m_formula_tokens.push_back(
                new range_ref_token(range_t(first, last)));
        }
        break;
        case formula_name_type::function:
            m_formula_tokens.push_back(new function_token(static_cast<size_t>(fn.func_oc)));
        break;
        case formula_name_type::named_expression:
            m_formula_tokens.push_back(new named_exp_token(name.get(), name.size()));
        break;
        default:
        {
            ostringstream os;
            os << "failed to resolve a name '" << name.str() << "'.";
            throw parse_error(os.str());
        }
    }
}

void formula_parser::literal(const lexer_token_base& t)
{
    mem_str_buf s = t.get_string();
    size_t sid = m_context.add_string(s.get(), s.size());
    m_formula_tokens.push_back(new string_token(sid));
}

void formula_parser::value(const lexer_token_base& t)
{
    double val = t.get_value();
    m_formula_tokens.push_back(new value_token(val));
}

}
