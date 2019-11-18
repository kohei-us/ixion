/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_FORMULA_FUNCTION_OPCODE_HPP
#define INCLUDED_IXION_FORMULA_FUNCTION_OPCODE_HPP

#include "ixion/env.hpp"

namespace ixion {

enum class formula_function_t
{
    func_unknown = 0,

    // statistical functions
    func_max,
    func_min,
    func_average,
    func_sum,
    func_counta,

    // logical functions
    func_if,

    // mathematical functions,
    func_int,
    func_rand,

    // string functions
    func_left,
    func_len,
    func_concatenate,

    // date & time functions
    func_now,

    // array functions
    func_mmult,

    // other
    func_subtotal,

    func_wait // dummy function used only for testing.

    // TODO: more functions to come...
};

IXION_DLLPUBLIC const char* get_formula_function_name(formula_function_t func);

}

#endif
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
