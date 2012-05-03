/*************************************************************************
 *
 * Copyright (c) 2010, 2011 Kohei Yoshida
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

#include "ixion/cell.hpp"
#include "ixion/exceptions.hpp"
#include "ixion/formula_interpreter.hpp"
#include "ixion/formula_result.hpp"
#include "ixion/cell_listener_tracker.hpp"
#include "ixion/interface/model_context.hpp"
#include "ixion/interface/session_handler.hpp"

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>

#include <string>
#include <sstream>
#include <iostream>

#define DEBUG_FORMULA_CELL 0
#if DEBUG_FORMULA_CELL
#include "ixion/formula_name_resolver.hpp"
#endif


#define FORMULA_CIRCULAR_SAFE 0x000001
#define FORMULA_SHARED_TOKENS 0x000002

using namespace std;

namespace ixion {

namespace {

class ref_token_picker : public unary_function<formula_token_base, void>
{
public:
    ref_token_picker() :
        mp_tokens(new vector<const formula_token_base*>()) {}

    ref_token_picker(const ref_token_picker& r) :
        mp_tokens(r.mp_tokens) {}

    void operator() (const formula_token_base& t)
    {
        switch (t.get_opcode())
        {
            case fop_single_ref:
            case fop_range_ref:
                mp_tokens->push_back(&t);
            break;
            default:
                ; // ignore the rest.
        }
    }

    void swap_tokens(vector<const formula_token_base*>& dest)
    {
        mp_tokens->swap(dest);
    }

private:
    ::boost::shared_ptr<vector<const formula_token_base*> > mp_tokens;
};

}

base_cell::base_cell(celltype_t celltype, double value) :
    m_raw_bits(0),
    m_value(value)
{
    m_data.celltype = celltype;
}

base_cell::base_cell(celltype_t celltype, size_t identifier) :
    m_raw_bits(0),
    m_identifier(identifier)
{
    m_data.celltype = celltype;
}

base_cell::~base_cell() {}

void base_cell::set_flag(int mask, bool value)
{
    if (value)
        m_data.flag |= mask;
    else
        m_data.flag &= ~mask;
}

bool base_cell::get_flag(int mask) const
{
    return (m_data.flag & mask) != 0;
}

void base_cell::reset_flag()
{
    m_data.flag &= ~FORMULA_CIRCULAR_SAFE;
}

double base_cell::get_value() const
{
    switch (get_celltype())
    {
        case celltype_formula:
            return static_cast<const formula_cell*>(this)->get_value();
        case celltype_numeric:
            return m_value;
        case celltype_string:
        case celltype_unknown:
        default:
            return 0.0;
    }
}

size_t base_cell::get_identifier() const
{
    return get_celltype() == celltype_numeric ? 0 : m_identifier;
}

void base_cell::set_identifier(size_t identifier)
{
    m_identifier = identifier;
}

celltype_t base_cell::get_celltype() const
{
    return static_cast<celltype_t>(m_data.celltype & celltype_mask);
}

formula_cell::interpret_status::interpret_status() :
    result(NULL) {}

formula_cell::interpret_status::~interpret_status()
{
    delete result;
}

// ============================================================================

formula_cell::formula_cell() :
    base_cell(celltype_formula, static_cast<size_t>(0))
{
}

formula_cell::formula_cell(size_t tokens_identifier) :
    base_cell(celltype_formula, tokens_identifier)
{
}

formula_cell::~formula_cell()
{
}

double formula_cell::get_value() const
{
    ::boost::mutex::scoped_lock lock(m_interpret_status.mtx);
    wait_for_interpreted_result(lock);

    if (!m_interpret_status.result)
        // Result not cached yet.  Reference error.
        throw formula_error(fe_ref_result_not_available);

    if (m_interpret_status.result->get_type() == formula_result::rt_error)
        // Error condition.
        throw formula_error(m_interpret_status.result->get_error());

    assert(m_interpret_status.result->get_type() == formula_result::rt_value);
    return m_interpret_status.result->get_value();
}

void formula_cell::interpret(iface::model_context& context, const abs_address_t& pos)
{
#if DEBUG_FORMULA_CELL
    const formula_name_resolver& resolver = context.get_name_resolver();
    __IXION_DEBUG_OUT__ << resolver.get_name(pos, false) << ": interpreting" << endl;
#endif
    {
        ::boost::mutex::scoped_lock lock(m_interpret_status.mtx);

        if (m_interpret_status.result)
        {
            // When the result is already cached before the cell is interpreted,
            // it can mean the cell has circular dependency.
            if (m_interpret_status.result->get_type() == formula_result::rt_error)
            {
                iface::session_handler* handler = context.get_session_handler();
                if (handler)
                {
                    handler->begin_cell_interpret(pos);
                    const char* msg = get_formula_error_name(m_interpret_status.result->get_error());
                    handler->set_formula_error(msg);
                }
            }
            return;
        }

        formula_interpreter fin(this, context);
        fin.set_origin(pos);
        m_interpret_status.result = new formula_result;
        if (fin.interpret())
        {
            // Successful interpretation.
            *m_interpret_status.result = fin.get_result();
        }
        else
        {
            // Interpretation ended with an error condition.
            m_interpret_status.result->set_error(fin.get_error());
        }
    }
    m_interpret_status.cond.notify_all();
}

bool formula_cell::is_circular_safe() const
{
    return get_flag(FORMULA_CIRCULAR_SAFE);
}

void formula_cell::check_circular(const iface::model_context& cxt, const abs_address_t& pos)
{
    // TODO: Check to make sure this is being run on the main thread only.
    const formula_tokens_t* tokens = NULL;
    if (is_shared())
        tokens = cxt.get_shared_formula_tokens(pos.sheet, m_identifier);
    else
        tokens = cxt.get_formula_tokens(pos.sheet, m_identifier);

    if (!tokens)
    {
        std::ostringstream os;
        if (is_shared())
            os << "failed to retrieve shared formula tokens from formula cell's identifier. ";
        else
            os << "failed to retrieve formula tokens from formula cell's identifier. ";
        os << "(identifier=" << m_identifier << ")";
        throw model_context_error(os.str());
    }

    formula_tokens_t::const_iterator itr = tokens->begin(), itr_end = tokens->end();
    for (; itr != itr_end; ++itr)
    {
        switch (itr->get_opcode())
        {
            case fop_single_ref:
            {
                abs_address_t addr = itr->get_single_ref().to_abs(pos);
                const formula_cell* ref = cxt.get_formula_cell(addr);

                if (!ref)
                    continue;

                if (!check_ref_for_circular_safety(*ref, addr))
                    return;
            }
            break;
            case fop_range_ref:
            {
                abs_range_t range = itr->get_range_ref().to_abs(pos);
                for (sheet_t sheet = range.first.sheet; sheet <= range.last.sheet; ++sheet)
                {
                    for (col_t col = range.first.column; col <= range.last.column; ++col)
                    {
                        for (row_t row = range.first.row; row <= range.last.row; ++row)
                        {
                            abs_address_t addr(sheet, row, col);
                            if (cxt.is_empty(addr) || cxt.get_celltype(addr) != celltype_formula)
                                continue;

                            if (!check_ref_for_circular_safety(*cxt.get_formula_cell(addr), addr))
                                return;
                        }
                    }
                }
            }
            default:
#if DEBUG_FORMULA_CELL
                __IXION_DEBUG_OUT__ << "check_circular: token type " << get_opcode_name(itr->get_opcode())
                    << " was not processed." << endl;
#else
                ;
#endif
        }

    }

    // No circular dependencies.  Good.
    set_flag(FORMULA_CIRCULAR_SAFE, true);
}

bool formula_cell::check_ref_for_circular_safety(const formula_cell& ref, const abs_address_t& pos)
{
    if (!ref.is_circular_safe())
    {
        // Circular dependency detected !!
#if DEBUG_FORMULA_CELL
        __IXION_DEBUG_OUT__ << "circular dependency detected !!" << endl;
#endif
        assert(!m_interpret_status.result);
        m_interpret_status.result = new formula_result(fe_ref_result_not_available);
        return false;
    }
    return true;
}

void formula_cell::reset()
{
    ::boost::mutex::scoped_lock lock(m_interpret_status.mtx);
    delete m_interpret_status.result;
    m_interpret_status.result = NULL;
    reset_flag();
}

void formula_cell::get_ref_tokens(const iface::model_context& cxt, const abs_address_t& pos, vector<const formula_token_base*>& tokens)
{
    const formula_tokens_t* this_tokens = NULL;
    if (is_shared())
        this_tokens = cxt.get_shared_formula_tokens(pos.sheet, m_identifier);
    else
        this_tokens = cxt.get_formula_tokens(pos.sheet, m_identifier);

    if (!this_tokens)
        return;

    ref_token_picker func;
    for_each(this_tokens->begin(), this_tokens->end(), func).swap_tokens(tokens);
}

const formula_result* formula_cell::get_result_cache() const
{
    ::boost::mutex::scoped_lock lock(m_interpret_status.mtx);
    return m_interpret_status.result;
}

bool formula_cell::is_shared() const
{
    return get_flag(FORMULA_SHARED_TOKENS);
}

void formula_cell::set_shared(bool b)
{
    set_flag(FORMULA_SHARED_TOKENS, b);
}

void formula_cell::wait_for_interpreted_result(::boost::mutex::scoped_lock& lock) const
{
#if DEBUG_FORMULA_CELL
    __IXION_DEBUG_OUT__ << "wait for interpreted result" << endl;
#endif
    while (!m_interpret_status.result)
    {
#if DEBUG_FORMULA_CELL
        __IXION_DEBUG_OUT__ << "waiting" << endl;
#endif
        m_interpret_status.cond.wait(lock);
    }
}

}
