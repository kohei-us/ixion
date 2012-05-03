/*************************************************************************
 *
 * Copyright (c) 2011 Kohei Yoshida
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

#ifndef __IXION_INTERFACE_SESSION_HANDLER_HPP__
#define __IXION_INTERFACE_SESSION_HANDLER_HPP__

#include "ixion/formula_opcode.hpp"
#include "ixion/formula_function_opcode.hpp"

#include <cstdlib>

namespace ixion {

class formula_cell;
class formula_result;
struct address_t;
struct range_t;
struct abs_address_t;

namespace iface {

class session_handler
{
public:
    virtual ~session_handler() {}

    virtual void begin_cell_interpret(const abs_address_t& pos) = 0;
    virtual void set_result(const formula_result& result) = 0;
    virtual void set_invalid_expression(const char* msg) = 0;
    virtual void set_formula_error(const char* msg) = 0;
    virtual void push_token(fopcode_t fop) = 0;
    virtual void push_value(double val) = 0;
    virtual void push_string(size_t sid) = 0;
    virtual void push_single_ref(const address_t& addr, const abs_address_t& pos) = 0;
    virtual void push_range_ref(const range_t& range, const abs_address_t& pos) = 0;
    virtual void push_function(formula_function_t foc) = 0;
};

}}

#endif
