/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/lexer_tokens.hpp"

#include <iostream>
#include <sstream>
#include <algorithm>

using namespace std;

namespace ixion {

namespace {

class token_printer : public unary_function<unique_ptr<lexer_token_base>, void>
{
public:
    token_printer(ostringstream& os, bool verbose) : m_os(os), m_verbose(verbose) {}
    void operator() (const unique_ptr<lexer_token_base>& r) const
    {
        lexer_opcode_t oc = r->get_opcode();
        if (m_verbose)
            m_os << "(" << get_opcode_name(oc) << ")'" << r->print() << "' ";
        else
            m_os << r->print();
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
        case lexer_opcode_t::value:      return "value";
        case lexer_opcode_t::string:     return "string";
        case lexer_opcode_t::name:       return "name";
        case lexer_opcode_t::divide:     return "divide";
        case lexer_opcode_t::minus:      return "minus";
        case lexer_opcode_t::multiply:   return "multiply";
        case lexer_opcode_t::equal:      return "equal";
        case lexer_opcode_t::less:       return "less";
        case lexer_opcode_t::greater:    return "greater";
        case lexer_opcode_t::plus:       return "plus";
        case lexer_opcode_t::open:       return "open";
        case lexer_opcode_t::close:      return "close";
        case lexer_opcode_t::sep:        return "sep";
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
        case lexer_opcode_t::plus:
            return "+";
        case lexer_opcode_t::minus:
            return "-";
        case lexer_opcode_t::divide:
            return "/";
        case lexer_opcode_t::multiply:
            return "*";
        case lexer_opcode_t::equal:
            return "=";
        case lexer_opcode_t::less:
            return "<";
        case lexer_opcode_t::greater:
            return ">";
        case lexer_opcode_t::open:
            return "(";
        case lexer_opcode_t::close:
            return ")";
        case lexer_opcode_t::sep:
            return ",";
        case lexer_opcode_t::name:
        case lexer_opcode_t::string:
        case lexer_opcode_t::value:
        default:
            ;
    }
    return "";
}

// ============================================================================

lexer_value_token::lexer_value_token(double val) :
    lexer_token_base(lexer_opcode_t::value),
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
    lexer_token_base(lexer_opcode_t::string), m_str(p, n) {}

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
    lexer_token_base(lexer_opcode_t::name), m_str(p, n) {}

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
