/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "queue_entry.hpp"

namespace ixion {

queue_entry::queue_entry(formula_cell* _p, const abs_address_t& _pos) :
    p(_p), pos(_pos) {}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
