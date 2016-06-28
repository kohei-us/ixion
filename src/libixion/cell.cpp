/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/cell.hpp"
#include "ixion/address.hpp"
#include "ixion/cell_listener_tracker.hpp"
#include "ixion/exceptions.hpp"
#include "ixion/formula_result.hpp"
#include "ixion/formula_tokens.hpp"
#include "ixion/interface/formula_model_access.hpp"
#include "ixion/interface/session_handler.hpp"
#include "ixion/global.hpp"

#include "formula_interpreter.hpp"

#include <mutex>
#include <condition_variable>

#include <cassert>
#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>

#define DEBUG_FORMULA_CELL 0
#if DEBUG_FORMULA_CELL
#include "ixion/formula_name_resolver.hpp"
#endif


#define FORMULA_CIRCULAR_SAFE 0x01
#define FORMULA_SHARED_TOKENS 0x02

using namespace std;

namespace ixion {

struct interpret_status
{
    interpret_status(const interpret_status&) = delete;
    interpret_status& operator=(const interpret_status&) = delete;

    std::mutex mtx;
    std::condition_variable cond;

    std::unique_ptr<formula_result> result;

    interpret_status() : result(nullptr) {}
};

struct formula_cell::impl
{
    mutable interpret_status m_interpret_status;
    size_t m_identifier;
    bool m_shared_token:1;
    bool m_circular_safe:1;

    impl() :
        m_identifier(0),
        m_shared_token(false),
        m_circular_safe(false) {}

    impl(size_t tokens_identifier) :
        m_identifier(tokens_identifier),
        m_shared_token(false),
        m_circular_safe(false) {}

    /**
     * Block until the result becomes available.
     *
     * @param lock mutex lock associated with the result cache data.
     */
    void wait_for_interpreted_result(std::unique_lock<std::mutex>& lock) const
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

    void reset_flag()
    {
        m_circular_safe = false;
    }

    /**
     * Check if this cell contains a circular reference.
     *
     * @return true if this cell contains no circular reference, hence
     *         considered "safe", false otherwise.
     */
    bool is_circular_safe() const
    {
        return m_circular_safe;
    }

    bool check_ref_for_circular_safety(const formula_cell& ref, const abs_address_t& pos)
    {
        if (!ref.mp_impl->is_circular_safe())
        {
            // Circular dependency detected !!
#if DEBUG_FORMULA_CELL
            __IXION_DEBUG_OUT__ << "circular dependency detected !!" << endl;
#endif
            assert(!m_interpret_status.result);
            m_interpret_status.result =
                ixion::make_unique<formula_result>(fe_ref_result_not_available);

            return false;
        }
        return true;
    }

