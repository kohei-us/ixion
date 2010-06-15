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
#include <iostream>

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

formula_cell::result_cache::result_cache() : value(0.0), error(fe_no_error) {}
formula_cell::result_cache::result_cache(const result_cache& r) : 
    value(r.value), text(r.text), error(r.error) {}

formula_cell::interpret_status::interpret_status() : 
    result(NULL)
{}

formula_cell::interpret_status::interpret_status(const interpret_status& r) :
    result(NULL)
{
    if (r.result)
        result = new result_cache(*r.result);
}

formula_cell::interpret_status::~interpret_status()
{
    delete result;
}

// ============================================================================

formula_cell::interpret_guard::interpret_guard(interpret_status& status, const string& cell_name) :
    m_status(status),
    m_cell_name(cell_name)
{
    ::boost::mutex::scoped_lock lock(m_status.mtx);
#if DEBUG_FORMULA_CELL
    ostringstream os;
    os << m_cell_name << " locked" << endl;
    cout << os.str();
#endif
}

formula_cell::interpret_guard::~interpret_guard()
{
    ::boost::mutex::scoped_lock lock(m_status.mtx);
#if DEBUG_FORMULA_CELL
    ostringstream os;
    os << m_cell_name << " unlocked" << endl;
    cout << os.str();
#endif
}

// ============================================================================

formula_cell::formula_cell() :
    base_cell(celltype_formula),
    m_circular_safe(false)
{
}

formula_cell::formula_cell(formula_tokens_t& tokens) :
    base_cell(celltype_formula),
    m_circular_safe(false)
{
    // Note that this will empty the passed token container !
    m_tokens.swap(tokens);
}

formula_cell::formula_cell(const formula_cell& r) :
    base_cell(r),
    m_tokens(r.m_tokens),
    m_interpret_status(r.m_interpret_status),
    m_circular_safe(r.m_circular_safe)
{
}

formula_cell::~formula_cell()
{
}

double formula_cell::get_value() const
{
    ::boost::mutex::scoped_lock lock(m_interpret_status.mtx);
#if DEBUG_FORMULA_CELL
    ostringstream os;
#endif

    wait_for_interpreted_result(lock);

    if (!m_interpret_status.result)
        // Result not cached yet.  Reference error.
        throw formula_error(fe_ref_result_not_available);

    if (m_interpret_status.result->error != fe_no_error)
        // Error condition.
        throw formula_error(m_interpret_status.result->error);

#if DEBUG_FORMULA_CELL
    os << "returning value of " << m_interpret_status.result->value << endl;
    cout << os.str();
#endif
    return m_interpret_status.result->value;
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
    string cell_name = get_cell_name(cell_ptr_name_map, this);
    interpret_guard guard(m_interpret_status, cell_name);

    if (m_interpret_status.result)
    {
        // When the result is already cached before the cell is interpreted, 
        // it can only mean the cell has circular dependency.
        assert(m_interpret_status.result->error != fe_no_error);
        ostringstream os;
        os << get_formula_result_output_separator() << endl;
        os << cell_name << ": result = " << get_formula_error_name(m_interpret_status.result->error) << endl;
        cout << os.str();
        return;
    }

#if DEBUG_FORMULA_CELL
    ostringstream os;
    os << cell_name << " is being interpreted" << endl;
    cout << os.str();
    os.clear();
#endif

    formula_interpreter fin(this, cell_ptr_name_map);
    result_cache result;
    if (fin.interpret())
    {
        // Successful interpretation.
        result.value = fin.get_result();
        result.text.clear();
    }
    else
    {
        // Interpretation ended with an error condition.
        result.error = fin.get_error();
    }

    // The result pointer being non-NULL indicates the availability of the 
    // interpretation result.  So this must be done atomically.
    m_interpret_status.result = new result_cache(result);
    m_interpret_status.cond.notify_all();

#if DEBUG_FORMULA_CELL
    os << cell_name << " interpretation finished" << endl;
    cout << os.str();
    os.clear();
#endif
}

bool formula_cell::is_circular_safe() const
{
    return m_circular_safe;
}

void formula_cell::check_circular()
{
    // TODO: Check to make sure this is being run on the main thread only.

    formula_tokens_t::iterator itr = m_tokens.begin(), itr_end = m_tokens.end();
    for (; itr != itr_end; ++itr)
    {
        fopcode_t op = itr->get_opcode();
        if (op != fop_single_ref)
            continue;

        const base_cell* ref = itr->get_single_ref();
        if (ref->get_celltype() != celltype_formula)
            continue;

        if (!static_cast<const formula_cell*>(ref)->is_circular_safe())
        {
            // Circular dependency detected !!
#if DEBUG_FORMULA_CELL
            cout << "circular dependency detected !!" << endl;
#endif
            assert(!m_interpret_status.result);
            m_interpret_status.result = new result_cache;
            m_interpret_status.result->error = fe_ref_result_not_available;
            return;
        }
    }

    // No circular dependencies.  Good.
    m_circular_safe = true;
}

void formula_cell::reset(const cell_ptr_name_map_t& cell_ptr_name_map)
{
    string cell_name = get_cell_name(cell_ptr_name_map, this);
    interpret_guard guard(m_interpret_status, cell_name);
    delete m_interpret_status.result;
    m_interpret_status.result = NULL;
    m_circular_safe = false;
}

void formula_cell::swap_tokens(formula_tokens_t& tokens)
{
    m_tokens.swap(tokens);
}

void formula_cell::wait_for_interpreted_result(::boost::mutex::scoped_lock& lock) const
{
#if DEBUG_FORMULA_CELL
    ostringstream os;
    os << "wait for result" << endl;
    cout << os.str();
#endif
    while (!m_interpret_status.result)
    {
#if DEBUG_FORMULA_CELL
        os.clear();
        os << "waiting" << endl;
        cout << os.str();
#endif
        m_interpret_status.cond.wait(lock);
    }
#if DEBUG_FORMULA_CELL
    os.clear();
    os << "done waiting for result" << endl;
    cout << os.str();
#endif
}

}
