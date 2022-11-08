/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/table.hpp"

namespace ixion {

table_t::table_t() :
    name(empty_string_id),
    column_first(empty_string_id),
    column_last(empty_string_id),
    areas(table_area_none) {}

bool table_t::operator== (const table_t& r) const
{
    return name == r.name && column_first == r.column_first && column_last == r.column_first && areas == r.areas;
}

bool table_t::operator!= (const table_t& r) const
{
    return !operator==(r);
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
