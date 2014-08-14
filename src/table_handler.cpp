/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "table_handler.hpp"

namespace ixion {

table_handler::entry::entry() :
    name(empty_string_id), range(abs_range_t::invalid), totals_row_count(0) {}

table_handler::~table_handler() {}

void table_handler::insert(entry* p)
{
    if (!p)
        return;

    unique_ptr<entry> px(p);
    string_id_t name = p->name;
    m_entries.insert(name, px.release());
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
