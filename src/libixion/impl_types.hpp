/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <variant>

namespace ixion {

class matrix;

using matrix_or_value_t = std::variant<matrix, double>;

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
