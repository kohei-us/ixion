/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __IXION_FORMULA_INTERPRETER_HPP__
#define __IXION_FORMULA_INTERPRETER_HPP__

#include "ixion/global.hpp"
#include "ixion/formula_tokens.hpp"
#include "ixion/formula_result.hpp"

#include "formula_value_stack.hpp"

#include <sstream>
#include <unordered_set>

namespace ixion {

class formula_cell;

namespace iface {

class formula_model_access;
class session_handler;

}

/**
 * The formula interpreter parses a series of formula tokens representing a
 * formula expression, and calculates the result of that expression.
 *
 * <p>Intermediate result of each handler is pushed onto the stack and
 * popped from it for the calling method to retrieve.  By the end of the
 * interpretation there should only be one result left on the stack which is
 * the final result of the interpretation of the expression.  The number of
 * intermediate results (or stack values) on the stack is normally one at
 * the end of each handler, except for the function handler where the number
 * of stack values may be more than one when the function may take more than
 * one argument.</p>
 */
class formula_interpreter
{
    typedef std::unordered_set< ::std::string> name_set;

public:
    typedef ::std::vector<const formula_token*> local_tokens_type;

    formula_interpreter() = delete;
    formula_interpreter(const formula_interpreter&) = delete;
    formula_interpreter& operator= (formula_interpreter) = delete;

    formula_interpreter(const formula_cell* cell, iface::formula_model_access& cxt);
    ~formula_interpreter();

    void set_origin(const abs_address_t& pos);
    bool interpret();
    formula_result transfer_result();
    formula_error_t get_error() const;

private:
    /**
     * Expand all named expressions into a flat set of tokens.  This is also
     * where we detect circular referencing of named expressions.
     */
    void init_tokens();

    void pop_result();

    void expand_named_expression(const formula_tokens_t* expr, name_set& used_names);

    bool has_token() const;
    void next();
    const formula_token& token() const;
    const formula_token& next_token();

    // The following methods are handlers.  In each handler, the initial
    // position is always set to the first unprocessed token.  Each handler is
    // responsible for setting the token position to the next unprocessed
    // position when it finishes.

    void expression();
    void term();
    void factor();
    void paren();
    void single_ref();
    void range_ref();
    void table_ref();
    void constant();
    void literal();
    void function();

private:
    const formula_cell* m_parent_cell;
    iface::formula_model_access& m_context;
    std::unique_ptr<iface::session_handler> mp_handler;
    abs_address_t m_pos;

    formula_value_stack m_stack;
    local_tokens_type m_tokens;
    local_tokens_type::const_iterator m_cur_token_itr;
    local_tokens_type::const_iterator m_end_token_pos;

    formula_result m_result;
    formula_error_t m_error;
};

}

#endif
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
