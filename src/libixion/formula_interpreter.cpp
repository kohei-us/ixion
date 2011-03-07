/*************************************************************************
 *
 * Copyright (c) 2010 Kohei Yoshida
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

#include "formula_interpreter.hpp"
#include "cell.hpp"
#include "global.hpp"
#include "formula_functions.hpp"
#include "model_context.hpp"

#include <string>
#include <iostream>
#include <sstream>

using namespace std;

namespace ixion {

namespace {

class invalid_expression : public general_error
{
public:
    invalid_expression(const string& msg) : general_error(msg) {}
};

}

formula_interpreter::formula_interpreter(const formula_cell* cell, const model_context& cxt) :
    m_parent_cell(cell),
    m_tokens(cell->get_tokens()),
    m_context(cxt),
    m_end_token_pos(m_tokens.end()),
    m_result(0.0),
    m_error(fe_no_error)
{
}

formula_interpreter::~formula_interpreter()
{
}

bool formula_interpreter::interpret()
{
    if (m_tokens.empty())
    {
        cout << "interpreter error: no tokens to interpret" << endl;
        return false;
    }

    m_cur_token_itr = m_tokens.begin();
    m_error = fe_no_error;
    m_result = 0.0;
    m_outbuf.clear();
    string cell_name = global::get_cell_name(m_parent_cell);
    m_outbuf << get_formula_result_output_separator() << endl;
    m_outbuf << cell_name << ": ";
    try
    {
        m_result = expression();
        m_outbuf << endl << cell_name << ": result = " << m_result << endl;
        cout << m_outbuf.str();
        return true;
    }
    catch (const invalid_expression& e)
    {
        m_outbuf << endl << cell_name << ": invalid expression: " << e.what() << endl;
        cout << m_outbuf.str();
        m_error = fe_invalid_expression;
    }
    catch (const formula_error& e)
    {
        m_outbuf << endl << "result = " << e.what() << endl;
        cout <<  m_outbuf.str();
        m_error = e.get_error();
    }
    return false;
}

double formula_interpreter::get_result() const
{
    return m_result;
}

formula_error_t formula_interpreter::get_error() const
{
    return m_error;
}

bool formula_interpreter::has_token() const
{
    return m_cur_token_itr != m_end_token_pos;
}

void formula_interpreter::next()
{
    ++m_cur_token_itr;
}

const formula_token_base& formula_interpreter::token() const
{
    return *m_cur_token_itr;
}

const formula_token_base& formula_interpreter::next_token()
{
    next();
    if (!has_token())
        throw invalid_expression("expecting a token but no more tokens found.");

    return token();
}

double formula_interpreter::expression()
{
    // <term> + <term> + <term> + ... + <term>

    double val = term();
    while (has_token())
    {
        fopcode_t oc = token().get_opcode();
        switch (oc)
        {
            case fop_plus:
            {
                m_outbuf << "+";
                next();
                double val2 = term();
                val += val2;
            }
            break;
            case fop_minus:
            {
                m_outbuf << "-";
                next();
                double val2 = term();
                val -= val2;
            }
            break;
            default:
                return val;
        }
    }
    return val;
}

double formula_interpreter::named_expression()
{
    const string& expr_name = token().get_name();
    m_outbuf << expr_name;
    const formula_cell* expr = m_context.get_named_expression(expr_name);
    if (!expr)
    {
        ostringstream os;
        os << "unable to find named expression '" << expr_name << "'";
        throw invalid_expression(os.str());
    }

    // NOTE: We need look into expanding named expressions prior to
    // interpreting formula tokens to avoid potential "recursion too deep"
    // problem.  Plus, this nested interpretation does not resolve circular
    // name dependency issue.
    formula_interpreter fi(expr, m_context);
    if (!fi.interpret())
    {
        ostringstream os;
        os << "failed to interpret named expression '" << expr_name << "'";
        throw invalid_expression(os.str());
    }
    double res = fi.get_result();
    next();
    return res;
}

double formula_interpreter::term()
{
    // <factor> || <factor> * <term>

    double val = factor();
    if (!has_token())
        return val;

    fopcode_t oc = token().get_opcode();
    switch (oc)
    {
        case fop_multiply:
            m_outbuf << "*";
            next();
            return val*term();
        case fop_divide:
        {
            m_outbuf << "/";
            next();
            double val2 = term();
            if (val2 == 0.0)
                throw formula_error(fe_division_by_zero);
            return val/val2;
        }
        default:
            ;
    }
    return val;
}

double formula_interpreter::factor()
{
    // <constant> || <variable> || <named expression> || '(' <expression> ')' || <function>

    fopcode_t oc = token().get_opcode();
    switch (oc)
    {
        case fop_open:
            return paren();
        case fop_named_expression:
            return named_expression();
        case fop_value:
            return constant();
        case fop_single_ref:
            return variable();
        case fop_function:
            return function();
        default:
        {
            ostringstream os;
            os << "factor: unexpected token type: <" << get_opcode_name(oc) << ">";
            throw invalid_expression(os.str());
        }
    }
    return 0.0;
}

double formula_interpreter::paren()
{
    m_outbuf << "(";
    next();
    double val = expression();
    if (token().get_opcode() != fop_close)
        throw invalid_expression("paren: expected close paren");

    m_outbuf << ")";
    next();
    return val;
}

double formula_interpreter::variable()
{
    const base_cell* pref = token().get_single_ref();
    if (pref == m_parent_cell)
    {
        // self-referencing is not permitted.
        throw formula_error(fe_ref_result_not_available);
    }

    double val = pref->get_value();
    m_outbuf << global::get_cell_name(pref);
    next();
    return val;
}

double formula_interpreter::constant()
{
    double val = token().get_value();
    m_outbuf << val;
    next();
    return val;
}

double formula_interpreter::function()
{
    // <func name> '(' <expression> ',' <expression> ',' ... ',' <expression> ')'
    assert(token().get_opcode() == fop_function);
    formula_function_t func_oc = formula_functions::get_function_opcode(token());
    m_outbuf << formula_functions::get_function_name(func_oc);

    if (next_token().get_opcode() != fop_open)
        throw invalid_expression("expecting a '(' after a function name.");

    m_outbuf << "(";

    vector<double> args;
    fopcode_t oc = next_token().get_opcode();
    bool expect_sep = false;
    while (oc != fop_close)
    {
        if (expect_sep)
        {
            if (oc != fop_sep)
                throw invalid_expression("argument separator is expected, but not found.");
            next();
            expect_sep = false;
            m_outbuf << ",";
        }
        else
        {
            double arg = expression();
            args.push_back(arg);
            expect_sep = true;
        }
        oc = token().get_opcode();
    }

    m_outbuf << ")";
    next();
    return formula_functions::interpret(func_oc, args);
}

}
