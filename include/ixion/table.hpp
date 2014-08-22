/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef IXION_TABLE_HPP
#define IXION_TABLE_HPP

#include "env.hpp"
#include "types.hpp"

namespace ixion {

struct IXION_DLLPUBLIC table_t
{
    string_id_t name;
    string_id_t column;
    table_areas_t areas;

    table_t();
};

}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
