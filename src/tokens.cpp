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

const char* token_base::print() const
{
    switch (m_opcode)
    {
        case oc_plus:
            return "+";
        case oc_minus:
            return "-";
        case oc_divide:
            return "/";
        case oc_multiply:
            return "*";
    }
    return "";
}

// ============================================================================

value_token::value_token(double val) :
    token_base(oc_value),
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

}

