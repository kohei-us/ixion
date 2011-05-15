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

#include "ixion/formula_functions.hpp"
#include "ixion/formula_tokens.hpp"
#include "ixion/model_context.hpp"

#ifdef max
#undef max
#endif

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
    { "WAIT", func_wait },
    { "SUM", func_sum }
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

formula_functions::formula_functions(const model_context& cxt) :
    m_context(cxt)
{
}

formula_functions::~formula_functions()
{
}

double formula_functions::interpret(formula_function_t oc, value_stack_t& args) const
{
    switch (oc)
    {
        case func_max:
            return max(args);
        case func_average:
            return average(args);
        case func_min:
            return min(args);
        case func_wait:
            return wait(args);
        case func_sum:
            return sum(args);
        case func_unknown:
        default:
            ;
    }
    return 0.0;
}

double formula_functions::max(const value_stack_t& args) const
{
    if (args.empty())
        throw formula_functions::invalid_arg("MAX requires one or more arguments.");

    value_stack_t::const_iterator itr = args.begin(), itr_end = args.end();
    double ret = itr->get_value();
    for (++itr; itr != itr_end; ++itr)
    {
        if (itr->get_value() > ret)
            ret = itr->get_value();
    }
    return ret;
}

double formula_functions::min(const value_stack_t& args) const
{
    if (args.empty())
        throw formula_functions::invalid_arg("MIN requires one or more arguments.");

    value_stack_t::const_iterator itr = args.begin(), itr_end = args.end();
    double ret = itr->get_value();
    for (++itr; itr != itr_end; ++itr)
    {
        if (itr->get_value() < ret)
            ret = itr->get_value();
    }
    return ret;
}

double formula_functions::sum(const value_stack_t& args) const
{
    if (args.empty())
        throw formula_functions::invalid_arg("SUM requires one or more arguments.");

    value_stack_t::const_iterator itr = args.begin(), itr_end = args.end();
    double ret = itr->get_value();
    for (++itr; itr != itr_end; ++itr)
    {
        ret += itr->get_value();
    }
    return ret;
}

double formula_functions::average(const value_stack_t& args) const
{
    if (args.empty())
        throw formula_functions::invalid_arg("AVERAGE requires one or more arguments.");

    value_stack_t::const_iterator itr = args.begin(), itr_end = args.end();
    double ret = itr->get_value();
    long count = 1;
    for (++itr; itr != itr_end; ++itr)
    {
        ret += itr->get_value();
        ++count;
    }
    return ret / count;
}

double formula_functions::wait(const value_stack_t& args) const
{
    global::sleep(1);
    return 1;
}

}
