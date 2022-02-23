/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <ixion/cell.hpp>
#include <ixion/address.hpp>
#include <ixion/exceptions.hpp>
#include <ixion/formula_result.hpp>
#include <ixion/formula_tokens.hpp>
#include <ixion/interface/session_handler.hpp>
#include <ixion/global.hpp>
#include <ixion/matrix.hpp>
#include <ixion/formula_name_resolver.hpp>
#include <ixion/formula.hpp>
#include <ixion/model_context.hpp>

#include "formula_interpreter.hpp"
#include "debug.hpp"

#include <cassert>
#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <functional>

#include "calc_status.hpp"

#define FORMULA_CIRCULAR_SAFE 0x01
#define FORMULA_SHARED_TOKENS 0x02

using namespace std;

namespace ixion {

namespace {

#if IXION_LOGGING

std::string gen_trace_output(const formula_cell& fc, const model_context& cxt, const abs_address_t& pos)
{
    auto resolver = formula_name_resolver::get(formula_name_resolver_t::excel_a1, &cxt);
    std::ostringstream os;
    os << "pos=" << pos.get_name() << "; formula='" << print_formula_tokens(cxt, pos, *resolver, fc.get_tokens()->get()) << "'";
    return os.str();
}

#endif

}

struct formula_cell::impl
{
    mutable calc_status_ptr_t m_calc_status;
    formula_tokens_store_ptr_t m_tokens;
    rc_address_t m_group_pos;

    impl() : impl(-1, -1, new calc_status, formula_tokens_store_ptr_t()) {}

    impl(const formula_tokens_store_ptr_t& tokens) : impl(-1, -1, new calc_status, tokens) {}

    impl(row_t row, col_t col, const calc_status_ptr_t& cs,
        const formula_tokens_store_ptr_t& tokens) :
        m_calc_status(cs),
        m_tokens(tokens),
        m_group_pos(row, col, false, false) {}

    /**
     * Block until the result becomes available.
     *
     * @param lock mutex lock associated with the result cache data.
     */
    void wait_for_interpreted_result(std::unique_lock<std::mutex>& lock) const
    {
        IXION_TRACE("Wait for the interpreted result");

        while (!m_calc_status->result)
        {
            IXION_TRACE("Waiting...");
            m_calc_status->cond.wait(lock);
        }
    }

    void reset_flag()
    {
        m_calc_status->circular_safe = false;
    }

    /**
     * Check if this cell contains a circular reference.
     *
     * @return true if this cell contains no circular reference, hence
     *         considered "safe", false otherwise.
     */
    bool is_circular_safe() const
    {
        return m_calc_status->circular_safe;
    }

    bool check_ref_for_circular_safety(const formula_cell& ref, const abs_address_t& pos)
    {
        if (!ref.mp_impl->is_circular_safe())
        {
            // Circular dependency detected !!
            IXION_DEBUG("Circular dependency detected !!");
            assert(!m_calc_status->result);
            m_calc_status->result =
                std::make_unique<formula_result>(formula_error_t::ref_result_not_available);

            return false;
        }
        return true;
    }

    void check_calc_status_or_throw() const
    {
        if (!m_calc_status->result)
        {
            // Result not cached yet.  Reference error.
            IXION_DEBUG("Result not cached yet. This is a reference error.");
            throw formula_error(formula_error_t::ref_result_not_available);
        }

        if (m_calc_status->result->get_type() == formula_result::result_type::error)
        {
            // Error condition.
            IXION_DEBUG("Error in result.");
            throw formula_error(m_calc_status->result->get_error());
        }
    }

    double fetch_value_from_result() const
    {
        check_calc_status_or_throw();

        switch (m_calc_status->result->get_type())
        {
            case formula_result::result_type::boolean:
                return m_calc_status->result->get_boolean() ? 1.0 : 0.0;
            case formula_result::result_type::value:
                return m_calc_status->result->get_value();
            case formula_result::result_type::matrix:
            {
                IXION_TRACE("Fetching a matrix result.");
                const matrix& m = m_calc_status->result->get_matrix();
                row_t row_size = m.row_size();
                col_t col_size = m.col_size();

                if (m_group_pos.row >= row_size || m_group_pos.column >= col_size)
                    throw formula_error(formula_error_t::invalid_value_type);

                matrix::element elem = m.get(m_group_pos.row, m_group_pos.column);

                switch (elem.type)
                {
                    case matrix::element_type::numeric:
                        return std::get<double>(elem.value);
                    case matrix::element_type::empty:
                        return 0.0;
                    case matrix::element_type::boolean:
                        return std::get<bool>(elem.value) ? 1.0 : 0.0;
                    case matrix::element_type::string:
                    case matrix::element_type::error:
                    default:
                        throw formula_error(formula_error_t::invalid_value_type);
                }
            }
            default:
            {
                std::ostringstream os;
                os << "numeric result was requested, but the actual result is of "
                    << m_calc_status->result->get_type() << " type.";
                throw formula_error(
                    formula_error_t::invalid_value_type,
                    os.str()
                );
            }
        }
    }

