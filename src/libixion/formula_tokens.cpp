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

#include "ixion/formula_tokens.hpp"

using ::std::string;

namespace ixion {

const char* get_opcode_name(fopcode_t oc)
{
    switch (oc)
    {
        case fop_close:
            return "close";
        case fop_divide:
            return "divide";
        case fop_err_no_ref:
            return "error no ref";
        case fop_minus:
            return "minus";
        case fop_multiply:
            return "multiply";
        case fop_open:
            return "open";
        case fop_plus:
            return "plus";
        case fop_sep:
            return "separator";
        case fop_single_ref:
            return "single ref";
        case fop_range_ref:
            return "range ref";
        case fop_named_expression:
            return "named expression";
        case fop_string:
            return "string";
        case fop_value:
            return "value";
        case fop_function:
            return "function";
        default:
            ;
    }
    return "unknown";
}

const char* get_formula_opcode_string(fopcode_t oc)
{
    switch (oc)
    {
        case fop_close:
            return ")";
        case fop_divide:
            return "/";
        case fop_minus:
            return "-";
        case fop_multiply:
            return "*";
        case fop_open:
            return "(";
        case fop_plus:
            return "+";
        case fop_sep:
            return ",";
        case fop_string:
        case fop_value:
        case fop_function:
        case fop_err_no_ref:
        case fop_single_ref:
        case fop_range_ref:
        case fop_named_expression:
        default:
            ;
    }
    return "";
}

// ============================================================================

formula_token_base::formula_token_base(fopcode_t op) :
    m_opcode(op)
{
}

formula_token_base::formula_token_base(const formula_token_base& r) :
    m_opcode(r.m_opcode)
{
}

formula_token_base::~formula_token_base()
{
}

fopcode_t formula_token_base::get_opcode() const
{
    return m_opcode;
}

address_t formula_token_base::get_single_ref() const
{
    return address_t();
}

range_t formula_token_base::get_range_ref() const
{
    return range_t();
}

double formula_token_base::get_value() const
{
    return 0.0;
}

size_t formula_token_base::get_index() const
{
    return 0;
}

std::string formula_token_base::get_name() const
{
    return std::string();
}

// ============================================================================

opcode_token::opcode_token(fopcode_t oc) :
    formula_token_base(oc)
{
}

opcode_token::opcode_token(const opcode_token& r) :
    formula_token_base(r)
{
}

opcode_token::~opcode_token()
{
}

// ============================================================================

value_token::value_token(double value) :
    formula_token_base(fop_value),
    m_value(value)
{
}

value_token::~value_token()
{
}

double value_token::get_value() const
{
    return m_value;
}

// ============================================================================

single_ref_token::single_ref_token(const address_t& addr) :
    formula_token_base(fop_single_ref),
    m_address(addr)
{
}

single_ref_token::single_ref_token(const single_ref_token& r) :
    formula_token_base(r),
    m_address(r.m_address)
{
}

single_ref_token::~single_ref_token()
{
}

address_t single_ref_token::get_single_ref() const
{
    return m_address;
}

// ============================================================================

range_ref_token::range_ref_token(const range_t& range) :
    formula_token_base(fop_range_ref),
    m_range(range)
{
}

range_ref_token::range_ref_token(const range_ref_token& r) :
    formula_token_base(r),
    m_range(r.m_range)
{
}

range_ref_token::~range_ref_token()
{
}

range_t range_ref_token::get_range_ref() const
{
    return m_range;
}

// ============================================================================

named_exp_token::named_exp_token(const char* p, size_t n) :
    formula_token_base(fop_named_expression),
    m_name(p, n) {}

named_exp_token::named_exp_token(const named_exp_token& r) :
    formula_token_base(r),
    m_name(r.m_name) {}

named_exp_token::~named_exp_token() {}

string named_exp_token::get_name() const
{
    return m_name;
}

// ============================================================================

function_token::function_token(size_t func_oc) :
    formula_token_base(fop_function),
    m_func_oc(func_oc)
{
}

function_token::function_token(const function_token& r) :
    formula_token_base(r),
    m_func_oc(r.m_func_oc)
{
}

function_token::~function_token()
{
}

size_t function_token::get_index() const
{
    return m_func_oc;
}

}
