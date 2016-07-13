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
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/time.h>
#endif

#define IXION_DEBUG_GLOBAL 0

using namespace std;

namespace ixion {

const char* get_formula_result_output_separator()
{
    static const char* sep =
        "---------------------------------------------------------";
    return sep;
}

double global::get_current_time()
{
#ifdef _WIN32
    FILETIME ft;
    __int64 *time64 = (__int64 *) &ft;
    GetSystemTimeAsFileTime (&ft);
    return *time64 / 10000000.0;
#else
    timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
#endif
}

void global::load_file_content(const string& filepath, string& content)
{
    ifstream file(filepath.c_str());
    if (!file)
        // failed to open the specified file.
        throw file_not_found(filepath);

    ostringstream os;
    os << file.rdbuf() << ' '; // extra char as the end position.
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

// ============================================================================

formula_error::formula_error(formula_error_t fe) :
    m_ferror(fe)
{
}

formula_error::~formula_error() throw()
{
}

const char* formula_error::what() const throw()
{
    return get_formula_error_name(m_ferror);
}

formula_error_t formula_error::get_error() const
{
    return m_ferror;
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
