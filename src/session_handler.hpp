/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_SESSION_HANDLER_HPP
#define INCLUDED_IXION_SESSION_HANDLER_HPP

#include "ixion/interface/session_handler.hpp"
#include "ixion/model_context.hpp"

#include <memory>

namespace ixion {

class session_handler : public iface::session_handler
{
    struct impl;
    std::unique_ptr<impl> mp_impl;

public:
    session_handler(const model_context& cxt, bool show_sheet_name);
    virtual ~session_handler();

    virtual void begin_cell_interpret(const abs_address_t& pos) override;
    virtual void end_cell_interpret() override;
    virtual void set_result(const formula_result& result) override;
    virtual void set_invalid_expression(const char* msg) override;
    virtual void set_formula_error(const char* msg) override;

    virtual void push_token(fopcode_t fop) override;
    virtual void push_value(double val) override;
    virtual void push_string(size_t sid) override;
    virtual void push_single_ref(const address_t& addr, const abs_address_t& pos) override;
    virtual void push_range_ref(const range_t& range, const abs_address_t& pos) override;
    virtual void push_table_ref(const table_t& table) override;
    virtual void push_function(formula_function_t foc) override;

    class factory : public model_context::session_handler_factory
    {
        const model_context& m_context;
        bool m_show_sheet_name;
    public:
        factory(const model_context& cxt);

        virtual std::unique_ptr<iface::session_handler> create() override;

        void show_sheet_name(bool b);
    };

    /**
     * Print string to stdout in a thread-safe way.
     *
     * @param msg string to print to stdout.
     */
    static void print(const std::string& msg);
};

}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
