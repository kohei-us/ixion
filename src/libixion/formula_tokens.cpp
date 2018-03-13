/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/formula_tokens.hpp"
#include "ixion/exceptions.hpp"

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
        case fop_equal:
            return "=";
        case fop_not_equal:
            return "<>";
        case fop_less:
            return "<";
        case fop_less_equal:
            return "<=";
        case fop_greater:
            return ">";
        case fop_greater_equal:
            return ">=";
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

formula_token::formula_token(fopcode_t op) :
    m_opcode(op)
{
}

formula_token::formula_token(const formula_token& r) :
    m_opcode(r.m_opcode)
{
}

formula_token::~formula_token()
{
}

fopcode_t formula_token::get_opcode() const
{
    return m_opcode;
}

bool formula_token::operator== (const formula_token& r) const
{
    if (m_opcode != r.m_opcode)
        return false;

    switch (m_opcode)
    {
        case fop_close:
        case fop_divide:
        case fop_minus:
        case fop_multiply:
        case fop_open:
        case fop_plus:
        case fop_sep:
            return true;
        case fop_single_ref:
            return get_single_ref() == r.get_single_ref();
        case fop_range_ref:
            return get_range_ref() == r.get_range_ref();
        case fop_named_expression:
            return get_name() == r.get_name();
        case fop_string:
            return get_index() == r.get_index();
        case fop_value:
            return get_value() == r.get_value();
        case fop_function:
            return get_index() == r.get_index();
        default:
            ;
    }
    return false;
}

bool formula_token::operator!= (const formula_token& r) const
{
    return !operator== (r);
}

address_t formula_token::get_single_ref() const
{
    return address_t();
}

range_t formula_token::get_range_ref() const
{
    return range_t();
}

table_t formula_token::get_table_ref() const
{
    return table_t();
}

double formula_token::get_value() const
{
    return 0.0;
}

size_t formula_token::get_index() const
{
    return 0;
}

std::string formula_token::get_name() const
{
    return std::string();
}

void formula_token::write_string(std::ostream& /*os*/) const
{
}

struct formula_tokens_store::impl
{
    formula_tokens_t m_tokens;
    size_t m_refcount;

    impl() : m_refcount(0) {}
};

formula_tokens_store::formula_tokens_store() : mp_impl(ixion::make_unique<impl>()) {}
formula_tokens_store::~formula_tokens_store() {}

void formula_tokens_store::add_ref()
{
    ++mp_impl->m_refcount;
}

void formula_tokens_store::release_ref()
{
    if (--mp_impl->m_refcount == 0)
        delete this;
}

bool operator== (const formula_tokens_t& left, const formula_tokens_t& right)
{
    size_t n = left.size();
    if (n != right.size())
        return false;

    formula_tokens_t::const_iterator itr = left.begin(), itr_end = left.end(), itr2 = right.begin();
    for (; itr != itr_end; ++itr, ++itr2)
    {
        if (*itr != *itr2)
            return false;
    }
    return true;
}

std::ostream& operator<< (std::ostream& os, const formula_token& ft)
{
    ft.write_string(os);
    return os;
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
