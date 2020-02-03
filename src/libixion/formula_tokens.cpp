/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/formula_tokens.hpp"
#include "ixion/exceptions.hpp"
#include "ixion/global.hpp"

using ::std::string;

namespace ixion {

const char* get_opcode_name(fopcode_t oc)
{
    // Make sure the names are ordered identically to the ordering of the enum members.
    static const std::vector<const char*> names = {
        "unknown", // fop_unknown
        "single ref", // fop_single_ref
        "range ref", // fop_range_ref
        "table ref", // fop_table_ref
        "named expression", // fop_named_expression
        "string", // fop_string
        "value", // fop_value
        "function", // fop_function
        "plus", // fop_plus
        "minus", // fop_minus
        "divide", // fop_divide
        "multiply", // fop_multiply
        "exponent", // fop_exponent
        "concat", // fop_concat
        "equal", // fop_equal
        "not equal", // fop_not_equal
        "less", // fop_less
        "greater", // fop_greater
        "less equal", // fop_less_equal
        "greater equal", // fop_greater_equal
        "open", // fop_open
        "close", // fop_close
        "sep", // fop_sep
        "error", // fop_error
    };

    if (size_t(oc) >= names.size())
        return "???";

    return names[oc];
}

const char* get_formula_opcode_string(fopcode_t oc)
{
    static const char* empty = "";

    // Make sure the names are ordered identically to the ordering of the enum members.
    static const std::vector<const char*> names = {
        empty, // fop_unknown
        empty, // fop_single_ref
        empty, // fop_range_ref
        empty, // fop_table_ref
        empty, // fop_named_expression
        empty, // fop_string
        empty, // fop_value
        empty, // fop_function
        "+", // fop_plus
        "-", // fop_minus
        "/", // fop_divide
        "*", // fop_multiply
        "^", // fop_exponent
        "&", // fop_concat
        "=", // fop_equal
        "<>", // fop_not_equal
        "<", // fop_less
        ">", // fop_greater
        "<=", // fop_less_equal
        ">=", // fop_greater_equal
        "(", // fop_open
        ")", // fop_close
        empty, // fop_sep
        empty, // fop_error
    };

    if (size_t(oc) >= names.size())
        return empty;

    return names[oc];
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
        case fop_exponent:
        case fop_concat:
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

formula_tokens_store::formula_tokens_store() :
    mp_impl(ixion::make_unique<impl>())
{
}

formula_tokens_store::~formula_tokens_store()
{
}

formula_tokens_store_ptr_t formula_tokens_store::create()
{
    return formula_tokens_store_ptr_t(new formula_tokens_store);
}

void formula_tokens_store::add_ref()
{
    ++mp_impl->m_refcount;
}

void formula_tokens_store::release_ref()
{
    if (--mp_impl->m_refcount == 0)
        delete this;
}

size_t formula_tokens_store::get_reference_count() const
{
    return mp_impl->m_refcount;
}

formula_tokens_t& formula_tokens_store::get()
{
    return mp_impl->m_tokens;
}

const formula_tokens_t& formula_tokens_store::get() const
{
    return mp_impl->m_tokens;
}

named_expression_t::named_expression_t() {}
named_expression_t::named_expression_t(const abs_address_t& _origin, formula_tokens_t _tokens) :
    origin(_origin), tokens(std::move(_tokens)) {}

named_expression_t::named_expression_t(named_expression_t&& other) :
    origin(other.origin), tokens(std::move(other.tokens)) {}

named_expression_t::~named_expression_t() {}

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
