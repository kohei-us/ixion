/*************************************************************************
 *
 * Copyright (c) 2010 Kohei Yoshida
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

#include "cell.hpp"
#include "formula_interpreter.hpp"

#include <boost/thread.hpp>

#include <string>
#include <sstream>

#define DEBUG_FORMULA_CELL 0

using namespace std;

namespace ixion {

// ============================================================================

address::address()
{
}

address::~address()
{
}

// ============================================================================

base_cell::base_cell(celltype_t celltype) :
    m_celltype(celltype)
{
}

base_cell::base_cell(const base_cell& r) :
    m_celltype(r.m_celltype)
{
}

base_cell::~base_cell()
{
}

celltype_t base_cell::get_celltype() const
{
    return m_celltype;
}

// ============================================================================

string_cell::string_cell(const string& formula) :
    base_cell(celltype_string),
    m_formula(formula)
{
}

string_cell::string_cell(const string_cell& r) :
    base_cell(r),
    m_formula(r.m_formula)
{
}

string_cell::~string_cell()
{
}

const char* string_cell::print() const
{
    return m_formula.c_str();
}

// ============================================================================

formula_cell::result_cache::result_cache() : value(0.0) {}
formula_cell::result_cache::result_cache(const result_cache& r) : 
    value(r.value), text(r.text) {}

formula_cell::interpret_status::interpret_status() : 
    cell_in_computation(NULL)
{}

formula_cell::interpret_guard::interpret_guard(interpret_status& status, const formula_cell* cell) :
    m_status(status)
{
    ::boost::mutex::scoped_lock lock(m_status.mtx);
    m_status.cell_in_computation = cell;
#if DEBUG_FORMULA_CELL
    cout << "in computation" << endl;
#endif
}

formula_cell::interpret_guard::~interpret_guard()
{
    ::boost::mutex::scoped_lock lock(m_status.mtx);
    m_status.cell_in_computation = NULL;
    m_status.cond.notify_all();
#if DEBUG_FORMULA_CELL
    cout << "out of computation" << endl;
#endif
}

formula_cell::formula_cell() :
    base_cell(celltype_formula),
    m_error(fe_no_error),
    mp_result(NULL)
{
}

formula_cell::formula_cell(formula_tokens_t& tokens) :
    base_cell(celltype_formula),
    m_error(fe_no_error),
    mp_result(NULL)
{
    // Note that this will empty the passed token container !
    m_tokens.swap(tokens);
}

formula_cell::formula_cell(const formula_cell& r) :
    base_cell(r),
    m_tokens(r.m_tokens),
    m_error(r.m_error),
    mp_result(NULL)
{
    if (r.mp_result)
        mp_result = new result_cache(*r.mp_result);
}

formula_cell::~formula_cell()
{
    delete mp_result;
}

double formula_cell::get_value() const
{
    wait_for_interpreted_result();

    if (m_error != fe_no_error)
        // Error condition.
        throw formula_error(m_error);

    if (!mp_result)
        // Result not cached yet.  Reference error.
        throw formula_error(fe_ref_result_not_available);

    return mp_result->value;
}

const char* formula_cell::print() const
{
    return print_tokens(m_tokens, false);
}

const formula_tokens_t& formula_cell::get_tokens() const
{
    return m_tokens;
}

void formula_cell::interpret(const cell_ptr_name_map_t& cell_ptr_name_map)
{
    interpret_guard guard(m_interpret_status, this);

    formula_interpreter fin(this, cell_ptr_name_map);
    if (fin.interpret())
        set_result(fin.get_result());
    else
        set_error(fin.get_error());
}

void formula_cell::swap_tokens(formula_tokens_t& tokens)
{
    m_tokens.swap(tokens);
}

void formula_cell::wait_for_interpreted_result() const
{
    ::boost::mutex::scoped_lock lock(m_interpret_status.mtx);
    while (m_interpret_status.cell_in_computation)
    {
#if DEBUG_FORMULA_CELL
        cout << "waiting" << endl;
#endif
        m_interpret_status.cond.wait(lock);
    }
}

void formula_cell::set_result(double result)
{
    if (!mp_result)
        mp_result = new result_cache;

    mp_result->value = result;
    mp_result->text.clear();
}

void formula_cell::set_error(formula_error_t error)
{
    m_error = error;
}

}
