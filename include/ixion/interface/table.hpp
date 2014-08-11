/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef IXION_INTERFACE_TABLE_HPP
#define IXION_INTERFACE_TABLE_HPP

#include "../env.hpp"

namespace ixion {

struct abs_range_t;

namespace iface {

class table
{
public:
    virtual IXION_DLLPUBLIC ~table();
};

}}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
