/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "session_handler.hpp"
#include "app_common.hpp"

#include "ixion/formula_name_resolver.hpp"
#include "ixion/formula_result.hpp"
#include "ixion/formula_tokens.hpp"
#include "ixion/config.hpp"

#include <string>
#include <iostream>
#include <sstream>
#include <mutex>

using namespace std;

namespace ixion {

session_handler::factory::factory(const model_context& cxt) :
    m_context(cxt), m_show_sheet_name(false) {}

session_handler::factory::~factory() {}

std::unique_ptr<iface::session_handler> session_handler::factory::create()
{
    return std::make_unique<session_handler>(m_context, m_show_sheet_name);
}

void session_handler::factory::show_sheet_name(bool b)
{
    m_show_sheet_name = b;
}

struct session_handler::impl
{
    const model_context& m_context;
    std::unique_ptr<formula_name_resolver> mp_resolver;
    std::string m_cell_name;
    std::ostringstream m_buf;
    bool m_show_sheet_name;

    impl(const model_context& cxt, bool show_sheet_name) :
        m_context(cxt),
        mp_resolver(formula_name_resolver::get(formula_name_resolver_t::excel_a1, &cxt)),
        m_show_sheet_name(show_sheet_name) {}
};

session_handler::session_handler(const model_context& cxt, bool show_sheet_name) :
    mp_impl(std::make_unique<impl>(cxt, show_sheet_name)) {}

session_handler::~session_handler() {}

void session_handler::begin_cell_interpret(const abs_address_t& pos)
{
    // Convert absolute to relative address, which looks better when printed.
    address_t pos_display(pos);
    pos_display.set_absolute(false);
    mp_impl->m_cell_name = mp_impl->mp_resolver->get_name(pos_display, abs_address_t(), mp_impl->m_show_sheet_name);

    mp_impl->m_buf << detail::get_formula_result_output_separator() << endl;
    mp_impl->m_buf << mp_impl->m_cell_name << ": ";
}

void session_handler::end_cell_interpret()
{
    print(mp_impl->m_buf.str());
}

void session_handler::set_result(const formula_result& result)
{
    mp_impl->m_buf << std::endl << mp_impl->m_cell_name << ": result = ";

    switch (result.get_type())
    {
        case formula_result::result_type::string:
        {
            constexpr char quote = '\'';
            mp_impl->m_buf << quote << result.str(mp_impl->m_context) << quote;
            break;
        }
        default:
            mp_impl->m_buf << result.str(mp_impl->m_context);
    }

    mp_impl->m_buf << " [" << result.get_type() << ']' << std::endl;
}

void session_handler::set_invalid_expression(std::string_view msg)
{
    mp_impl->m_buf << endl << mp_impl->m_cell_name << ": invalid expression: " << msg << endl;
}

void session_handler::set_formula_error(std::string_view msg)
{
    mp_impl->m_buf << endl << mp_impl->m_cell_name << ": result = " << msg << endl;
}

void session_handler::push_token(fopcode_t fop)
{
    switch (fop)
    {
        case fop_sep:
        {
            mp_impl->m_buf << mp_impl->m_context.get_config().sep_function_arg;
            break;
        }
        case fop_array_row_sep:
        {
            mp_impl->m_buf << mp_impl->m_context.get_config().sep_matrix_row;
            break;
        }
        default:
            mp_impl->m_buf << get_formula_opcode_string(fop);
    }
}

void session_handler::push_value(double val)
{
    mp_impl->m_buf << val;
}

void session_handler::push_error(formula_error_t err)
{
    mp_impl->m_buf << err;
}

void session_handler::push_string(size_t sid)
{
    const string* p = mp_impl->m_context.get_string(sid);
    mp_impl->m_buf << '"';
    if (p)
        mp_impl->m_buf << *p;
    else
        mp_impl->m_buf << "(null string)";
    mp_impl->m_buf << '"';
}

void session_handler::push_single_ref(const address_t& addr, const abs_address_t& pos)
{
    mp_impl->m_buf << mp_impl->mp_resolver->get_name(addr, pos, false);
}

void session_handler::push_range_ref(const range_t& range, const abs_address_t& pos)
{
    mp_impl->m_buf << mp_impl->mp_resolver->get_name(range, pos, false);
}

void session_handler::push_table_ref(const table_t& table)
{
    mp_impl->m_buf << mp_impl->mp_resolver->get_name(table);
}

void session_handler::push_function(formula_function_t foc)
{
    mp_impl->m_buf << get_formula_function_name(foc);
}

void session_handler::print(const std::string& msg)
{
    static std::mutex mtx;
    std::lock_guard<std::mutex> lock(mtx);
    cout << msg;
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

