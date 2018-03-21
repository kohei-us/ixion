/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "calc_status.hpp"

namespace ixion {

calc_status::calc_status() : result(nullptr), refcount(0) {}

void calc_status::add_ref()
{
    ++refcount;
}

void calc_status::release_ref()
{
    if (--refcount == 0)
        delete this;
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
