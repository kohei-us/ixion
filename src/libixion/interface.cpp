/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/interface/table.hpp"
#include "ixion/interface/session_handler.hpp"
#include "ixion/interface/model_context.hpp"

namespace ixion { namespace iface {

table::~table() {}

session_handler::~session_handler() {}

model_context::~model_context() {}

session_handler* model_context::get_session_handler() const
{
    return NULL;
}

}}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
