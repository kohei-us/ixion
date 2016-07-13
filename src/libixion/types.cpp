/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/types.hpp"

#include <limits>

namespace ixion {

const sheet_t global_scope = -1;
const sheet_t invalid_sheet = -2;

const string_id_t empty_string_id = std::numeric_limits<string_id_t>::max();

const char* get_formula_error_name(formula_error_t fe)
{
    static const char* default_err_name = "#ERR!";
    static const char* names[] = {
        "",        // no error
        "#REF!",   // result not available
        "#DIV/0!", // division by zero
        "#NUM!"    // invalid expression
    };
    static const size_t name_size = 4;
    if (static_cast<size_t>(fe) < name_size)
        return names[static_cast<size_t>(fe)];

    return default_err_name;
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
