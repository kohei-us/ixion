/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <ixion/dirty_cell_tracker.hpp>
#include <cassert>

using namespace ixion;

void test_cell_to_cell()
{
    dirty_cell_tracker tracker;
    // A1 to listen to A2.
    tracker.add(abs_address_t(0, 0, 0), abs_address_t(0, 1, 0));
}

int main()
{
    test_cell_to_cell();

    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