    std::string_view fetch_string_from_result() const
    {
        check_calc_status_or_throw();

        switch (m_calc_status->result->get_type())
        {
            case formula_result::result_type::string:
                return m_calc_status->result->get_string();
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
                    case matrix::element_type::string:
                        return std::get<std::string_view>(elem.value);
                    case matrix::element_type::numeric:
                    case matrix::element_type::empty:
                    case matrix::element_type::boolean:
                    case matrix::element_type::error:
                    default:
                        throw formula_error(formula_error_t::invalid_value_type);
                }
            }
            default:
            {
                std::ostringstream os;
                os << "string result was requested, but the actual result is of "
                    << m_calc_status->result->get_type() << " type.";
                throw formula_error(
                    formula_error_t::invalid_value_type,
                    os.str()
                );
            }
        }

        return std::string_view{};
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

    formula_result get_single_formula_result(const formula_result& src) const
    {
        if (!is_grouped())
            return src;  // returns a copy.

        if (src.get_type() != formula_result::result_type::matrix)
            // A grouped cell should have a matrix result whose size equals the
            // size of the group. But in case of anything else, just return the
            // stored value.
            return src;

        const matrix& m = src.get_matrix();
        row_t row_size = m.row_size();
        col_t col_size = m.col_size();

        if (m_group_pos.row >= row_size || m_group_pos.column >= col_size)
            return formula_result(formula_error_t::invalid_value_type);

        matrix::element elem = m.get(m_group_pos.row, m_group_pos.column);

        switch (elem.type)
        {
            case matrix::element_type::numeric:
                return formula_result(std::get<double>(elem.value));
            case matrix::element_type::string:
            {
                std::string s{std::get<std::string_view>(elem.value)};
                return formula_result(std::move(s));
            }
            case matrix::element_type::error:
                return formula_result(std::get<formula_error_t>(elem.value));
            case matrix::element_type::empty:
                return formula_result();
            case matrix::element_type::boolean:
                return formula_result(std::get<bool>(elem.value) ? 1.0 : 0.0);
            default:
                throw std::logic_error("unhandled element type of a matrix result value.");
        }
    }

    void set_single_formula_result(formula_result result)
    {
        if (is_grouped())
        {
            std::unique_lock<std::mutex> lock(m_calc_status->mtx);

            if (!m_calc_status->result)
            {
                m_calc_status->result =
                    std::make_unique<formula_result>(
                        matrix(m_calc_status->group_size.row, m_calc_status->group_size.column));
            }

            matrix& m = m_calc_status->result->get_matrix();
            assert(m_group_pos.row < row_t(m.row_size()));
            assert(m_group_pos.column < col_t(m.col_size()));

            switch (result.get_type())
            {
                case formula_result::result_type::boolean:
                    m.set(m_group_pos.row, m_group_pos.column, result.get_boolean());
                    break;
                case formula_result::result_type::value:
                    m.set(m_group_pos.row, m_group_pos.column, result.get_value());
                    break;
                case formula_result::result_type::string:
                    m.set(m_group_pos.row, m_group_pos.column, result.get_string());
                    break;
                case formula_result::result_type::error:
                    m.set(m_group_pos.row, m_group_pos.column, result.get_error());
                    break;
                case formula_result::result_type::matrix:
                    throw std::logic_error("setting a cached result of matrix value directly is not yet supported.");
            }

            return;
        }

        std::unique_lock<std::mutex> lock(m_calc_status->mtx);
        m_calc_status->result = std::make_unique<formula_result>(std::move(result));
    }
};

formula_cell::formula_cell() : mp_impl(std::make_unique<impl>()) {}

formula_cell::formula_cell(const formula_tokens_store_ptr_t& tokens) :
    mp_impl(std::make_unique<impl>(tokens)) {}

