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

#ifndef __IXION_FORMULA_FUNCTIONS_HPP__
#define __IXION_FORMULA_FUNCTIONS_HPP__

#include "ixion/global.hpp"

#include <string>
#include <vector>

namespace ixion {

class formula_token_base;
class model_context;

enum formula_function_t
{
    func_unknown = 0,

    func_max,
    func_min,
    func_average,
    func_sum,

    func_wait // dummy function used only for testing.

    // TODO: more functions to come...
};

class formula_functions
{
public:
    class invalid_arg : public general_error
    {
    public:
        invalid_arg(const ::std::string& msg);
    };

    formula_functions(const model_context& cxt);
    ~formula_functions();

    static formula_function_t get_function_opcode(const formula_token_base& token);
    static formula_function_t get_function_opcode(const ::std::string& name);
    static const char* get_function_name(formula_function_t oc);

    void interpret(formula_function_t oc, value_stack_t& args) const;

private:
    void max(value_stack_t& args) const;
    void min(value_stack_t& args) const;
    void sum(value_stack_t& args) const;
    void average(value_stack_t& args) const;
    void wait(value_stack_t& args) const;

private:
    const model_context& m_context;
};

}

#endif
