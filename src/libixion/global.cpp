/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/global.hpp"
#include "ixion/address.hpp"
#include "ixion/matrix.hpp"
#include "ixion/cell.hpp"
#include "ixion/exceptions.hpp"
#include "ixion/formula_result.hpp"

#include <iostream>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <chrono>

using namespace std;

namespace ixion {

double get_current_time()
{
    unsigned long usec_since_epoch =
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

    return usec_since_epoch / 1000000.0;
}

double to_double(std::string_view s)
{
    const char* p = s.data();
    std::size_t n = s.size();

    if (!n)
        return 0.0;

    // First, use the standard C API.
    const char* p_last_check = p + n;
    char* p_last;
    double val = strtod(p, &p_last);
    if (p_last == p_last_check)
        return val;

    // If that fails, do the manual conversion, which may introduce rounding
    // errors.  Revise this to reduce the amount of rounding error.
    bool dot = false;
    double frac = 1.0;
    double sign = 1.0;
    for (size_t i = 0; i < n; ++i, ++p)
    {
        if (i == 0)
        {
            if (*p == '+')
                // Skip this.
                continue;

            if (*p == '-')
            {
                sign = -1.0;
                continue;
            }
        }
        if (*p == '.')
        {
            if (dot)
                // Second dot is not allowed.
                break;
            dot = true;
            continue;
        }

        if (*p < '0' || '9' < *p)
            // not a digit.  End the parse.
            break;

        int digit = *p - '0';
        if (dot)
        {
            frac *= 0.1;
            val += digit * frac;
        }
        else
        {
            val *= 10.0;
            val += digit;
        }
    }
    return sign*val;
}

bool to_bool(std::string_view s)
{
    return s == "true";
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
