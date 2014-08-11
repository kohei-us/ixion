/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __IXION_INTERFACE_SESSION_HANDLER_HPP__
#define __IXION_INTERFACE_SESSION_HANDLER_HPP__

#include "ixion/formula_opcode.hpp"
#include "ixion/formula_function_opcode.hpp"
#include "../env.hpp"

#include <cstdlib>

namespace ixion {

class formula_cell;
class formula_result;
struct address_t;
struct range_t;
struct abs_address_t;

namespace iface {

class IXION_DLLPUBLIC session_handler
{
public:
    virtual ~session_handler();

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
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
