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
#include <mutex>

using namespace std;

namespace ixion {

session_handler::factory::factory(const model_context& cxt) : m_context(cxt) {}

std::unique_ptr<iface::session_handler> session_handler::factory::create()
{
    return ixion::make_unique<session_handler>(m_context);
}

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

    m_buf << get_formula_result_output_separator() << endl;
    m_buf << m_cell_name << ": ";
}

void session_handler::end_cell_interpret()
{
    print(m_buf.str());
}

void session_handler::set_result(const formula_result& result)
{
    m_buf << endl << m_cell_name << ": result = " << result.str(m_context) << endl;
}

void session_handler::set_invalid_expression(const char* msg)
{
    m_buf << endl << m_cell_name << ": invalid expression: " << msg << endl;
}

void session_handler::set_formula_error(const char* msg)
{
    m_buf << endl << m_cell_name << ": result = " << msg << endl;
}

void session_handler::push_token(fopcode_t fop)
{
    m_buf << get_formula_opcode_string(fop);
}

void session_handler::push_value(double val)
{
    m_buf << val;
}

void session_handler::push_string(size_t sid)
{
    const string* p = m_context.get_string(sid);
    m_buf << '"';
    if (p)
        m_buf << *p;
    else
        m_buf << "(null string)";
    m_buf << '"';
}

void session_handler::push_single_ref(const address_t& addr, const abs_address_t& pos)
{
    m_buf << mp_resolver->get_name(addr, pos, false);
}

void session_handler::push_range_ref(const range_t& range, const abs_address_t& pos)
{
    m_buf << mp_resolver->get_name(range, pos, false);
}

void session_handler::push_table_ref(const table_t& table)
{
    m_buf << mp_resolver->get_name(table);
}

void session_handler::push_function(formula_function_t foc)
{
    m_buf << get_formula_function_name(foc);
}

void session_handler::print(const std::string& msg)
{
    static std::mutex mtx;
    std::lock_guard<std::mutex> lock(mtx);
    cout << msg;
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

