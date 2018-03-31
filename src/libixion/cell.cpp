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
#include "ixion/matrix.hpp"

#include "formula_interpreter.hpp"

#include <cassert>
#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <functional>

#define DEBUG_FORMULA_CELL 0
#if DEBUG_FORMULA_CELL
#include "ixion/formula_name_resolver.hpp"
#endif

#include "calc_status.hpp"

#define FORMULA_CIRCULAR_SAFE 0x01
#define FORMULA_SHARED_TOKENS 0x02

using namespace std;

namespace ixion {

struct formula_cell::impl
{
    mutable calc_status_ptr_t m_calc_status;
    formula_tokens_store_ptr_t m_tokens;
    rc_address_t m_group_pos;

    bool m_circular_safe:1;

    impl() : impl(-1, -1, new calc_status, formula_tokens_store_ptr_t()) {}

    impl(const formula_tokens_store_ptr_t& tokens) : impl(-1, -1, new calc_status, tokens) {}

    impl(row_t row, col_t col, const calc_status_ptr_t& cs,
        const formula_tokens_store_ptr_t& tokens) :
        m_calc_status(cs),
        m_tokens(tokens),
        m_group_pos(row, col, false, false),
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
        while (!m_calc_status->result)
        {
#if DEBUG_FORMULA_CELL
            __IXION_DEBUG_OUT__ << "waiting" << endl;
#endif
            m_calc_status->cond.wait(lock);
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
            assert(!m_calc_status->result);
            m_calc_status->result =
                ixion::make_unique<formula_result>(formula_error_t::ref_result_not_available);

            return false;
        }
        return true;
    }

    double fetch_value_from_result() const
    {
        if (!m_calc_status->result)
            // Result not cached yet.  Reference error.
            throw formula_error(formula_error_t::ref_result_not_available);

        if (m_calc_status->result->get_type() == formula_result::result_type::error)
            // Error condition.
            throw formula_error(m_calc_status->result->get_error());

        switch (m_calc_status->result->get_type())
        {
            case formula_result::result_type::value:
                return m_calc_status->result->get_value();
            case formula_result::result_type::matrix:
            {
                const matrix& m = m_calc_status->result->get_matrix();
                row_t row_size = m.row_size();
                col_t col_size = m.col_size();

                if (m_group_pos.row >= row_size || m_group_pos.column >= col_size)
                    throw formula_error(formula_error_t::invalid_value_type);

                matrix::element elem = m.get(m_group_pos.row, m_group_pos.column);

                switch (elem.type)
                {
                    case matrix::element_type::numeric:
                        return elem.numeric;
                    case matrix::element_type::empty:
                        return 0.0;
                    case matrix::element_type::boolean:
                        return elem.boolean ? 1.0 : 0.0;
                    case matrix::element_type::string:
                    default:
                        throw formula_error(formula_error_t::invalid_value_type);
                }
            }
            default:
                throw formula_error(formula_error_t::invalid_value_type);
        }
    }

    bool is_grouped() const
    {
        return m_group_pos.column >= 0 && m_group_pos.row >= 0;
    }

    bool is_group_parent() const
    {
        return m_group_pos.column == 0 && m_group_pos.row == 0;
    }

    bool calc_allowed() const
    {
        if (!is_grouped())
            return true;

        return is_group_parent();
    }
};

formula_cell::formula_cell() : mp_impl(ixion::make_unique<impl>()) {}

formula_cell::formula_cell(const formula_tokens_store_ptr_t& tokens) :
    mp_impl(ixion::make_unique<impl>(tokens)) {}

formula_cell::formula_cell(
    row_t group_row, col_t group_col,
    const calc_status_ptr_t& cs,
    const formula_tokens_store_ptr_t& tokens) :
    mp_impl(ixion::make_unique<impl>(group_row, group_col, cs, tokens)) {}

formula_cell::~formula_cell()
{
}

const formula_tokens_store_ptr_t& formula_cell::get_tokens() const
{
    return mp_impl->m_tokens;
}

void formula_cell::set_tokens(const formula_tokens_store_ptr_t& tokens)
{
    mp_impl->m_tokens = tokens;
}

double formula_cell::get_value() const
{
    std::unique_lock<std::mutex> lock(mp_impl->m_calc_status->mtx);
    mp_impl->wait_for_interpreted_result(lock);
    return mp_impl->fetch_value_from_result();
}

double formula_cell::get_value_nowait() const
{
    std::lock_guard<std::mutex> lock(mp_impl->m_calc_status->mtx);
    return mp_impl->fetch_value_from_result();
}

