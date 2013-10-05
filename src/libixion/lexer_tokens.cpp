/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/lexer_tokens.hpp"

#include <iostream>
#include <sstream>

using namespace std;

namespace ixion {

namespace {

class token_printer : public unary_function<lexer_token_base, void>
{
public:
    token_printer(ostringstream& os, bool verbose) : m_os(os), m_verbose(verbose) {}
    void operator() (const lexer_token_base& r) const
    {
        lexer_opcode_t oc = r.get_opcode();
        if (m_verbose)
            m_os << "(" << get_opcode_name(oc) << ")'" << r.print() << "' ";
        else
            m_os << r.print();
    }
private:
    ostringstream& m_os;
    bool m_verbose;
};

}

string print_tokens(const lexer_tokens_t& tokens, bool verbose)
{
    ostringstream os;
    for_each(tokens.begin(), tokens.end(), token_printer(os, verbose));
    return os.str();
}

// ============================================================================

const char* get_opcode_name(lexer_opcode_t oc)
{
    switch (oc)
    {
        case op_value:      return "value";
        case op_string:     return "string";
        case op_name:       return "name";
        case op_divide:     return "divide";
        case op_minus:      return "minus";
        case op_multiply:   return "multiply";
        case op_equal:      return "equal";
        case op_less:       return "less";
        case op_greater:    return "greater";
        case op_plus:       return "plus";
        case op_open:       return "open";
        case op_close:      return "close";
        case op_sep:        return "sep";
        default:
            ;
    }
    return "";
}

// ============================================================================

lexer_token_base::lexer_token_base(lexer_opcode_t oc) :
    m_opcode(oc)
{
}

lexer_token_base::lexer_token_base(const lexer_token_base& r) :
    m_opcode(r.m_opcode)
{
}

lexer_token_base::~lexer_token_base()
{
}

lexer_opcode_t lexer_token_base::get_opcode() const
{
    return m_opcode;
}

double lexer_token_base::get_value() const
{
    return 0.0;
}

mem_str_buf lexer_token_base::get_string() const
{
    return mem_str_buf();
}

// ============================================================================

lexer_token::lexer_token(lexer_opcode_t oc) :
    lexer_token_base(oc)
{
}

lexer_token::~lexer_token()
{
}

string lexer_token::print() const
{
    switch (get_opcode())
    {
        case op_plus:
            return "+";
        case op_minus:
            return "-";
        case op_divide:
            return "/";
        case op_multiply:
            return "*";
        case op_equal:
            return "=";
        case op_less:
            return "<";
        case op_greater:
            return ">";
        case op_open:
            return "(";
        case op_close:
            return ")";
        case op_sep:
            return ",";
        case op_name:
        case op_string:
        case op_value:
        default:
            ;
    }
    return "";
}

// ============================================================================

lexer_value_token::lexer_value_token(double val) :
    lexer_token_base(op_value),
    m_val(val)
{
}

lexer_value_token::lexer_value_token(const lexer_value_token& r) :
    lexer_token_base(r),
    m_val(r.m_val)
{
}

lexer_value_token::~lexer_value_token()
{
}

double lexer_value_token::get_value() const
{
    return m_val;
}

string lexer_value_token::print() const
{
    ostringstream os;
    os << m_val;
    return os.str();
}

// ============================================================================

lexer_string_token::lexer_string_token(const char* p, size_t n) :
    lexer_token_base(op_string), m_str(p, n) {}

lexer_string_token::lexer_string_token(const lexer_string_token& r) :
    lexer_token_base(r), m_str(r.m_str) {}

lexer_string_token::~lexer_string_token() {}

mem_str_buf lexer_string_token::get_string() const
{
    return m_str;
}

string lexer_string_token::print() const
{
    return m_str.str();
}

// ============================================================================

lexer_name_token::lexer_name_token(const char* p, size_t n) :
    lexer_token_base(op_name), m_str(p, n) {}

lexer_name_token::lexer_name_token(const lexer_name_token& r) :
    lexer_token_base(r), m_str(r.m_str) {}

lexer_name_token::~lexer_name_token() {}

mem_str_buf lexer_name_token::get_string() const
{
    return m_str;
}

string lexer_name_token::print() const
{
    return m_str.str();
}

// ============================================================================

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
