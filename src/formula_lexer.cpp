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

#include "formula_lexer.hpp"

#include <iostream>

using namespace std;

namespace ixion {

// ============================================================================

class tokenizer : public ::boost::noncopyable
{
public:
    tokenizer(tokens_t& tokens, string& formula) :
        m_tokens(tokens), m_formula(formula), m_pos(0), m_size(formula.size()), m_char(0) {}

    void run();

private:
    void numeral();
    void name();
    void plus();
    void minus();
    void divide();
    void multiply();
    void open_bracket();
    void close_bracket();

private:
    tokens_t& m_tokens;
    string& m_formula;
    size_t m_pos;
    size_t m_size;
    char m_char;
};

void tokenizer::run()
{
    for (m_pos = 0; m_pos < m_size; ++m_pos)
    {
        m_char = m_formula[m_pos];
        cout << "char: '" << c << "'" << endl;
        switch (m_char)
        {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                numeral();
                break;
            case '+':
                plus();
                break;
            case '-':
                minus();
                break;
            case '/':
                divide();
                break;
            case '*':
                multiply();
                break;
            case '(':
                open_bracket();
                break;
            case ')':
                close_bracket();
                break;
            default:
                name();
        }
    }
}

void tokenizer::numeral()
{
}

void tokenizer::name()
{
}

void tokenizer::plus()
{
}

void tokenizer::minus()
{
}

void tokenizer::divide()
{
}

void tokenizer::multiply()
{
}

void tokenizer::open_bracket()
{
}

void tokenizer::close_bracket()
{
}

// ============================================================================

formula_lexer::formula_lexer(const string& formula) :
    m_formula(formula)
{
}

formula_lexer::~formula_lexer()
{
}

void formula_lexer::tokenize()
{
    cout << "formula: " << m_formula << endl;
    tokenizer tkr(m_tokens, m_formula);
    tkr.run();
}

void formula_lexer::swap_tokens(tokens_t& tokens)
{
    m_tokens.swap(tokens);
}

}
