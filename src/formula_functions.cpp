/*************************************************************************
 *
 * Copyright (c) 2010 Kohei Yoshida
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

#include "formula_functions.hpp"
#include "formula_tokens.hpp"

#include <unistd.h>
#include <iostream>

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
    { "WAIT", func_wait }
};

size_t builtin_func_count = sizeof(builtin_funcs) / sizeof(builtin_func);

const char* unknown_func_name = "unknown";

}

// ============================================================================

formula_functions::invalid_arg::invalid_arg(const string& msg) :
    general_error(msg) {}

formula_function_t formula_functions::get_function_opcode(const formula_token_base& token)
{
    assert(token.get_opcode() == fop_function);
    return static_cast<formula_function_t>(token.get_index());
}

formula_function_t formula_functions::get_function_opcode(const string& name)
{
    for (size_t i = 0; i < builtin_func_count; ++i)
    {
        if (name == string(builtin_funcs[i].name))
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

double formula_functions::interpret(formula_function_t oc, const args_type& args)
{
    switch (oc)
    {
        case func_max:
            return formula_functions::max(args);
        case func_average:
            break;
        case func_min:
            break;
        case func_wait:
            return wait(args);
        case func_unknown:
        default:
            ;
    }
    return 0.0;
}

double formula_functions::max(const args_type& args)
{
    if (args.empty())
        throw formula_functions::invalid_arg("MAX requires one or more arguments.");

    args_type::const_iterator itr = args.begin(), itr_end = args.end();
    double ret = *itr;
    for (++itr; itr != itr_end; ++itr)
    {
        if (*itr > ret)
            ret = *itr;
    }
    return ret;
}

double formula_functions::wait(const args_type& args)
{
    sleep(1);
    return 1;
}

}
