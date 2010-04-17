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
    m_tokens(tokens)
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
        expression();
    }
    catch (const invalid_expression& e)
    {
        cout << "invalid expression: " << e.what() << endl;
    }
}

bool formula_interpreter::has_next() const
{
    return m_cur_token_itr != m_tokens.end();
}

const formula_token_base& formula_interpreter::next_token()
{
    if (!has_next())
        throw invalid_expression("no more token present, but expected one");

    const formula_token_base& t = *m_cur_token_itr;
    ++m_cur_token_itr;
    return t;
}

void formula_interpreter::expression()
{
    // <term> || <term> + <expression>
    term();
    if (has_next())
    {
        plus_op();
        expression();
    }
}

void formula_interpreter::term()
{
    // <factor> || <factor> * <term>
    factor();
    if (has_next())
    {
        multiply_op();
        term();
    }
}

void formula_interpreter::factor()
{
    // <constant> || <variable> || '(' <expression> ')'
    const formula_token_base& t1 = next_token();
    fopcode_t oc1 = t1.get_opcode();
    if (oc1 == fop_open)
    {
        expression();
        const formula_token_base& t2 = next_token();
        if (t2.get_opcode() != fop_close)
            throw invalid_expression("factor: expected close paren");
    }
    else if (oc1 == fop_value)
    {
    }
    else if (oc1 == fop_single_ref)
    {
    }
    else
        throw invalid_expression("factor: unexpected token type");
}

void formula_interpreter::variable()
{
}

void formula_interpreter::constant()
{
}

void formula_interpreter::plus_op()
{
    const formula_token_base& t = next_token();
}

void formula_interpreter::multiply_op()
{
    const formula_token_base& t = next_token();
}

}