formula_cell::formula_cell(
    row_t group_row, col_t group_col,
    const calc_status_ptr_t& cs,
    const formula_tokens_store_ptr_t& tokens) :
    mp_impl(std::make_unique<impl>(group_row, group_col, cs, tokens)) {}

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

double formula_cell::get_value(formula_result_wait_policy_t policy) const
{
    std::unique_lock<std::mutex> lock(mp_impl->m_calc_status->mtx);
    if (policy == formula_result_wait_policy_t::block_until_done)
        mp_impl->wait_for_interpreted_result(lock);
    return mp_impl->fetch_value_from_result();
}

std::string_view formula_cell::get_string(formula_result_wait_policy_t policy) const
{
    std::unique_lock<std::mutex> lock(mp_impl->m_calc_status->mtx);
    if (policy == formula_result_wait_policy_t::block_until_done)
        mp_impl->wait_for_interpreted_result(lock);
    return mp_impl->fetch_string_from_result();
}

void formula_cell::interpret(model_context& context, const abs_address_t& pos)
{
    IXION_TRACE(gen_trace_output(*this, context, pos));

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
                    std::string_view msg = get_formula_error_name(status.result->get_error());
                    handler->set_formula_error(msg);
                    handler->end_cell_interpret();
                }
            }
            return;
        }

        formula_interpreter fin(this, context);
        fin.set_origin(pos);
        status.result = std::make_unique<formula_result>();
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

void formula_cell::check_circular(const model_context& cxt, const abs_address_t& pos)
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
                    rc_size_t sheet_size = cxt.get_sheet_size();
                    col_t col_first = range.first.column, col_last = range.last.column;
                    if (range.all_columns())
                    {
                        col_first = 0;
                        col_last = sheet_size.column - 1;
                    }

                    for (col_t col = col_first; col <= col_last; ++col)
                    {
                        row_t row_first = range.first.row, row_last = range.last.row;
                        if (range.all_rows())
                        {
                            assert(row_last == row_unset);
                            row_first = 0;
                            row_last = sheet_size.row - 1;
                        }

                        for (row_t row = row_first; row <= row_last; ++row)
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
                ;
        }

    }

    // No circular dependencies.  Good.
    mp_impl->m_calc_status->circular_safe = true;
}

void formula_cell::reset()
{
    std::lock_guard<std::mutex> lock(mp_impl->m_calc_status->mtx);
    mp_impl->m_calc_status->result.reset();
    mp_impl->reset_flag();
}

std::vector<const formula_token*> formula_cell::get_ref_tokens(
    const model_context& cxt, const abs_address_t& pos) const
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
                const named_expression_t* named_exp =
                    cxt.get_named_expression(pos.sheet, t->get_name());

                if (!named_exp)
                    // silently ignore non-existing names.
                    break;

                // recursive call.
                std::for_each(named_exp->tokens.begin(), named_exp->tokens.end(), get_refs);
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

const formula_result& formula_cell::get_raw_result_cache(formula_result_wait_policy_t policy) const
{
    std::unique_lock<std::mutex> lock(mp_impl->m_calc_status->mtx);

    if (policy == formula_result_wait_policy_t::block_until_done)
        mp_impl->wait_for_interpreted_result(lock);

    if (!mp_impl->m_calc_status->result)
    {
        IXION_DEBUG("Result not yet available.");
        throw formula_error(formula_error_t::ref_result_not_available);
    }

    return *mp_impl->m_calc_status->result;
}

formula_result formula_cell::get_result_cache(formula_result_wait_policy_t policy) const
{
    const formula_result& src = get_raw_result_cache(policy);
    return mp_impl->get_single_formula_result(src);
}

void formula_cell::set_result_cache(formula_result result)
{
    mp_impl->set_single_formula_result(result);
}

formula_group_t formula_cell::get_group_properties() const
{
    uintptr_t identity = reinterpret_cast<uintptr_t>(mp_impl->m_calc_status.get());
    return formula_group_t(mp_impl->m_calc_status->group_size, identity, mp_impl->is_grouped());
}

abs_address_t formula_cell::get_parent_position(const abs_address_t& pos) const
{
    if (!mp_impl->is_grouped())
        return pos;

    abs_address_t parent_pos = pos;
    parent_pos.column -= mp_impl->m_group_pos.column;
    parent_pos.row -= mp_impl->m_group_pos.row;
    return parent_pos;
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
