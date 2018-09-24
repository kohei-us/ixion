/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <ixion/dirty_cell_tracker.hpp>
#include <cassert>
#include <iostream>

using namespace ixion;

void test_cell_to_cell()
{
    dirty_cell_tracker tracker;

    // A2 to listen to A1.
    tracker.add(abs_address_t(0, 1, 0), abs_address_t(0, 0, 0));

    // A1 is modified.  A2 should be updated.
    abs_address_set_t mod_cells;
    mod_cells.emplace(0, 0, 0);
    abs_address_set_t res = tracker.query_dirty_cells(mod_cells);
    assert(res.size() == 1);
    abs_address_t cell = *res.cbegin();
    assert(cell == abs_address_t(0, 1, 0));

    // A3 to listen to A2.
    tracker.add(abs_address_t(0, 2, 0), abs_address_t(0, 1, 0));

    // A1 is modified.  Both A2 and A3 should be updated.
    res = tracker.query_dirty_cells(mod_cells);
    assert(res.size() == 2);

    assert(res.count(abs_address_t(0, 1, 0)) > 0);
    assert(res.count(abs_address_t(0, 2, 0)) > 0);
}

int main()
{
    test_cell_to_cell();

    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
