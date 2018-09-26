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

void test_empty_query()
{
    dirty_cell_tracker tracker;

    // Empty query.
    abs_address_set_t mod_cells;
    abs_address_set_t res = tracker.query_dirty_cells(mod_cells);
    assert(res.empty());

    // A "modified" cell is outside existing sheet range. Make sure we don't
    // crash here.
    mod_cells.emplace(1, 1, 1);
    res = tracker.query_dirty_cells(mod_cells);
    assert(res.empty());
}

void test_cell_to_cell()
{
    dirty_cell_tracker tracker;

    // A2 to listen to A1.
    abs_address_t A1(0, 0, 0);
    abs_address_t A2(0, 1, 0);
    tracker.add(A2, A1);

    // A1 is modified.  A2 should be updated.
    abs_address_set_t mod_cells;
    mod_cells.insert(A1);
    abs_address_set_t res = tracker.query_dirty_cells(mod_cells);
    assert(res.size() == 1);
    abs_address_t cell = *res.cbegin();
    assert(cell == A2);

    // A3 to listen to A2.
    abs_address_t A3(0, 2, 0);
    tracker.add(A3, A2);

    // A1 is modified.  Both A2 and A3 should be updated.
    res = tracker.query_dirty_cells(mod_cells);
    assert(res.size() == 2);

    assert(res.count(A2) > 0);
    assert(res.count(A3) > 0);
}

void test_cell_to_range()
{
    dirty_cell_tracker tracker;

    // B2 listens to C1:D4.
    abs_address_t B2(0, 1, 1);
    tracker.add(B2, abs_range_t(0, 0, 2, 4, 2));

    // D3 gets modified.  B2 should be updated.
    abs_address_set_t mod_cells;
    mod_cells.emplace(0, 2, 3);
    abs_address_set_t res = tracker.query_dirty_cells(mod_cells);
    assert(res.size() == 1);
    assert(res.count(B2) > 0);

    // E10 listens to A1:B4 which includes B2.
    abs_address_t E10(0, 9, 4);
    tracker.add(E10, abs_range_t(0, 0, 0, 4, 2));

    // D3 gets modified again. This time both B2 and E10 need updating.
    res = tracker.query_dirty_cells(mod_cells);
    assert(res.size() == 2);
    assert(res.count(B2) > 0);
    assert(res.count(E10) > 0);
}

int main()
{
    test_empty_query();
    test_cell_to_cell();
    test_cell_to_range();

    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
