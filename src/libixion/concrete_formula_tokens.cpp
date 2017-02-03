/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "concrete_formula_tokens.hpp"
#include "ixion/formula_function_opcode.hpp"

namespace ixion {

// ============================================================================

opcode_token::opcode_token(fopcode_t oc) :
    formula_token(oc)
{
}

opcode_token::opcode_token(const opcode_token& r) :
    formula_token(r)
{
}

opcode_token::~opcode_token()
{
}

void opcode_token::write_string(std::ostream& os) const
{
    os << "opcode token: '";
    os << get_formula_opcode_string(get_opcode());
    os << "'";
}

// ============================================================================

value_token::value_token(double value) :
    formula_token(fop_value),
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

void value_token::write_string(std::ostream& os) const
{
    os << "value token: " << m_value;
}

string_token::string_token(size_t str_identifier) :
    formula_token(fop_string),
    m_str_identifier(str_identifier) {}

string_token::~string_token() {}

size_t string_token::get_index() const
{
    return m_str_identifier;
}

void string_token::write_string(std::ostream& os) const
{
    os << "string token: (identifier=" << m_str_identifier << ")";
}

// ============================================================================

single_ref_token::single_ref_token(const address_t& addr) :
    formula_token(fop_single_ref),
    m_address(addr)
{
}

single_ref_token::single_ref_token(const single_ref_token& r) :
    formula_token(r),
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

void single_ref_token::write_string(std::ostream& os) const
{
    os << "single ref token: " << m_address;
}

// ============================================================================

range_ref_token::range_ref_token(const range_t& range) :
    formula_token(fop_range_ref),
    m_range(range)
{
}

range_ref_token::range_ref_token(const range_ref_token& r) :
    formula_token(r),
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

void range_ref_token::write_string(std::ostream& os) const
{
    os << "range ref token: " << m_range;
}

table_ref_token::table_ref_token(const table_t& table) :
    formula_token(fop_table_ref),
    m_table(table) {}

table_ref_token::table_ref_token(const table_ref_token& r) :
    formula_token(r),
    m_table(r.m_table) {}

table_ref_token::~table_ref_token() {}

table_t table_ref_token::get_table_ref() const
{
    return m_table;
}

void table_ref_token::write_string(std::ostream& os) const
{
    os << "table ref token: " << "TODO";
}

named_exp_token::named_exp_token(const char* p, size_t n) :
    formula_token(fop_named_expression),
    m_name(p, n) {}

named_exp_token::named_exp_token(const named_exp_token& r) :
    formula_token(r),
    m_name(r.m_name) {}

named_exp_token::~named_exp_token() {}

std::string named_exp_token::get_name() const
{
    return m_name;
}

void named_exp_token::write_string(std::ostream& os) const
{
    os << "named expression token: '" << m_name << "'";
}

// ============================================================================

function_token::function_token(size_t func_oc) :
    formula_token(fop_function),
    m_func_oc(func_oc)
{
}

function_token::function_token(const function_token& r) :
    formula_token(r),
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

void function_token::write_string(std::ostream& os) const
{
    os << "function token: (opcode=" << m_func_oc << "; name='"
        << get_formula_function_name(static_cast<formula_function_t>(m_func_oc))
        << "')";
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
