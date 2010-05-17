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

#define DEBUG_LEXER 0

using namespace std;

namespace ixion {

// ============================================================================

class tokenizer : public ::boost::noncopyable
{
public:
    tokenizer(lexer_tokens_t& tokens, string& formula) :
        m_tokens(tokens), 
        m_formula(formula), 
        m_buffer_type(buf_name), 
        m_pos(0), 
        m_size(formula.size()), 
        m_char(0),
        m_csep(',')
    {
    }

    void run();

private:
    void numeral();
    void space();
    void name();
    void plus();
    void minus();
    void divide();
    void multiply();
    void dot();
    void sep();
    void open_bracket();
    void close_bracket();

    void flush_buffer();

private:
    enum buffer_t {
        buf_numeral,
        buf_name
    };

    lexer_tokens_t& m_tokens;
    string& m_formula;

    vector<char> m_buffer;
    buffer_t m_buffer_type;
    
    size_t m_pos;
    size_t m_size;
    char   m_char;
    
    // data that influence tokenization behavior.
    char m_csep; // function argument separator
};

void tokenizer::run()
{
    for (m_pos = 0; m_pos < m_size; ++m_pos)
    {
        m_char = m_formula[m_pos];
#if DEBUG_LEXER
        cout << "char: '" << m_char << "'" << endl;
#endif
        if (m_char == m_csep)
        {
            sep();
            continue;
        }

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
            case ' ':
                space();
                break;
            case '.':
                dot();
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

    flush_buffer();
}

void tokenizer::numeral()
{
    if (m_buffer.empty())
        m_buffer_type = buf_numeral;

    m_buffer.push_back(m_char);
}

void tokenizer::space()
{
    // space is ignored for now.
}

void tokenizer::name()
{
    if (m_buffer_type != buf_name)
    {    
        flush_buffer();
        m_buffer_type = buf_name;
    }

    m_buffer.push_back(m_char);
}

void tokenizer::plus()
{
    flush_buffer();
    m_tokens.push_back(new lexer_token(op_plus));
}

void tokenizer::minus()
{
    flush_buffer();
    m_tokens.push_back(new lexer_token(op_minus));
}

void tokenizer::divide()
{
    flush_buffer();
    m_tokens.push_back(new lexer_token(op_divide));
}

void tokenizer::multiply()
{
    flush_buffer();
    m_tokens.push_back(new lexer_token(op_multiply));
}

void tokenizer::dot()
{
}

void tokenizer::sep()
{
    flush_buffer();
    m_tokens.push_back(new lexer_token(op_sep));
}

void tokenizer::open_bracket()
{
    flush_buffer();
    m_tokens.push_back(new lexer_token(op_open));
}

void tokenizer::close_bracket()
{
    flush_buffer();
    m_tokens.push_back(new lexer_token(op_close));
}

void tokenizer::flush_buffer()
{
    if (m_buffer.empty())
        return;

    m_buffer.push_back(0);
    switch (m_buffer_type)
    {
        case buf_numeral:
        {
            double val = strtod(&m_buffer[0], NULL);
            m_tokens.push_back(new lexer_value_token(val));
        }
        break;
        case buf_name:
        {
            string str = &m_buffer[0];
            m_tokens.push_back(new lexer_name_token(str));
        }
        break;
        default:
            throw formula_lexer::tokenize_error("unknown buffer type");
    }
    m_buffer.clear();
}

// ============================================================================

formula_lexer::tokenize_error::tokenize_error(const string& msg) :
    m_msg(msg)
{
}

formula_lexer::tokenize_error::~tokenize_error() throw()
{
}

const char* formula_lexer::tokenize_error::what() const throw()
{
    return m_msg.c_str();
}

formula_lexer::formula_lexer(const string& formula) :
    m_formula(formula)
{
}

formula_lexer::~formula_lexer()
{
}

void formula_lexer::tokenize()
{
#if DEBUG_LEXER
    cout << "formula string: '" << m_formula << "'" << endl;
#endif        
    tokenizer tkr(m_tokens, m_formula);
    tkr.run();
}

void formula_lexer::swap_tokens(lexer_tokens_t& tokens)
{
    m_tokens.swap(tokens);
}

}
