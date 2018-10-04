/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "topo_sort_calculator.hpp"

#if IXION_THREADS
#include "cell_queue_manager.hpp"
#endif

#include "ixion/global.hpp"
#include "ixion/cell.hpp"
#include "ixion/formula_name_resolver.hpp"
#include "ixion/interface/formula_model_access.hpp"

#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>

using namespace std;

namespace ixion {

topo_sort_calculator::topo_sort_calculator(
    const abs_address_set_t& dirty_cells, iface::formula_model_access& cxt) :
    m_dirty_cells(dirty_cells), m_context(cxt)
{
}

topo_sort_calculator::~topo_sort_calculator()
{
}

void topo_sort_calculator::set_reference_relation(
    const abs_address_t& src, const abs_address_t& dest)
{
    m_deps.insert(src, dest);
}

void topo_sort_calculator::interpret_all_cells(size_t thread_count)
{
#if IXION_THREADS == 0
    thread_count = 0;  // threads are disabled thus not to be used.
#endif

    std::vector<abs_address_t> sorted_cells = sort_cells();

    // Reset cell status.
    std::for_each(sorted_cells.begin(), sorted_cells.end(),
        [&](const abs_address_t& pos)
        {
            formula_cell* p = m_context.get_formula_cell(pos);
            p->reset();
        }
    );

    // First, detect circular dependencies and mark those circular
    // dependent cells with appropriate error flags.
    std::for_each(sorted_cells.begin(), sorted_cells.end(),
        [&](const abs_address_t& pos)
        {
            formula_cell* p = m_context.get_formula_cell(pos);
            p->check_circular(m_context, pos);
        }
    );

    if (!thread_count)
    {
        // Interpret cells using just a single thread.
        std::for_each(sorted_cells.begin(), sorted_cells.end(),
            [&](const abs_address_t& pos)
            {
                formula_cell* p = m_context.get_formula_cell(pos);
                p->interpret(m_context, pos);
            }
        );
        return;
    }

#if IXION_THREADS
    // Interpret cells in topological order using threads.
    formula_cell_queue queue(m_context, std::move(sorted_cells), thread_count);
    queue.run();
#endif
}

std::vector<abs_address_t> topo_sort_calculator::sort_cells() const
{
    std::vector<abs_address_t> sorted_cells;
    dfs_type::back_inserter handler(sorted_cells);

    vector<abs_address_t> all_cells;
    all_cells.reserve(m_dirty_cells.size());
    abs_address_set_t::const_iterator itr = m_dirty_cells.begin(), itr_end = m_dirty_cells.end();
    for (; itr != itr_end; ++itr)
        all_cells.push_back(*itr);

    dfs_type dfs(all_cells, m_deps, handler);
    dfs.run();

    return sorted_cells;
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