void formula_cell::interpret(iface::formula_model_access& context, const abs_address_t& pos)
{
#if DEBUG_FORMULA_CELL
    const formula_name_resolver& resolver = context.get_name_resolver();
    __IXION_DEBUG_OUT__ << resolver.get_name(pos, false) << ": interpreting" << endl;
#endif
    if (!mp_impl->calc_allowed())
        throw std::logic_error("Calculation on this formula cell is not allowed.");

    calc_status& status = *mp_impl->m_calc_status;

    {
        std::lock_guard<std::mutex> lock(status.mtx);

        if (mp_impl->m_calc_status->result)
        {
            // When the result is already cached before the cell is interpreted,
            // it can mean the cell has circular dependency.
            if (status.result->get_type() == formula_result::result_type::error)
            {
                auto handler = context.create_session_handler();
                if (handler)
                {
                    handler->begin_cell_interpret(pos);
                    const char* msg = get_formula_error_name(status.result->get_error());
                    handler->set_formula_error(msg);
                    handler->end_cell_interpret();
                }
            }
            return;
        }

        formula_interpreter fin(this, context);
        fin.set_origin(pos);
        status.result = ixion::make_unique<formula_result>();
        if (fin.interpret())
        {
            // Successful interpretation.
            *status.result = fin.transfer_result();
        }
        else
        {
            // Interpretation ended with an error condition.
            status.result->set_error(fin.get_error());
        }
    }

    status.cond.notify_all();
}

void formula_cell::check_circular(const iface::formula_model_access& cxt, const abs_address_t& pos)
{
    // TODO: Check to make sure this is being run on the main thread only.
    const formula_tokens_t& tokens = mp_impl->m_tokens->get();
    for (const std::unique_ptr<formula_token>& t : tokens)
    {
        switch (t->get_opcode())
        {
            case fop_single_ref:
            {
                abs_address_t addr = t->get_single_ref().to_abs(pos);
                const formula_cell* ref = cxt.get_formula_cell(addr);

                if (!ref)
                    continue;

                if (!mp_impl->check_ref_for_circular_safety(*ref, addr))
                    return;

                break;
            }
            case fop_range_ref:
            {
                abs_range_t range = t->get_range_ref().to_abs(pos);
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

                break;
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
    std::lock_guard<std::mutex> lock(mp_impl->m_calc_status->mtx);
    mp_impl->m_calc_status->result.reset();
    mp_impl->reset_flag();
}

std::vector<const formula_token*> formula_cell::get_ref_tokens(
    const iface::formula_model_access& cxt, const abs_address_t& pos) const
{
    std::vector<const formula_token*> ret;

    std::function<void(const formula_tokens_t::value_type&)> get_refs = [&](const formula_tokens_t::value_type& t)
    {
        switch (t->get_opcode())
        {
            case fop_single_ref:
            case fop_range_ref:
                ret.push_back(t.get());
                break;
            case fop_named_expression:
            {
                const formula_tokens_t* named_exp =
                    cxt.get_named_expression(pos.sheet, t->get_name());

                if (!named_exp)
                    // silently ignore non-existing names.
                    break;

                // recursive call.
                std::for_each(named_exp->begin(), named_exp->end(), get_refs);
                break;
            }
            default:
                ; // ignore the rest.
        }
    };

    const formula_tokens_t& this_tokens = mp_impl->m_tokens->get();

    std::for_each(this_tokens.begin(), this_tokens.end(), get_refs);

    return ret;
}

const formula_result& formula_cell::get_result_cache() const
{
    std::unique_lock<std::mutex> lock(mp_impl->m_calc_status->mtx);
    mp_impl->wait_for_interpreted_result(lock);
    if (!mp_impl->m_calc_status->result)
        throw formula_error(formula_error_t::ref_result_not_available);

    return *mp_impl->m_calc_status->result;
}

const formula_result* formula_cell::get_result_cache_nowait() const
{
    std::unique_lock<std::mutex> lock(mp_impl->m_calc_status->mtx);
    return mp_impl->m_calc_status->result.get();
}

formula_result formula_cell::get_single_result_cache() const
{
    const formula_result& src = get_result_cache();

    if (!mp_impl->is_grouped())
        return src;  // returns a copy.

    if (src.get_type() != formula_result::result_type::matrix)
        // A grouped cell should have a matrix result whose size equals the
        // size of the group. But in case of anything else, just return the
        // stored value.
        return src;

    const matrix& m = src.get_matrix();
    row_t row_size = m.row_size();
    col_t col_size = m.col_size();

    if (mp_impl->m_group_pos.row >= row_size || mp_impl->m_group_pos.column >= col_size)
        return formula_result(formula_error_t::invalid_value_type);

    matrix::element elem = m.get(mp_impl->m_group_pos.row, mp_impl->m_group_pos.column);

    switch (elem.type)
    {
        case matrix::element_type::numeric:
            return formula_result(elem.numeric);
        case matrix::element_type::string:
            return formula_result(elem.string_id);
        case matrix::element_type::empty:
            return formula_result();
        case matrix::element_type::boolean:
            return formula_result(elem.boolean ? 1.0 : 0.0);
        default:
            throw std::logic_error("unhandled element type of a matrix result value.");
    }
}

formula_group_t formula_cell::get_group_properties() const
{
    uintptr_t identity = reinterpret_cast<uintptr_t>(mp_impl->m_calc_status.get());
    return formula_group_t(mp_impl->m_calc_status->group_size, identity);
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
