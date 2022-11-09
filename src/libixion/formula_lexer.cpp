/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "formula_lexer.hpp"
#include "debug.hpp"
#include "ixion/global.hpp"

#include <cassert>
#include <iostream>
#include <sstream>
#include <cctype>

namespace ixion {

class tokenizer
{
    enum buffer_type {
        buf_numeral,
        buf_name
    };

public:
    tokenizer() = delete;
    tokenizer(const tokenizer&) = delete;
    tokenizer& operator= (tokenizer) = delete;

    explicit tokenizer(lexer_tokens_t& tokens, const char* p, size_t n) :
        m_tokens(tokens),
        m_sep_arg(','),
        m_sep_decimal('.'),
        mp_first(p),
        mp_char(NULL),
        m_size(n),
        m_pos(0),
        mp_char_stored(nullptr),
        m_pos_stored(0)
    {
    }

    void run();

    void set_sep_arg(char c);

private:
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
    void push_pos();
    void pop_pos();

private:
    lexer_tokens_t& m_tokens;

    char m_sep_arg;
    char m_sep_decimal;

    const char* mp_first;
    const char* mp_char;
    const size_t m_size;
    size_t m_pos;

    const char* mp_char_stored;
    size_t m_pos_stored;
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
        if (std::isdigit(*mp_char))
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
            op(lexer_opcode_t::sep);
            continue;
        }

        switch (*mp_char)
        {
            case ' ':
                space();
                break;
            case '+':
                op(lexer_opcode_t::plus);
                break;
            case '-':
                op(lexer_opcode_t::minus);
                break;
            case '/':
                op(lexer_opcode_t::divide);
                break;
            case '*':
                op(lexer_opcode_t::multiply);
                break;
            case '^':
                op(lexer_opcode_t::exponent);
                break;
            case '&':
                op(lexer_opcode_t::concat);
                break;
            case '=':
                op(lexer_opcode_t::equal);
                break;
            case '<':
                op(lexer_opcode_t::less);
                break;
            case '>':
                op(lexer_opcode_t::greater);
                break;
            case '(':
                op(lexer_opcode_t::open);
                break;
            case ')':
                op(lexer_opcode_t::close);
                break;
            case '"':
                string();
                break;
        }
    }
}

void tokenizer::set_sep_arg(char c)
{
    m_sep_arg = c;
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
        case '<':
        case '>':
        case '^':
        case '&':
            return true;
    }
    return false;
}

void tokenizer::numeral()
{
    const char* p = mp_char;
    push_pos();

    size_t len = 1;
    size_t sep_count = 0;
    for (next(); has_char(); next(), ++len)
    {
        if (*mp_char == ':')
        {
            // Treat this as a name.  This may be a part of a row-only range (e.g. 3:3).
            pop_pos();
            name();
            return;
        }

        if (std::isdigit(*mp_char))
            continue;
        if (is_decimal_sep(*mp_char) && ++sep_count <= 1)
            continue;

        break;
    }

    if (sep_count > 1)
    {
        // failed to parse this as a numeral. Treat this as a name.
        IXION_TRACE("error parsing '" << std::string(p, len) << "' as a numeral, treating it as a name.");
        pop_pos();
        name();
        return;
    }
    double val = to_double({p, len});
    m_tokens.push_back(std::make_unique<lexer_value_token>(val));
}

void tokenizer::space()
{
    // space is ignored for now.
    next();
}

void tokenizer::name()
{
    std::vector<char> scopes;

    const char* p = mp_char;
    size_t len = 0;

    for (; has_char(); next(), ++len)
    {
        char c = *mp_char;

        if (!scopes.empty() && scopes.back() == c)
        {
            scopes.pop_back();
            continue;
        }

        switch (c)
        {
            case '[':
                scopes.push_back(']');
                continue;
            case '\'':
                scopes.push_back('\'');
                continue;
        }

        if (!scopes.empty())
            continue;

        if (is_op(c))
            break;
    }

    m_tokens.push_back(std::make_unique<lexer_name_token>(p, len));
}

void tokenizer::op(lexer_opcode_t oc)
{
    m_tokens.push_back(std::make_unique<lexer_token>(oc));
    next();
}

void tokenizer::string()
{
    next();
    const char* p = mp_char;
    size_t len = 0;
    for (; *mp_char != '"' && has_char(); ++len)
        next();

    m_tokens.push_back(std::make_unique<lexer_string_token>(p, len));

    if (*mp_char == '"')
        next();
}

void tokenizer::next()
{
    ++mp_char;
    ++m_pos;
}

void tokenizer::push_pos()
{
    mp_char_stored = mp_char;
    m_pos_stored = m_pos;
}

void tokenizer::pop_pos()
{
    mp_char = mp_char_stored;
    m_pos = m_pos_stored;

    mp_char_stored = NULL;
    m_pos_stored = 0;
}

bool tokenizer::has_char() const
{
    return m_pos < m_size;
}

// ============================================================================

formula_lexer::tokenize_error::tokenize_error(const std::string& msg) : general_error(msg) {}

formula_lexer::formula_lexer(const config& config, const char* p, size_t n) :
    m_config(config), mp_first(p), m_size(n) {}

formula_lexer::~formula_lexer() {}

void formula_lexer::tokenize()
{
    tokenizer tkr(m_tokens, mp_first, m_size);
    tkr.set_sep_arg(m_config.sep_function_arg);
    tkr.run();
}

void formula_lexer::swap_tokens(lexer_tokens_t& tokens)
{
    m_tokens.swap(tokens);
}

}
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
