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

    // A2 no longer tracks A1, and because of this, a modification of A1
    // should not trigger any updates.
    tracker.remove(A2, A1);
    res = tracker.query_dirty_cells(mod_cells);
    assert(res.empty());

    // Add A2->A1 once again, to re-establish the previous chain.
    tracker.add(A2, A1);
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
    abs_range_t C1_D4(0, 0, 2, 4, 2);
    tracker.add(B2, C1_D4);

    // D3 gets modified.  B2 should be updated.
    abs_address_set_t mod_cells;
    mod_cells.emplace(0, 2, 3);
    abs_address_set_t res = tracker.query_dirty_cells(mod_cells);
    assert(res.size() == 1);
    assert(res.count(B2) > 0);

    // E10 listens to A1:B4 which includes B2.
    abs_address_t E10(0, 9, 4);
    abs_range_t A1_B4(0, 0, 0, 4, 2);
    tracker.add(E10, A1_B4);

    // D3 gets modified again. This time both B2 and E10 need updating.
    res = tracker.query_dirty_cells(mod_cells);
    assert(res.size() == 2);
    assert(res.count(B2) > 0);
    assert(res.count(E10) > 0);

    // B2 no longer listens to C1:D4.
    tracker.remove(B2, C1_D4);

    // D3 gets modified again, but no cells need updating.
    res = tracker.query_dirty_cells(mod_cells);
    assert(res.empty());

    // B3 gets modified.  E10 should be updated.
    mod_cells.clear();
    abs_address_t B3(0, 2, 1);
    mod_cells.insert(B3);
    res = tracker.query_dirty_cells(mod_cells);
    assert(res.size() == 1);
    assert(res.count(E10) > 0);
}

void test_volatile_cells()
{
    dirty_cell_tracker tracker;

    // We use sheet 2 in this test.
    abs_address_t A1(1, 0, 0);
    abs_address_t B2(1, 1, 1);
    abs_address_t C3(1, 2, 2);

    tracker.add_volatile(A1);

    // No cells have been modified.
    abs_address_set_t mod_cells;
    abs_address_set_t res = tracker.query_dirty_cells(mod_cells);

    assert(res.size() == 1);
    assert(res.count(A1) > 0);

    tracker.add_volatile(B2);
    res = tracker.query_dirty_cells(mod_cells);
    assert(res.size() == 2);
    assert(res.count(A1) > 0);
    assert(res.count(B2) > 0);

    tracker.add_volatile(C3);
    res = tracker.query_dirty_cells(mod_cells);
    assert(res.size() == 3);
    assert(res.count(A1) > 0);
    assert(res.count(B2) > 0);
    assert(res.count(C3) > 0);

    // Start removing the volatile cells one by one.
    tracker.remove_volatile(B2);
    res = tracker.query_dirty_cells(mod_cells);
    assert(res.size() == 2);
    assert(res.count(A1) > 0);
    assert(res.count(C3) > 0);

    tracker.remove_volatile(C3);
    res = tracker.query_dirty_cells(mod_cells);
    assert(res.size() == 1);
    assert(res.count(A1) > 0);

    tracker.remove_volatile(A1);
    res = tracker.query_dirty_cells(mod_cells);
    assert(res.empty());
}

void test_multi_sheets()
{
    dirty_cell_tracker tracker;

    // B2 on sheet 2 tracks A10 on sheet 1.
    abs_address_t s2_B2(1, 1, 1);
    abs_address_t s1_A10(0, 9, 0);
    tracker.add(s2_B2, s1_A10);

    // A10 on sheet 1 gets modified.
    abs_address_set_t res = tracker.query_dirty_cells(s1_A10);
    assert(res.size() == 1);
    assert(res.count(s2_B2) > 0);

    // A10 on sheet 2 gets modified. This should trigger no updates.
    abs_address_t s2_A10(1, 9, 0);
    res = tracker.query_dirty_cells(s2_A10);
    assert(res.empty());
}

int main()
{
    test_empty_query();
    test_cell_to_cell();
    test_cell_to_range();
    test_volatile_cells();
    test_multi_sheets();

    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
