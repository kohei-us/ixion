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
    static formula_function_t get_function_opcode(std::string_view s);
    static std::string_view get_function_name(formula_function_t oc);

    void interpret(formula_function_t oc, formula_value_stack& args);

private:
    void fnc_max(formula_value_stack& args) const;
    void fnc_min(formula_value_stack& args) const;
    void fnc_sum(formula_value_stack& args) const;
    void fnc_counta(formula_value_stack& args) const;
    void fnc_abs(formula_value_stack& args) const;
    void fnc_average(formula_value_stack& args) const;
    void fnc_mmult(formula_value_stack& args) const;
    void fnc_pi(formula_value_stack& args) const;
    void fnc_int(formula_value_stack& args) const;

    void fnc_if(formula_value_stack& args) const;
    void fnc_isblank(formula_value_stack& args) const;
    void fnc_isformula(formula_value_stack& args) const;
    void fnc_isnontext(formula_value_stack& args) const;
    void fnc_isnumber(formula_value_stack& args) const;
    void fnc_isref(formula_value_stack& args) const;
    void fnc_istext(formula_value_stack& args) const;

    void fnc_len(formula_value_stack& args) const;
    void fnc_concatenate(formula_value_stack& args) const;
    void fnc_left(formula_value_stack& args) const;

    void fnc_now(formula_value_stack& args) const;

    void fnc_wait(formula_value_stack& args) const;

    void fnc_subtotal(formula_value_stack& args) const;

private:
    iface::formula_model_access& m_context;
};

}

#endif
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
