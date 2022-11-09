/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <ixion/formula_tokens.hpp>
#include <ixion/exceptions.hpp>
#include <ixion/global.hpp>
#include <ixion/macros.hpp>

namespace ixion {

std::string_view get_opcode_name(fopcode_t oc)
{
    // Make sure the names are ordered identically to the ordering of the enum members.
    static const std::string_view names[] = {
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

    if (std::size_t(oc) >= std::size(names))
        return "???";

    return names[oc];
}

std::string_view get_formula_opcode_string(fopcode_t oc)
{
    static const char* empty = "";

    // Make sure the names are ordered identically to the ordering of the enum members.
    static const std::string_view names[] = {
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

    if (std::size_t(oc) >= std::size(names))
        return empty;

    return names[oc];
}

// ============================================================================

formula_token::formula_token(fopcode_t op) :
    opcode(op)
{
}

formula_token::formula_token(const address_t& addr) :
    opcode(fop_single_ref), value(addr)
{
}

formula_token::formula_token(const range_t& range) :
    opcode(fop_range_ref), value(range)
{
}

formula_token::formula_token(const table_t& table) :
    opcode(fop_table_ref), value(table)
{
}

formula_token::formula_token(formula_function_t func) :
    opcode(fop_function), value(func)
{
}

formula_token::formula_token(double v) :
    opcode(fop_value), value(v)
{
}

formula_token::formula_token(string_id_t sid) :
    opcode(fop_string), value(sid)
{
}

formula_token::formula_token(std::string name) :
    opcode(fop_named_expression), value(std::move(name))
{
}


formula_token::formula_token(const formula_token& r) = default;
formula_token::formula_token(formula_token&& r) = default;
formula_token::~formula_token() = default;

bool formula_token::operator== (const formula_token& r) const
{
    return opcode == r.opcode && value == r.value;
}

bool formula_token::operator!= (const formula_token& r) const
{
    return !operator== (r);
}

struct formula_tokens_store::impl
{
    formula_tokens_t m_tokens;
    size_t m_refcount;

    impl() : m_refcount(0) {}
};

formula_tokens_store::formula_tokens_store() :
    mp_impl(std::make_unique<impl>())
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
    switch (ft.opcode)
    {
        case fop_string:
        {
            os << "string token: (identifier=" << std::get<string_id_t>(ft.value) << ")";
            break;
        }
        case fop_value:
        {
            os << "value token: " << std::get<double>(ft.value);
            break;
        }
        case fop_single_ref:
        {
            os << "single ref token: " << std::get<address_t>(ft.value);
            break;
        }
        case fop_range_ref:
        {
            os << "range ref token: " << std::get<range_t>(ft.value);
            break;
        }
        case fop_table_ref:
        {
            os << "table ref token: " << std::get<table_t>(ft.value);
            break;
        }
        case fop_named_expression:
        {
            os << "named expression token: '" << std::get<std::string>(ft.value) << "'";
            break;
        }
        case fop_function:
        {
            using _int_type = std::underlying_type_t<formula_function_t>;

            formula_function_t v = std::get<formula_function_t>(ft.value);
            os << "function token: (opcode=" << _int_type(v) << "; name='" << get_formula_function_name(v) << "')";
            break;
        }
        case fop_plus:
        case fop_minus:
        case fop_divide:
        case fop_multiply:
        case fop_exponent:
        case fop_concat:
        case fop_equal:
        case fop_not_equal:
        case fop_less:
        case fop_greater:
        case fop_less_equal:
        case fop_greater_equal:
        case fop_open:
        case fop_close:
        case fop_sep:
        case fop_error:
        case fop_unknown:
            os << "opcode token: (name=" << get_opcode_name(ft.opcode) << "; s='"
                << get_formula_opcode_string(ft.opcode) << "')";
            break;
    }

    return os;
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
