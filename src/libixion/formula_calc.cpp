/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/formula.hpp"
#include "ixion/address.hpp"
#include "ixion/cell.hpp"
#include "ixion/formula_name_resolver.hpp"

#include "queue_entry.hpp"
#include "debug.hpp"

#if IXION_THREADS
#include "cell_queue_manager.hpp"
#endif

#include <algorithm>

namespace ixion {

namespace {

class calc_scope
{
    iface::formula_model_access& m_cxt;
public:
    calc_scope(iface::formula_model_access& cxt) : m_cxt(cxt)
    {
        m_cxt.notify(formula_event_t::calculation_begins);
    }

    ~calc_scope()
    {
        m_cxt.notify(formula_event_t::calculation_ends);
    }
};

}

void calculate_sorted_cells(
    iface::formula_model_access& cxt, const std::vector<abs_range_t>& formula_cells, size_t thread_count)
{
#if IXION_THREADS == 0
    thread_count = 0;  // threads are disabled thus not to be used.
#endif

    calc_scope cs(cxt);

    std::vector<queue_entry> entries;
    entries.reserve(formula_cells.size());

    for (const abs_range_t& r : formula_cells)
        entries.emplace_back(cxt.get_formula_cell(r.first), r.first);

    // Reset cell status.
    for (queue_entry& e : entries)
    {
        e.p->reset();
        IXION_TRACE("pos=" << e.pos.get_name() << " formula=" << detail::print_formula_expression(cxt, e.pos, *e.p));
    }

    // First, detect circular dependencies and mark those circular
    // dependent cells with appropriate error flags.
    for (queue_entry& e : entries)
        e.p->check_circular(cxt, e.pos);

    if (!thread_count)
    {
        // Interpret cells using just a single thread.
        for (queue_entry& e : entries)
            e.p->interpret(cxt, e.pos);

        return;
    }

#if IXION_THREADS
    // Interpret cells in topological order using threads.
    formula_cell_queue queue(cxt, std::move(entries), thread_count);
    queue.run();
#endif
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
