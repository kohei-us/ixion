/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "formula_groups.hpp"

#include <cassert>

namespace ixion { namespace detail {

std::vector<formula_group_entry> get_formula_groups(const column_store_t& col)
{
    std::vector<formula_group_entry> groups;

    row_t block_row = 0;

    for (auto it = col.cbegin(); it != col.cend(); block_row += it->size, ++it)
    {
        if (it->type != element_type_formula)
            continue;

        for (std::size_t offset = 0; offset < it->size;)
        {
            formula_cell** cells = &formula_element_block::at(*it->data, offset);
            // A bug-free formula element block never stores a null cell.
            assert(cells[0]);

            formula_group_t group = cells[0]->get_group_properties();
            row_t size = group.grouped ? group.size.row : 1;
            assert(offset + size <= it->size);

            groups.push_back({cells, block_row + static_cast<row_t>(offset), size});
            offset += size;
        }
    }

    return groups;
}

}}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
