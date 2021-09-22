/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "formula_parser.hpp"
#include "ixion/formula_name_resolver.hpp"
#include "ixion/interface/formula_model_access.hpp"

#include "formula_functions.hpp"
#include "concrete_formula_tokens.hpp"

#include <iostream>
#include <sstream>

using namespace std;

namespace ixion {

namespace {

class ref_error : public general_error
{
public:
    ref_error(const string& msg) :
        general_error(msg) {}
};

}

// ----------------------------------------------------------------------------

formula_parser::parse_error::parse_error(const string& msg) :
    general_error(msg) {}

// ----------------------------------------------------------------------------

formula_parser::formula_parser(
    const lexer_tokens_t& tokens, iface::formula_model_access& cxt, const formula_name_resolver& resolver) :
    m_itr_cur(tokens.end()),
    m_itr_end(tokens.end()),
    m_tokens(tokens),
    m_context(cxt),
    m_resolver(resolver)
{
}

formula_parser::~formula_parser()
{
}

void formula_parser::set_origin(const abs_address_t& pos)
{
    m_pos = pos;
}

void formula_parser::parse()
{
    for (m_itr_cur = m_tokens.begin(); has_token(); next())
    {
        const lexer_token_base& t = get_token();
        lexer_opcode_t oc = t.get_opcode();
        switch (oc)
        {
            case lexer_opcode_t::open:
            case lexer_opcode_t::close:
            case lexer_opcode_t::plus:
            case lexer_opcode_t::minus:
            case lexer_opcode_t::multiply:
            case lexer_opcode_t::exponent:
            case lexer_opcode_t::concat:
            case lexer_opcode_t::equal:
            case lexer_opcode_t::divide:
            case lexer_opcode_t::sep:
                primitive(oc);
                break;
            case lexer_opcode_t::name:
                name(t);
                break;
            case lexer_opcode_t::string:
                literal(t);
                break;
            case lexer_opcode_t::value:
                value(t);
                break;
            case lexer_opcode_t::less:
                less(t);
                break;
            case lexer_opcode_t::greater:
                greater(t);
                break;
            default:
                ;
        }
    }
}

void formula_parser::print_tokens() const
{
}

formula_tokens_t& formula_parser::get_tokens()
{
    return m_formula_tokens;
}

void formula_parser::primitive(lexer_opcode_t oc)
{
    fopcode_t foc = fop_unknown;
    switch (oc)
    {
        case lexer_opcode_t::close:
            foc = fop_close;
            break;
        case lexer_opcode_t::divide:
            foc = fop_divide;
            break;
        case lexer_opcode_t::minus:
            foc = fop_minus;
            break;
        case lexer_opcode_t::multiply:
            foc = fop_multiply;
            break;
        case lexer_opcode_t::exponent:
            foc = fop_exponent;
            break;
        case lexer_opcode_t::concat:
            foc = fop_concat;
            break;
        case lexer_opcode_t::equal:
            foc = fop_equal;
            break;
        case lexer_opcode_t::open:
            foc = fop_open;
            break;
        case lexer_opcode_t::plus:
            foc = fop_plus;
            break;
        case lexer_opcode_t::sep:
            foc = fop_sep;
            break;
        default:
            throw parse_error("unknown primitive token received");
    }
    m_formula_tokens.push_back(make_unique<opcode_token>(foc));
}

void formula_parser::name(const lexer_token_base& t)
{
    mem_str_buf name = t.get_string();

    formula_name_t fn = m_resolver.resolve({name.get(), name.size()}, m_pos);

    switch (fn.type)
    {
        case formula_name_t::cell_reference:
            m_formula_tokens.push_back(
                make_unique<single_ref_token>(std::get<address_t>(fn.value)));
            break;
        case formula_name_t::range_reference:
        {
            m_formula_tokens.push_back(
                make_unique<range_ref_token>(std::get<range_t>(fn.value)));
            break;
        }
        case formula_name_t::table_reference:
        {
            table_t table;
            formula_name_t::table_type src_table = std::get<formula_name_t::table_type>(fn.value);
            table.name = m_context.add_string(src_table.name);
            table.column_first = m_context.add_string(src_table.column_first);
            table.column_last = m_context.add_string(src_table.column_last);
            table.areas = src_table.areas;
            m_formula_tokens.push_back(make_unique<table_ref_token>(table));
            break;
        }
        case formula_name_t::function:
            m_formula_tokens.push_back(make_unique<function_token>(std::get<formula_function_t>(fn.value)));
            break;
        case formula_name_t::named_expression:
            m_formula_tokens.push_back(
                make_unique<named_exp_token>(name.get(), name.size()));
            break;
        default:
        {
            std::ostringstream os;
            os << "failed to resolve a name token '" << name.str() << "'.";
            throw parse_error(os.str());
        }
    }
}

void formula_parser::literal(const lexer_token_base& t)
{
    mem_str_buf s = t.get_string();
    string_id_t sid = m_context.add_string({s.get(), s.size()});
    m_formula_tokens.push_back(make_unique<string_token>(sid));
}

void formula_parser::value(const lexer_token_base& t)
{
    double val = t.get_value();
    m_formula_tokens.push_back(make_unique<value_token>(val));
}

void formula_parser::less(const lexer_token_base& t)
{
    if (has_next())
    {
        next();
        switch (get_token().get_opcode())
        {
            case lexer_opcode_t::equal:
                m_formula_tokens.push_back(make_unique<opcode_token>(fop_less_equal));
                return;
            case lexer_opcode_t::greater:
                m_formula_tokens.push_back(make_unique<opcode_token>(fop_not_equal));
                return;
            default:
                ;
        }
        prev();
    }
    m_formula_tokens.push_back(make_unique<opcode_token>(fop_less));
}

void formula_parser::greater(const lexer_token_base& t)
{
    if (has_next())
    {
        next();
        if (get_token().get_opcode() == lexer_opcode_t::equal)
        {
            m_formula_tokens.push_back(make_unique<opcode_token>(fop_greater_equal));
            return;
        }
        prev();
    }
    m_formula_tokens.push_back(make_unique<opcode_token>(fop_greater));

}

const lexer_token_base& formula_parser::get_token() const
{
    return **m_itr_cur;
}

bool formula_parser::has_token() const
{
    return m_itr_cur != m_itr_end;
}

bool formula_parser::has_next() const
{
    return (m_itr_cur+1) != m_itr_end;
}

void formula_parser::next()
{
    ++m_itr_cur;
}

void formula_parser::prev()
{
    --m_itr_cur;
}

}
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
