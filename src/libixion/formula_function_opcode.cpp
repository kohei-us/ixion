/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/formula_function_opcode.hpp"

namespace ixion {

bool is_volatile(formula_function_t func)
{
    switch (func)
    {
        case func_now:
            return true;
        default:
            ;
    }
    return false;
}

}
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