    double fetch_value_from_result() const
    {
        if (!m_interpret_status.result)
            // Result not cached yet.  Reference error.
            throw formula_error(fe_ref_result_not_available);

        if (m_interpret_status.result->get_type() == formula_result::rt_error)
            // Error condition.
            throw formula_error(m_interpret_status.result->get_error());

        assert(m_interpret_status.result->get_type() == formula_result::rt_value);
        return m_interpret_status.result->get_value();
    }
};

formula_cell::formula_cell() : mp_impl(ixion::make_unique<impl>()) {}

formula_cell::formula_cell(size_t tokens_identifier) :
    mp_impl(ixion::make_unique<impl>(tokens_identifier)) {}

formula_cell::~formula_cell()
{
}

size_t formula_cell::get_identifier() const
{
    return mp_impl->m_identifier;
}

void formula_cell::set_identifier(size_t identifier)
{
    mp_impl->m_identifier = identifier;
}

double formula_cell::get_value() const
{
    std::unique_lock<std::mutex> lock(mp_impl->m_interpret_status.mtx);
    mp_impl->wait_for_interpreted_result(lock);
    return mp_impl->fetch_value_from_result();
}

double formula_cell::get_value_nowait() const
{
    std::lock_guard<std::mutex> lock(mp_impl->m_interpret_status.mtx);
    return mp_impl->fetch_value_from_result();
}

void formula_cell::interpret(iface::formula_model_access& context, const abs_address_t& pos)
{
#if DEBUG_FORMULA_CELL
    const formula_name_resolver& resolver = context.get_name_resolver();
    __IXION_DEBUG_OUT__ << resolver.get_name(pos, false) << ": interpreting" << endl;
#endif
    {
        std::unique_lock<std::mutex> lock(mp_impl->m_interpret_status.mtx);

        if (mp_impl->m_interpret_status.result)
        {
            // When the result is already cached before the cell is interpreted,
            // it can mean the cell has circular dependency.
            if (mp_impl->m_interpret_status.result->get_type() == formula_result::rt_error)
            {
                iface::session_handler* handler = context.get_session_handler();
                if (handler)
                {
                    handler->begin_cell_interpret(pos);
                    const char* msg = get_formula_error_name(mp_impl->m_interpret_status.result->get_error());
                    handler->set_formula_error(msg);
                }
            }
            return;
        }

        formula_interpreter fin(this, context);
        fin.set_origin(pos);
        mp_impl->m_interpret_status.result = ixion::make_unique<formula_result>();
        if (fin.interpret())
        {
            // Successful interpretation.
            *mp_impl->m_interpret_status.result = fin.get_result();
        }
        else
        {
            // Interpretation ended with an error condition.
            mp_impl->m_interpret_status.result->set_error(fin.get_error());
        }
    }

    mp_impl->m_interpret_status.cond.notify_all();
}

void formula_cell::check_circular(const iface::formula_model_access& cxt, const abs_address_t& pos)
{
    // TODO: Check to make sure this is being run on the main thread only.
    const formula_tokens_t* tokens = NULL;
    if (is_shared())
        tokens = cxt.get_shared_formula_tokens(pos.sheet, mp_impl->m_identifier);
    else
        tokens = cxt.get_formula_tokens(pos.sheet, mp_impl->m_identifier);

    if (!tokens)
    {
        std::ostringstream os;
        if (is_shared())
            os << "failed to retrieve shared formula tokens from formula cell's identifier. ";
        else
            os << "failed to retrieve formula tokens from formula cell's identifier. ";
        os << "(identifier=" << mp_impl->m_identifier << ")";
        throw model_context_error(os.str(), model_context_error::circular_dependency);
    }

    formula_tokens_t::const_iterator itr = tokens->begin(), itr_end = tokens->end();
    for (; itr != itr_end; ++itr)
    {
        switch ((*itr)->get_opcode())
        {
            case fop_single_ref:
            {
                abs_address_t addr = (*itr)->get_single_ref().to_abs(pos);
                const formula_cell* ref = cxt.get_formula_cell(addr);

                if (!ref)
                    continue;

                if (!mp_impl->check_ref_for_circular_safety(*ref, addr))
                    return;
            }
            break;
            case fop_range_ref:
            {
                abs_range_t range = (*itr)->get_range_ref().to_abs(pos);
                for (sheet_t sheet = range.first.sheet; sheet <= range.last.sheet; ++sheet)
                {
                    for (col_t col = range.first.column; col <= range.last.column; ++col)
                    {
                        for (row_t row = range.first.row; row <= range.last.row; ++row)
                        {
                            abs_address_t addr(sheet, row, col);
                            if (cxt.get_celltype(addr) != celltype_t::formula)
                                continue;

                            if (!mp_impl->check_ref_for_circular_safety(*cxt.get_formula_cell(addr), addr))
                                return;
                        }
                    }
                }
            }
            default:
#if DEBUG_FORMULA_CELL
                __IXION_DEBUG_OUT__ << "check_circular: token type " << get_opcode_name((*itr)->get_opcode())
                    << " was not processed." << endl;
#else
                ;
#endif
        }

    }

    // No circular dependencies.  Good.
    mp_impl->m_circular_safe = true;
}

void formula_cell::reset()
{
    std::unique_lock<std::mutex> lock(mp_impl->m_interpret_status.mtx);
    mp_impl->m_interpret_status.result.reset();
    mp_impl->reset_flag();
}

void formula_cell::get_ref_tokens(const iface::formula_model_access& cxt, const abs_address_t& pos, vector<const formula_token_base*>& tokens)
{
    const formula_tokens_t* this_tokens = NULL;
    if (is_shared())
        this_tokens = cxt.get_shared_formula_tokens(pos.sheet, mp_impl->m_identifier);
    else
        this_tokens = cxt.get_formula_tokens(pos.sheet, mp_impl->m_identifier);

    if (!this_tokens)
        return;

    std::for_each(this_tokens->begin(), this_tokens->end(),
        [&](const formula_tokens_t::value_type& t)
        {
            switch (t->get_opcode())
            {
                case fop_single_ref:
                case fop_range_ref:
                    tokens.push_back(&(*t));
                break;
                default:
                    ; // ignore the rest.
            }
        }
    );
}

const formula_result* formula_cell::get_result_cache() const
{
    std::unique_lock<std::mutex> lock(mp_impl->m_interpret_status.mtx);
    return mp_impl->m_interpret_status.result.get();
}

bool formula_cell::is_shared() const
{
    return mp_impl->m_shared_token;
}

void formula_cell::set_shared(bool b)
{
    mp_impl->m_shared_token = b;
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
