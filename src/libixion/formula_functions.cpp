/*************************************************************************
 *
 * Copyright (c) 2010, 2011 Kohei Yoshida
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 ************************************************************************/

#include "ixion/formula_functions.hpp"
#include "ixion/formula_tokens.hpp"
#include "ixion/matrix.hpp"
#include "ixion/mem_str_buf.hpp"
#include "ixion/interface/model_context.hpp"

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

#define DEBUG_FORMULA_FUNCTIONS 0

#include <iostream>
#include <sstream>

using namespace std;

namespace ixion {

namespace {

struct builtin_func
{
    const char*         name;
    formula_function_t  oc;
};

const builtin_func builtin_funcs[] = {
    { "MAX", func_max },
    { "MIN", func_min },
    { "AVERAGE", func_average },
    { "WAIT", func_wait },
    { "SUM", func_sum },
    { "LEN", func_len },
    { "CONCATENAME", func_concatenate },
    { "NOW", func_now },
};

size_t builtin_func_count = sizeof(builtin_funcs) / sizeof(builtin_func);

const char* unknown_func_name = "unknown";

/**
 * Traverse all elements of a passed matrix to sum up their values.
 */
double sum_matrix_elements(const matrix& mx)
{
    double sum = 0.0;
    matrix::size_pair_type sz = mx.size();
    for (size_t row = 0; row < sz.first; ++row)
        for (size_t col = 0; col < sz.second; ++col)
            sum += mx.get_numeric(row, col);

    return sum;
}

/**
 * Compare given string with a known function name to see if they are equal.
 * The comparison is case-insensitive.
 *
 * @param func_name function name. Null-terminated, and is all in upper
 *                  case.
 * @param p string to test. Not null-terminated and may be in mixed case.
 * @param n length of the string being tested.
 *
 * @return true if they are equal, false otherwise.
 */
bool match_func_name(const char* func_name, const char* p, size_t n)
{
    const char* fp = func_name;
    for (size_t i = 0; i < n; ++i, ++p, ++fp)
    {
        if (!*fp)
            // function name ended first.
            return false;

        char c = *p;
        if (c > 'Z')
            // convert to upper case.
            c -= 'a' - 'A';

        if (c != *fp)
            return false;
    }

    return *fp == 0;
}

}

// ============================================================================

formula_functions::invalid_arg::invalid_arg(const string& msg) :
    general_error(msg) {}

formula_function_t formula_functions::get_function_opcode(const formula_token_base& token)
{
    assert(token.get_opcode() == fop_function);
    return static_cast<formula_function_t>(token.get_index());
}

formula_function_t formula_functions::get_function_opcode(const char* p, size_t n)
{
    for (size_t i = 0; i < builtin_func_count; ++i)
    {
        if (match_func_name(builtin_funcs[i].name, p, n))
            return builtin_funcs[i].oc;
    }
    return func_unknown;
}

const char* formula_functions::get_function_name(formula_function_t oc)
{
    for (size_t i = 0; i < builtin_func_count; ++i)
    {
        if (oc == builtin_funcs[i].oc)
            return builtin_funcs[i].name;
    }
    return unknown_func_name;
}

formula_functions::formula_functions(iface::model_context& cxt) :
    m_context(cxt)
{
}

formula_functions::~formula_functions()
{
}

void formula_functions::interpret(formula_function_t oc, value_stack_t& args)
{
    switch (oc)
    {
        case func_max:
            max(args);
            break;
        case func_average:
            average(args);
            break;
        case func_min:
            min(args);
            break;
        case func_wait:
            wait(args);
            break;
        case func_sum:
            sum(args);
            break;
        case func_len:
            len(args);
            break;
        case func_concatenate:
            concatenate(args);
            break;
        case func_now:
            now(args);
            break;
        case func_unknown:
        default:
            throw formula_functions::invalid_arg("unknown function opcode");
    }
}

void formula_functions::max(value_stack_t& args) const
{
    if (args.empty())
        throw formula_functions::invalid_arg("MAX requires one or more arguments.");

    double ret = args.pop_value();
    while (!args.empty())
    {
        double v = args.pop_value();
        if (v > ret)
            ret = v;
    }
    args.push_value(ret);
}

void formula_functions::min(value_stack_t& args) const
{
    if (args.empty())
        throw formula_functions::invalid_arg("MIN requires one or more arguments.");

    double ret = args.pop_value();
    while (!args.empty())
    {
        double v = args.pop_value();
        if (v < ret)
            ret = v;
    }
    args.push_value(ret);
}

void formula_functions::sum(value_stack_t& args) const
{
#if DEBUG_FORMULA_FUNCTIONS
    __IXION_DEBUG_OUT__ << "function: sum" << endl;
#endif
    if (args.empty())
        throw formula_functions::invalid_arg("SUM requires one or more arguments.");

    double ret = 0;
    while (!args.empty())
    {

        switch (args.get_type())
        {
            case sv_range_ref:
                ret += sum_matrix_elements(args.pop_range_value());
            break;
            case sv_single_ref:
            case sv_string:
            case sv_value:
            default:
                ret += args.pop_value();
        }
    }

    args.push_value(ret);

#if DEBUG_FORMULA_FUNCTIONS
    __IXION_DEBUG_OUT__ << "function: sum end" << endl;
#endif
}

void formula_functions::average(value_stack_t& args) const
{
    if (args.empty())
        throw formula_functions::invalid_arg("AVERAGE requires one or more arguments.");

    double ret = 0;
    double count = 0.0;
    while (!args.empty())
    {
        ret += args.pop_value();
        ++count;
    }

    args.push_value(ret/count);
}

void formula_functions::len(value_stack_t& args) const
{
    if (args.size() != 1)
        throw formula_functions::invalid_arg("LEN requires exactly one argument.");

    string s = args.pop_string();
    args.clear();
    args.push_value(s.size());
}

void formula_functions::concatenate(value_stack_t& args)
{
    string s;
    while (!args.empty())
        s = args.pop_string() + s;
    size_t sid = m_context.add_string(&s[0], s.size());
    args.push_string(sid);
}

void formula_functions::now(value_stack_t& args) const
{
    if (!args.empty())
        throw formula_functions::invalid_arg("NOW takes no argument.");

    // TODO: this value is currently not accurate since we don't take into
    // account the zero date yet.
    double cur_time = global::get_current_time();
    cur_time /= 86400.0; // convert seconds to days.
    args.push_value(cur_time);
}

void formula_functions::wait(value_stack_t& args) const
{
    global::sleep(1);
    args.clear();
    args.push_value(1);
}

}
