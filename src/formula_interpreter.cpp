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
#include "global.hpp"

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

formula_interpreter::formula_interpreter(const cell_name_ptr_map_t& cell_map, const formula_tokens_t& tokens) :
    m_cell_name_ptr_map(cell_map),
    m_tokens(tokens),
    m_end_token_pos(m_tokens.end())
{
}

formula_interpreter::~formula_interpreter()
{
}

void formula_interpreter::interpret()
{
    m_cur_token_itr = m_tokens.begin();
    try
    {
        double result = expression();
        cout << endl;
        cout << "result = " << result << endl;
    }
    catch (const invalid_expression& e)
    {
        cout << endl;
        cout << "invalid expression: " << e.what() << endl;
    }
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

double formula_interpreter::expression()
{
    // <term> || <term> + <expression>

    double val = term();
    if (!has_token())
        return val;

    fopcode_t oc = token().get_opcode();
    if (oc == fop_plus || oc == fop_minus)
    {
        plus_op();
        double right = expression();
        if (oc == fop_plus)
            return val + right;
        else if (oc == fop_minus)
            return val - right;
    }
    return val;
}

double formula_interpreter::term()
{
    // <factor> || <factor> * <term>

    double val = factor();
    if (!has_token())
        return val;

    fopcode_t oc = token().get_opcode();
    if (oc == fop_multiply || oc == fop_divide)
    {
        multiply_op();
        double right = term();
        if (oc == fop_multiply)
            return val*right;
        else if (oc == fop_divide)
            return val/right;
    }
    return val;
}

double formula_interpreter::factor()
{
    // <constant> || <variable> || '(' <expression> ')'

    fopcode_t oc = token().get_opcode();
    switch (oc)
    {
        case fop_open:
            return paren();
        case fop_value:
            return constant();
        case fop_single_ref:
            return variable();
        default:
        {
            ostringstream os;
            os << "factor: unexpected token type ";
            os << oc;
            throw invalid_expression(os.str());
        }
    }
    return 0.0;
}

double formula_interpreter::paren()
{
    cout << "(";
    next();
    double val = expression();
    if (token().get_opcode() != fop_close)
        throw invalid_expression("paren: expected close paren");

    cout << ")";
    next();
    return val;
}

double formula_interpreter::variable()
{
    double val = token().get_value();
    cout << token().get_single_ref();
    next();
    return val;
}

double formula_interpreter::constant()
{
    double val = token().get_value();
    cout << val;
    next();
    return val;
}

void formula_interpreter::plus_op()
{
    const formula_token_base& t = token();
    switch (t.get_opcode())
    {
        case fop_plus:
            cout << "+";
        break;
        case fop_minus:
            cout << "-";
        break;
        default:
        {
            ostringstream os;
            os << "plus_op: unexpected token type " << t.get_opcode();
            throw invalid_expression(os.str());
        }
    }
    next();
}

void formula_interpreter::multiply_op()
{
    const formula_token_base& t = token();
    switch (t.get_opcode())
    {
        case fop_multiply:
            cout << "*";
        break;
        case fop_divide:
            cout << "/";
        break;
        default:
        {
            ostringstream os;
            os << "multiply_op: unexpected token type " << t.get_opcode();
            throw invalid_expression(os.str());
        }
    }
    next();
}

}
