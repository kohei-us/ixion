/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "lexer_tokens.hpp"

#include <iostream>
#include <sstream>
#include <algorithm>

using namespace std;

namespace ixion {

std::string print_tokens(const lexer_tokens_t& tokens, bool verbose)
{
    std::ostringstream os;

    for (const auto& t : tokens)
    {
        if (verbose)
            os << "(" << get_opcode_name(t.opcode) << ")'" << t << "' ";
        else
            os << t;
    }

    return os.str();
}

const char* get_opcode_name(lexer_opcode_t oc)
{
    switch (oc)
    {
        case lexer_opcode_t::value:
            return "value";
        case lexer_opcode_t::string:
            return "string";
        case lexer_opcode_t::name:
            return "name";
        case lexer_opcode_t::divide:
            return "divide";
        case lexer_opcode_t::minus:
            return "minus";
        case lexer_opcode_t::multiply:
            return "multiply";
        case lexer_opcode_t::exponent:
            return "exponent";
        case lexer_opcode_t::concat:
            return "concat";
        case lexer_opcode_t::equal:
            return "equal";
        case lexer_opcode_t::less:
            return "less";
        case lexer_opcode_t::greater:
            return "greater";
        case lexer_opcode_t::plus:
            return "plus";
        case lexer_opcode_t::open:
            return "open";
        case lexer_opcode_t::close:
            return "close";
        case lexer_opcode_t::sep:
            return "sep";
        case lexer_opcode_t::array_open:
            return "array-open";
        case lexer_opcode_t::array_close:
            return "array-close";
    }
    return "";
}

lexer_token::lexer_token(lexer_opcode_t _opcode) :
    opcode(_opcode)
{
}

lexer_token::lexer_token(lexer_opcode_t _opcode, std::string_view _value) :
    opcode(_opcode), value(_value)
{
}

lexer_token::lexer_token(double _value) :
    opcode(lexer_opcode_t::value), value(_value)
{
}


std::ostream& operator<<(std::ostream& os, const lexer_token& t)
{
    switch (t.opcode)
    {
        case lexer_opcode_t::plus:
            os << '+';
            break;
        case lexer_opcode_t::minus:
            os << '-';
            break;
        case lexer_opcode_t::divide:
            os << '/';
            break;
        case lexer_opcode_t::multiply:
            os << '*';
            break;
        case lexer_opcode_t::exponent:
            os << '^';
            break;
        case lexer_opcode_t::concat:
            os << '&';
            break;
        case lexer_opcode_t::equal:
            os << '=';
            break;
        case lexer_opcode_t::less:
            os << '<';
            break;
        case lexer_opcode_t::greater:
            os << '>';
            break;
        case lexer_opcode_t::open:
            os << '(';
            break;
        case lexer_opcode_t::close:
            os << ')';
            break;
        case lexer_opcode_t::array_open:
            os << '{';
            break;
        case lexer_opcode_t::array_close:
            os << '}';
            break;
        case lexer_opcode_t::sep:
            os << ',';
            break;
        case lexer_opcode_t::name:
        case lexer_opcode_t::string:
            os << std::get<std::string_view>(t.value);
            break;
        case lexer_opcode_t::value:
            os << std::get<double>(t.value);
            break;
    }

    return os;
}

} // namespace ixion

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
