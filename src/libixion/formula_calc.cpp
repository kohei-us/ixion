/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/formula.hpp"
#include "ixion/address.hpp"
#include "ixion/cell.hpp"

#if IXION_THREADS
#include "cell_queue_manager.hpp"
#endif

#include <algorithm>

namespace ixion {

void calculate_sorted_cells(
    iface::formula_model_access& cxt, const std::vector<abs_range_t>& formula_cells, size_t thread_count)
{
#if IXION_THREADS == 0
    thread_count = 0;  // threads are disabled thus not to be used.
#endif

    struct entry
    {
        formula_cell* p;
        abs_address_t pos;

        entry(formula_cell* _p, const abs_address_t& _pos) :
            p(_p), pos(_pos) {}
    };

    std::vector<entry> entries;
    entries.reserve(formula_cells.size());

    for (const abs_range_t& r : formula_cells)
        entries.emplace_back(cxt.get_formula_cell(r.first), r.first);

    // Reset cell status.
    for (entry& e : entries)
        e.p->reset();

    // First, detect circular dependencies and mark those circular
    // dependent cells with appropriate error flags.
    for (entry& e : entries)
        e.p->check_circular(cxt, e.pos);


    if (!thread_count)
    {
        // Interpret cells using just a single thread.
        for (entry& e : entries)
            e.p->interpret(cxt, e.pos);

        return;
    }

#if IXION_THREADS
    // Interpret cells in topological order using threads.
    std::vector<abs_address_t> addrs;
    for (entry& e : entries)
        addrs.push_back(e.pos);

    formula_cell_queue queue(cxt, std::move(addrs), thread_count);
    queue.run();
#endif
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
