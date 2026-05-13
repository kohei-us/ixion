/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once
#include "address_range.hpp"

namespace ixion {

using abs_address_iterator
    [[deprecated("use ixion::abs_address_range")]]
    = abs_address_range;

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
