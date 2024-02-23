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
#include <unordered_map>

namespace ixion {

namespace {

const std::unordered_map<char, lexer_opcode_t> ops_map = {
    { '&', lexer_opcode_t::concat },
    { '(', lexer_opcode_t::open },
    { ')', lexer_opcode_t::close },
    { '*', lexer_opcode_t::multiply },
    { '+', lexer_opcode_t::plus },
    { '-', lexer_opcode_t::minus },
    { '/', lexer_opcode_t::divide },
    { '<', lexer_opcode_t::less },
    { '=', lexer_opcode_t::equal },
    { '>', lexer_opcode_t::greater },
    { '^', lexer_opcode_t::exponent },
    { '{', lexer_opcode_t::array_open },
    { '}', lexer_opcode_t::array_close },
};

} // anonymous namespace

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

    explicit tokenizer(const char* p, size_t n) :
        mp_first(p),
        m_size(n)
    {
    }

    void run();
    lexer_tokens_t pop_tokens();

    void set_sep_arg(char c);

private:
    bool is_arg_sep(char c) const;
    bool is_array_row_sep(char c) const;
    bool is_decimal_sep(char c) const;
    bool is_op(char c) const;

    void init();

    void numeral();
    void space();
    void name();
    void op(lexer_opcode_t oc);
    void string();
    void error();

    bool has_char() const;
    void next();
    void push_pos();
    void pop_pos();

private:
    lexer_tokens_t m_tokens;

    char m_sep_arg = ',';
    char m_sep_array_row = ';';
    char m_sep_decimal = '.';

    const char* mp_first = nullptr;
    const char* mp_char = nullptr;
    const std::size_t m_size = 0;
    std::size_t m_pos = 0;

    const char* mp_char_stored = nullptr;
    std::size_t m_pos_stored = 0;
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

        if (auto it = ops_map.find(*mp_char); it != ops_map.end())
        {
            op(it->second);
            continue;
        }

        switch (*mp_char)
        {
            case ' ':
                space();
                continue;
            case '"':
                string();
                continue;
            case '#':
                error();
                continue;
        }

        if (is_arg_sep(*mp_char))
        {
            op(lexer_opcode_t::sep);
            continue;
        }

        if (is_array_row_sep(*mp_char))
        {
            op(lexer_opcode_t::array_row_sep);
            continue;
        }

        name();
    }
}

lexer_tokens_t tokenizer::pop_tokens()
{
    return std::move(m_tokens);
}

void tokenizer::set_sep_arg(char c)
{
    m_sep_arg = c;
}

bool tokenizer::is_arg_sep(char c) const
{
    return c == m_sep_arg;
}

bool tokenizer::is_array_row_sep(char c) const
{
    return c == m_sep_array_row;
}

bool tokenizer::is_decimal_sep(char c) const
{
    return c == m_sep_decimal;
}

bool tokenizer::is_op(char c) const
{
    if (is_arg_sep(c))
        return true;

    if (ops_map.count(c) > 0)
        return true;

    switch (*mp_char)
    {
        case ' ':
        case '"':
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
    m_tokens.emplace_back(val);
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

    m_tokens.emplace_back(lexer_opcode_t::name, std::string_view{p, len});
}

void tokenizer::op(lexer_opcode_t oc)
{
    m_tokens.emplace_back(oc);
    next();
}

void tokenizer::string()
{
    next();
    const char* p = mp_char;
    size_t len = 0;
    for (; *mp_char != '"' && has_char(); ++len)
        next();

    m_tokens.emplace_back(lexer_opcode_t::string, std::string_view{p, len});

    if (*mp_char == '"')
        next();
}

void tokenizer::error()
{
    const char* p0 = mp_char;
    next(); // skip '#'
    std::size_t len = 1;

    // TODO: re-implement this using mdds::trie_map later
    // https://gitlab.com/mdds/mdds/-/issues/95

    bool found = false;

    for (; !found && has_char(); next(), ++len)
    {
        if (*mp_char == '!' || *mp_char == '?')
            found = true;

        if (*mp_char == 'A' && len == 3 && *(mp_char - 1) == '/' && *(mp_char - 2) == 'N')
        {
            // This is '#N/A'
            found = true;
        }
    }

    if (found)
    {
        m_tokens.emplace_back(lexer_opcode_t::error, std::string_view{p0, len});
        return;
    }

    std::ostringstream os;
    os << "failed to parse an error token in lexer tokenizer: '" << std::string_view{p0, len} << "'";
    throw general_error(os.str());
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

formula_lexer::formula_lexer(const config& conf, const char* p, size_t n) :
    m_config(conf), mp_first(p), m_size(n) {}

formula_lexer::~formula_lexer() {}

void formula_lexer::tokenize()
{
    tokenizer tkr(mp_first, m_size);
    tkr.set_sep_arg(m_config.sep_function_arg);
    tkr.run();
    m_tokens = tkr.pop_tokens();
}

void formula_lexer::swap_tokens(lexer_tokens_t& tokens)
{
    m_tokens.swap(tokens);
}

}
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
