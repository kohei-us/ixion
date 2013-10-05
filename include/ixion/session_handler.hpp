/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

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

    virtual void begin_cell_interpret(const abs_address_t& pos);
    virtual void set_result(const formula_result& result);
    virtual void set_invalid_expression(const char* msg);
    virtual void set_formula_error(const char* msg);

    virtual void push_token(fopcode_t fop);
    virtual void push_value(double val);
    virtual void push_string(size_t sid);
    virtual void push_single_ref(const address_t& addr, const abs_address_t& pos);
    virtual void push_range_ref(const range_t& range, const abs_address_t& pos);
    virtual void push_function(formula_function_t foc);

private:
    const model_context& m_context;
    std::string m_cell_name;
};

}

#endif
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
