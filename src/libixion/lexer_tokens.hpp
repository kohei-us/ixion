/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_LEXER_TOKENS_HPP
#define INCLUDED_IXION_LEXER_TOKENS_HPP

#include <ixion/env.hpp>

#include <vector>
#include <memory>
#include <variant>
#include <string_view>

namespace ixion {

enum class lexer_opcode_t
{
    // data types
    value,
    string,
    name,
    error,

    // arithmetic operators
    plus,
    minus,
    divide,
    multiply,
    exponent,

    // string operators
    concat,

    // relational operators
    equal,
    less,
    greater,

    // parentheses, separators
    open,
    close,
    sep,
    array_open,
    array_close,
    array_row_sep,
};

const char* get_lexer_opcode_name(lexer_opcode_t oc);

struct lexer_token
{
    using value_type = std::variant<double, std::string_view>;

    lexer_opcode_t opcode;
    value_type value;

    lexer_token(lexer_opcode_t _opcode);
    lexer_token(lexer_opcode_t _opcode, std::string_view _value);
    lexer_token(double _value);
};

std::ostream& operator<<(std::ostream& os, const lexer_token& t);

using lexer_tokens_t = std::vector<lexer_token>;

std::string print_tokens(const lexer_tokens_t& tokens, bool verbose);

} // namespace ixion

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
