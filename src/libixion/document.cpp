/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/document.hpp"
#include "ixion/global.hpp"
#include "ixion/model_context.hpp"

namespace ixion {

struct document::impl
{
    model_context cxt;

    impl() {}
};

document::document() :
    mp_impl(ixion::make_unique<impl>()) {}

document::~document() {}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
