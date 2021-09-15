/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/global.hpp"
#include "ixion/mem_str_buf.hpp"
#include "ixion/address.hpp"
#include "ixion/matrix.hpp"
#include "ixion/cell.hpp"
#include "ixion/exceptions.hpp"
#include "ixion/formula_result.hpp"
#include "ixion/interface/formula_model_access.hpp"

#include <iostream>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <chrono>

using namespace std;

namespace ixion {

double global::get_current_time()
{
    unsigned long usec_since_epoch =
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

    return usec_since_epoch / 1000000.0;
}

void global::load_file_content(const string& filepath, string& content)
{
    ifstream file(filepath.c_str());
    if (!file)
        // failed to open the specified file.
        throw file_not_found(filepath);

    ostringstream os;
    os << file.rdbuf();
    file.close();

    os.str().swap(content);
}

double global::to_double(const char* p, size_t n)
{
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

bool global::to_bool(const char* p, size_t n)
{
    if (n == 4)
    {
        if (*p++ == 't' && *p++ == 'r' && *p++ == 'u' && *p == 'e')
            return true;
    }

    return false;
}

// ============================================================================

struct formula_error::impl
{
    formula_error_t error;
    std::string msg;
    std::string buffer;

    impl(formula_error_t _error) :
        error(_error) {}

    impl(formula_error_t _error, std::string _msg) :
        error(_error), msg(std::move(_msg)) {}
};

formula_error::formula_error(formula_error_t fe) :
    mp_impl(std::make_unique<impl>(fe)) {}

formula_error::formula_error(formula_error_t fe, std::string msg) :
    mp_impl(std::make_unique<impl>(fe, std::move(msg))) {}

formula_error::formula_error(formula_error&& other) :
    mp_impl(std::move(other.mp_impl))
{
    other.mp_impl = std::make_unique<impl>(formula_error_t::no_error);
}

formula_error::~formula_error() throw()
{
}

const char* formula_error::what() const throw()
{
    const char* error_name = get_formula_error_name(mp_impl->error);
    if (mp_impl->msg.empty())
        return error_name;

    std::ostringstream os;
    os << mp_impl->msg << " (type: " << error_name << ")";
    mp_impl->buffer = os.str();
    return mp_impl->buffer.data();
}

formula_error_t formula_error::get_error() const
{
    return mp_impl->error;
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
