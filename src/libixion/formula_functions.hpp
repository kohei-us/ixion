/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __IXION_FORMULA_FUNCTIONS_HPP__
#define __IXION_FORMULA_FUNCTIONS_HPP__

#include "ixion/global.hpp"
#include "ixion/exceptions.hpp"
#include "ixion/formula_function_opcode.hpp"

#include "formula_value_stack.hpp"

#include <string>
#include <vector>

namespace ixion {

class formula_token;

namespace iface {

class formula_model_access;

}

/**
 * Collection of built-in cell function implementations.  Note that those
 * functions that return a string result <i>may</i> modify the state of the
 * model context when the result string is not yet in the shared string
 * pool.
 */
class formula_functions
{
public:
    class invalid_arg : public general_error
    {
    public:
        invalid_arg(const ::std::string& msg);
    };

    formula_functions(iface::formula_model_access& cxt);
    ~formula_functions();

    static formula_function_t get_function_opcode(const formula_token& token);
    static formula_function_t get_function_opcode(const char* p, size_t n);
    static const char* get_function_name(formula_function_t oc);

    void interpret(formula_function_t oc, value_stack_t& args);

private:
    void fnc_max(value_stack_t& args) const;
    void fnc_min(value_stack_t& args) const;
    void fnc_sum(value_stack_t& args) const;
    void fnc_counta(value_stack_t& args) const;
    void fnc_average(value_stack_t& args) const;
    void fnc_mmult(value_stack_t& args) const;

    void fnc_if(value_stack_t& args) const;

    void fnc_len(value_stack_t& args) const;
    void fnc_concatenate(value_stack_t& args);

    void fnc_now(value_stack_t& args) const;

    void fnc_wait(value_stack_t& args) const;

    void fnc_subtotal(value_stack_t& args) const;

private:
    iface::formula_model_access& m_context;
};

}

#endif
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
