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

#include "tokens.hpp"

#include <iostream>
#include <sstream>

using namespace std;

namespace ixion {

// ============================================================================

const char* get_opcode_name(opcode_t oc)
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

token_base::token_base(opcode_t oc) :
    m_opcode(oc)
{
}

token_base::token_base(const token_base& r) :
    m_opcode(r.m_opcode)
{
}

token_base::~token_base()
{
}

opcode_t token_base::get_opcode() const
{
    return m_opcode;
}

double token_base::get_value() const
{
    return 0.0;
}

string token_base::get_string() const
{
    return string();
}

// ============================================================================

token::token(opcode_t oc) :
    token_base(oc)
{
}

token::~token()
{
}

const char* token::print() const
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
    }
    return "";
}

// ============================================================================

value_token::value_token(double val) :
    token_base(op_value),
    m_val(val) 
{
}

value_token::value_token(const value_token& r) :
    token_base(r),
    m_val(r.m_val) 
{
}

value_token::~value_token()
{
}

double value_token::get_value() const
{
    return m_val;
}

const char* value_token::print() const
{
    ostringstream os;
    os << m_val;
    return os.str().c_str();
}

// ============================================================================

string_token::string_token(const string& str) :
    token_base(op_string),
    m_str(str)
{
}

string_token::~string_token()
{
}

string string_token::get_string() const
{
    return m_str;
}

const char* string_token::print() const
{
    return m_str.c_str();
}

// ============================================================================

name_token::name_token(const string& name) :
    token_base(op_name),
    m_name(name)
{
}

name_token::~name_token()
{
}

string name_token::get_string() const
{
    return m_name;
}

const char* name_token::print() const
{
    return m_name.c_str();
}

// ============================================================================

}

