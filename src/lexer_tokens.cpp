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

#include "lexer_tokens.hpp"

#include <iostream>
#include <sstream>

using namespace std;

namespace ixion {

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

string lexer_token_base::get_string() const
{
    return string();
}

// ============================================================================

lexer_token::lexer_token(lexer_opcode_t oc) :
    lexer_token_base(oc)
{
}

lexer_token::~lexer_token()
{
}

const char* lexer_token::print() const
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
        case op_open:
            return "(";
        case op_close:
            return ")";
        case op_name:
        case op_sep:
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

const char* lexer_value_token::print() const
{
    ostringstream os;
    os << m_val;
    return os.str().c_str();
}

// ============================================================================

lexer_string_token::lexer_string_token(const string& str) :
    lexer_token_base(op_string),
    m_str(str)
{
}

lexer_string_token::~lexer_string_token()
{
}

string lexer_string_token::get_string() const
{
    return m_str;
}

const char* lexer_string_token::print() const
{
    return m_str.c_str();
}

// ============================================================================

lexer_name_token::lexer_name_token(const string& name) :
    lexer_token_base(op_name),
    m_name(name)
{
}

lexer_name_token::~lexer_name_token()
{
}

string lexer_name_token::get_string() const
{
    return m_name;
}

const char* lexer_name_token::print() const
{
    return m_name.c_str();
}

// ============================================================================

}

