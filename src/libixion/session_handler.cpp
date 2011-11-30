/*************************************************************************
 *
 * Copyright (c) 2011 Kohei Yoshida
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

#include "ixion/session_handler.hpp"
#include "ixion/formula_name_resolver.hpp"
#include "ixion/formula_functions.hpp"
#include "ixion/formula_result.hpp"

#include <string>
#include <iostream>

using namespace std;

namespace ixion {

session_handler::session_handler(const model_context& cxt) :
    m_context(cxt) {}

session_handler::~session_handler() {}

void session_handler::begin_cell_interpret(const formula_cell *p)
{
    m_cell_name = "<unknown cell>";
    const string* name = m_context.get_named_expression_name(p);
    if (name)
        m_cell_name = *name;
    else
        m_cell_name = m_context.get_cell_name(p);

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
    cout << m_context.get_name_resolver().get_name(addr, pos, false);
}

void session_handler::push_range_ref(const range_t& range, const abs_address_t& pos)
{
    cout << m_context.get_name_resolver().get_name(range, pos, false);
}

void session_handler::push_function(formula_function_t foc)
{
    cout << formula_functions::get_function_name(foc);
}

}
