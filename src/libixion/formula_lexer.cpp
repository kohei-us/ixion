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

#include "ixion/formula_lexer.hpp"

#include <iostream>
#include <sstream>

#define IXION_DEBUG_LEXER 0

using namespace std;

namespace ixion {

class tokenizer : public ::boost::noncopyable
{
    enum buffer_type {
        buf_numeral,
        buf_name
    };

public:
    explicit tokenizer(lexer_tokens_t& tokens, const char* p, size_t n) :
        m_tokens(tokens),
        m_sep_arg(','),
        m_sep_decimal('.'),
        mp_first(p),
        mp_char(NULL),
        m_size(n),
        m_pos(0),
        m_buf_type(buf_name)
    {
    }

    void run();

private:
    static bool is_digit(char c);

    void init();

    void numeral();
    void space();
    void name();
    void plus();
    void minus();
    void divide();
    void multiply();
    void sep_arg();
    void open_bracket();
    void close_bracket();
    void string();

    bool has_char() const;
    void next();
    void push_back();
    void flush_buffer();

private:
    lexer_tokens_t& m_tokens;

    char m_sep_arg;
    char m_sep_decimal;

    const char* mp_first;
    const char* mp_char;
    size_t m_size;
    size_t m_pos;

    mem_str_buf m_buf;
    buffer_type m_buf_type;
};

void tokenizer::init()
{
    mp_char = mp_first;
    m_pos = 0;
}

void tokenizer::run()
{
    m_tokens.clear();

    if (!m_size)
        // Nothing to do.
        return;

    init();

    while (m_pos < m_size)
    {
        if (*mp_char == m_sep_arg)
        {
            sep_arg();
            continue;
        }

        switch (*mp_char)
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
            case '"':
                string();
                break;
            default:
                name();
        }
    }

    flush_buffer();
}

bool tokenizer::is_digit(char c)
{
    return ('0' <= c && c <= '9');
}

void tokenizer::numeral()
{
    if (m_buf.empty())
        m_buf_type = buf_numeral;

    size_t sep_count = 0;
    while (true)
    {
        push_back();
        next();

        if (is_digit(*mp_char))
            continue;
        if (*mp_char == m_sep_decimal && ++sep_count <= 1)
            continue;
        break;
    }

    if (sep_count > 1 && m_buf_type == buf_numeral)
    {
        ostringstream os;
        os << "error parsing numeral: " << m_buf.str();
        throw formula_lexer::tokenize_error(os.str());
    }
}

void tokenizer::space()
{
    flush_buffer();

    // space is ignored for now.
    next();
}

void tokenizer::name()
{
    if (m_buf_type != buf_name)
    {
        flush_buffer();
        m_buf_type = buf_name;
    }

    push_back();
    next();
}

void tokenizer::plus()
{
    flush_buffer();
    m_tokens.push_back(new lexer_token(op_plus));
    next();
}

void tokenizer::minus()
{
    flush_buffer();
    m_tokens.push_back(new lexer_token(op_minus));
    next();
}

void tokenizer::divide()
{
    flush_buffer();
    m_tokens.push_back(new lexer_token(op_divide));
    next();
}

void tokenizer::multiply()
{
    flush_buffer();
    m_tokens.push_back(new lexer_token(op_multiply));
    next();
}

void tokenizer::sep_arg()
{
    flush_buffer();
    m_tokens.push_back(new lexer_token(op_sep));
    next();
}

void tokenizer::open_bracket()
{
    flush_buffer();
    m_tokens.push_back(new lexer_token(op_open));
    next();
}

void tokenizer::close_bracket()
{
    flush_buffer();
    m_tokens.push_back(new lexer_token(op_close));
    next();
}

void tokenizer::string()
{
    flush_buffer();
    next();
    const char* p = mp_char;
    size_t len = 0;
    for (; *mp_char != '"' && has_char(); ++len)
        next();

    if (len)
        m_tokens.push_back(new lexer_string_token(p, len));

    if (*mp_char == '"')
        next();
}

void tokenizer::next()
{
    ++mp_char;
    ++m_pos;
}

bool tokenizer::has_char() const
{
    return m_pos < m_size;
}

void tokenizer::push_back()
{
    if (m_buf.empty())
        m_buf.set_start(mp_char);
    else
        m_buf.inc();
}

void tokenizer::flush_buffer()
{
    if (m_buf.empty())
        return;

    switch (m_buf_type)
    {
        case buf_numeral:
        {
            std::string s = m_buf.str();
            double val = strtod(s.c_str(), NULL);
            m_tokens.push_back(new lexer_value_token(val));
        }
        break;
        case buf_name:
        {
            m_tokens.push_back(new lexer_name_token(m_buf.get(), m_buf.size()));
        }
        break;
        default:
            throw formula_lexer::tokenize_error("unknown buffer type");
    }
    m_buf.clear();
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

formula_lexer::formula_lexer(const char* p, size_t n) :
    mp_first(p), m_size(n) {}

formula_lexer::~formula_lexer() {}

void formula_lexer::tokenize()
{
#if IXION_DEBUG_LEXER
    cout << "formula string: '" << std::string(mp_first, m_size) << "'" << endl;
#endif
    tokenizer tkr(m_tokens, mp_first, m_size);
    tkr.run();
}

void formula_lexer::swap_tokens(lexer_tokens_t& tokens)
{
    m_tokens.swap(tokens);
}

}
