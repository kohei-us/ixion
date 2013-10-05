/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/types.hpp"

#include <limits>

namespace ixion {

const sheet_t global_scope = -1;
const sheet_t invalid_sheet = -2;

const string_id_t empty_string_id = std::numeric_limits<string_id_t>::max();

}
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
