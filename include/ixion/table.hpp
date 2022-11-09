/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_TABLE_HPP
#define INCLUDED_IXION_TABLE_HPP

#include "types.hpp"

namespace ixion {

struct IXION_DLLPUBLIC table_t
{
    string_id_t name;
    string_id_t column_first;
    string_id_t column_last;
    table_areas_t areas;

    table_t();

    bool operator== (const table_t& r) const;
    bool operator!= (const table_t& r) const;
};

IXION_DLLPUBLIC std::ostream& operator<<(std::ostream& os, const table_t& table);

}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
