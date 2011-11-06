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

#ifndef __IXION_SESSION_HANDLER_HPP__
#define __IXION_SESSION_HANDLER_HPP__

#include "ixion/interface/session_handler.hpp"
#include "ixion/model_context.hpp"

#include <string>

namespace ixion {

class session_handler : public iface::session_handler
{
public:
    session_handler(const model_context& cxt);
    virtual ~session_handler();

    virtual void begin_cell_interpret(const formula_cell* p);
    virtual void set_result(const formula_result& result);
    virtual void set_invalid_expression(const char* msg);
    virtual void set_formula_error(const char* msg);

    virtual void push_token(fopcode_t fop);
    virtual void push_value(double val);
    virtual void push_single_ref(const address_t& addr, const abs_address_t& pos);
    virtual void push_range_ref(const range_t& range, const abs_address_t& pos);
    virtual void push_function(formula_function_t foc);

private:
    const model_context& m_context;
    std::string m_cell_name;
};

}

#endif
