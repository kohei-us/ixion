/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/interface/table_handler.hpp"
#include "ixion/interface/session_handler.hpp"
#include "ixion/interface/formula_model_access.hpp"

namespace ixion { namespace iface {

table_handler::~table_handler() {}

session_handler::~session_handler() {}

formula_model_access::formula_model_access() {}
formula_model_access::~formula_model_access() {}

std::unique_ptr<session_handler> formula_model_access::create_session_handler()
{
    return std::unique_ptr<session_handler>();
}

table_handler* formula_model_access::get_table_handler()
{
    return NULL;
}

const table_handler* formula_model_access::get_table_handler() const
{
    return NULL;
}

}}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
