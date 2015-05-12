/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "session_handler.hpp"
#include "ixion/formula_name_resolver.hpp"
#include "ixion/formula_result.hpp"
#include "ixion/formula_tokens.hpp"

#include <string>
#include <iostream>

using namespace std;

namespace ixion {

session_handler::session_handler(const model_context& cxt) :
    m_context(cxt),
    mp_resolver(formula_name_resolver::get(formula_name_resolver_t::excel_a1, &cxt)) {}

session_handler::~session_handler() {}

void session_handler::begin_cell_interpret(const abs_address_t& pos)
{
    // Convert absolute to relative address, which looks better when printed.
    address_t pos_display(pos);
    pos_display.set_absolute(false);
    m_cell_name = mp_resolver->get_name(pos_display, abs_address_t(), false);

    cout << get_formula_result_output_separator() << endl;
    cout << m_cell_name << ": ";
}

void session_handler::set_result(const formula_result& result)
{
    cout << endl << m_cell_name << ": result = " << result.str(m_context) << endl;
}

void session_handler::set_invalid_expression(const char* msg)
{
    cout << endl << m_cell_name << ": invalid expression: " << msg << endl;
}

void session_handler::set_formula_error(const char* msg)
{
    cout << endl << m_cell_name << ": result = " << msg << endl;
}

void session_handler::push_token(fopcode_t fop)
{
    cout << get_formula_opcode_string(fop);
}

void session_handler::push_value(double val)
{
    cout << val;
}

void session_handler::push_string(size_t sid)
{
    const string* p = m_context.get_string(sid);
    cout << '"';
    if (p)
        cout << *p;
    else
        cout << "(null string)";
    cout << '"';
}

void session_handler::push_single_ref(const address_t& addr, const abs_address_t& pos)
{
    cout << mp_resolver->get_name(addr, pos, false);
}

void session_handler::push_range_ref(const range_t& range, const abs_address_t& pos)
{
    cout << mp_resolver->get_name(range, pos, false);
}

void session_handler::push_table_ref(const table_t& table)
{
    cout << mp_resolver->get_name(table);
}

void session_handler::push_function(formula_function_t foc)
{
    cout << get_formula_function_name(foc);
}

}
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
