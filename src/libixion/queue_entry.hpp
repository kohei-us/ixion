/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_QUEUE_ENTRY_HPP
#define INCLUDED_QUEUE_ENTRY_HPP

#include "ixion/address.hpp"

namespace ixion {

class formula_cell;

struct queue_entry
{
    formula_cell* p;
    abs_address_t pos;

    queue_entry(formula_cell* _p, const abs_address_t& _pos);
};

}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
