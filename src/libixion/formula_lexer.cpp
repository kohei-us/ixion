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
#include "ixion/global.hpp"

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
        m_pos(0)
    {
    }

    void run();

private:
    static bool is_digit(char c);
    bool is_arg_sep(char c) const;
    bool is_decimal_sep(char c) const;
    bool is_op(char c) const;

    void init();

    void numeral();
    void space();
    void name();
    void op(lexer_opcode_t oc);
    void string();

    bool has_char() const;
    void next();

private:
    lexer_tokens_t& m_tokens;

    char m_sep_arg;
    char m_sep_decimal;

    const char* mp_first;
    const char* mp_char;
    const size_t m_size;
    size_t m_pos;
};

void tokenizer::init()
{
    m_tokens.clear();
    mp_char = mp_first;
    m_pos = 0;
}

void tokenizer::run()
{
    if (!m_size)
        // Nothing to do.
        return;

    init();

    while (has_char())
    {
        if (is_digit(*mp_char))
        {
            numeral();
            continue;
        }

        if (!is_op(*mp_char))
        {
            name();
            continue;
        }

        if (is_arg_sep(*mp_char))
        {
            op(op_sep);
            continue;
        }

        switch (*mp_char)
        {
            case ' ':
                space();
                break;
            case '+':
                op(op_plus);
                break;
            case '-':
                op(op_minus);
                break;
            case '/':
                op(op_divide);
                break;
            case '*':
                op(op_multiply);
                break;
            case '=':
                op(op_equal);
                break;
            case '(':
                op(op_open);
                break;
            case ')':
                op(op_close);
                break;
            case '"':
                string();
                break;
        }
    }
}

bool tokenizer::is_digit(char c)
{
    return ('0' <= c && c <= '9');
}

bool tokenizer::is_arg_sep(char c) const
{
    return c == m_sep_arg;
}

bool tokenizer::is_decimal_sep(char c) const
{
    return c == m_sep_decimal;
}

bool tokenizer::is_op(char c) const
{
    if (is_arg_sep(c))
        return true;

    switch (*mp_char)
    {
        case ' ':
        case '+':
        case '-':
        case '/':
        case '*':
        case '(':
        case ')':
        case '"':
        case '=':
            return true;
    }
    return false;
}

void tokenizer::numeral()
{
    const char* p = mp_char;
    size_t len = 1;
    size_t sep_count = 0;
    for (next(); has_char(); next(), ++len)
    {
        if (is_digit(*mp_char))
            continue;
        if (is_decimal_sep(*mp_char) && ++sep_count <= 1)
            continue;
        break;
    }

    if (sep_count > 1)
    {
        ostringstream os;
        os << "error parsing numeral: " << std::string(p, len);
        throw formula_lexer::tokenize_error(os.str());
    }
    double val = global::to_double(p, len);
    m_tokens.push_back(new lexer_value_token(val));
}

void tokenizer::space()
{
    // space is ignored for now.
    next();
}

void tokenizer::name()
{
    const char* p = mp_char;
    size_t len = 1;
    for (next(); has_char(); next(), ++len)
    {
        if (is_op(*mp_char))
            break;
    }

    m_tokens.push_back(new lexer_name_token(p, len));
}

void tokenizer::op(lexer_opcode_t oc)
{
    m_tokens.push_back(new lexer_token(oc));
    next();
}

void tokenizer::string()
{
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
    __IXION_DEBUG_OUT__ << "formula string: '" << std::string(mp_first, m_size) << "'" << endl;
#endif
    tokenizer tkr(m_tokens, mp_first, m_size);
    tkr.run();
#if IXION_DEBUG_LEXER
    __IXION_DEBUG_OUT__ << print_tokens(m_tokens, true) << endl;
#endif
}

void formula_lexer::swap_tokens(lexer_tokens_t& tokens)
{
    m_tokens.swap(tokens);
}

}
