/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once
#include "types.hpp"

#include <string_view>

namespace ixion {

struct IXION_DLLPUBLIC table_t
{
    std::string_view name;
    std::string_view column_first;
    std::string_view column_last;
    table_areas_t areas;

    table_t();

    bool operator== (const table_t& r) const;
};

IXION_DLLPUBLIC std::ostream& operator<<(std::ostream& os, const table_t& table);

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
