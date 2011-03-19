/*************************************************************************
 *
 * Copyright (c) 2010 Kohei Yoshida
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 ************************************************************************/

#include "depends_tracker.hpp"
#include "global.hpp"
#include "cell.hpp"
#include "formula_interpreter.hpp"
#include "cell_queue_manager.hpp"
#include "model_context.hpp"
#include "hash_container/map.hpp"

#include <vector>
#include <iostream>
#include <fstream>

#define DEBUG_DEPENDS_TRACKER 1

using namespace std;

namespace ixion {

namespace {

class cell_printer : public unary_function<const base_cell*, void>
{
public:
    cell_printer(const model_context& cxt) : m_cxt(cxt) {}

    void operator() (const base_cell* p) const
    {
        cout << "  " << m_cxt.get_cell_name(p) << endl;
    }

private:
    const model_context& m_cxt;
};

/**
 * Function object to reset the status of formula cell to pre-interpretation
 * status.
 */
struct cell_reset_handler : public unary_function<base_cell*, void>
{
    void operator() (base_cell* p) const
    {
        assert(p->get_celltype() == celltype_formula);
        static_cast<formula_cell*>(p)->reset();
    }
};

class circular_check_handler : public unary_function<base_cell*, void>
{
    const model_context& m_context;
public:
    circular_check_handler(const model_context& cxt) : m_context(cxt) {}

    void operator() (base_cell* p) const
    {
        assert(p->get_celltype() == celltype_formula);
        static_cast<formula_cell*>(p)->check_circular(m_context);
    }
};

struct thread_queue_handler : public unary_function<base_cell*, void>
{
    void operator() (base_cell* p) const
    {
        assert(p->get_celltype() == celltype_formula);
        formula_cell* fcell = static_cast<formula_cell*>(p);
        cell_queue_manager::add_cell(fcell);
    }
};

struct cell_interpret_handler : public unary_function<base_cell*, void>
{
    cell_interpret_handler(const cell_ptr_name_map_t& cell_names, const model_context& cxt) :
        m_cell_names(cell_names), m_context(cxt) {}

    void operator() (base_cell* p) const
    {
        assert(p->get_celltype() == celltype_formula);
        static_cast<formula_cell*>(p)->interpret(m_context);
    }
private:
    const cell_ptr_name_map_t& m_cell_names;
    const model_context& m_context;
};

}

dependency_tracker::cell_back_inserter::cell_back_inserter(vector<base_cell*> & sorted_cells) :
    m_sorted_cells(sorted_cells) {}

void dependency_tracker::cell_back_inserter::operator() (base_cell* cell)
{
    m_sorted_cells.push_back(cell);
}

// ============================================================================

dependency_tracker::dependency_tracker(const cell_ptr_name_map_t* names, const model_context& cxt) :
    mp_names(names), m_context(cxt)
{
}

dependency_tracker::~dependency_tracker()
{
}

void dependency_tracker::insert_depend(base_cell* origin_cell, base_cell* depend_cell)
{
#if DEBUG_DEPENDS_TRACKER
    cout << m_context.get_cell_name(origin_cell) << "->" << m_context.get_cell_name(depend_cell) << endl;
#endif
    m_deps.insert(origin_cell, depend_cell);
}

void dependency_tracker::interpret_all_cells(size_t thread_count)
{
    vector<base_cell*> sorted_cells;
    topo_sort_cells(sorted_cells);

#if DEBUG_DEPENDS_TRACKER
    cout << "Topologically sorted cells ---------------------------------" << endl;
    for_each(sorted_cells.begin(), sorted_cells.end(), cell_printer(m_context));
#endif

    // Reset cell status.
#if DEBUG_DEPENDS_TRACKER
    cout << "Reset cell status ------------------------------------------" << endl;
#endif
    for_each(sorted_cells.begin(), sorted_cells.end(), cell_reset_handler());

    // First, detect circular dependencies and mark those circular 
    // dependent cells with appropriate error flags.
#if DEBUG_DEPENDS_TRACKER
    cout << "Check circular dependencies --------------------------------" << endl;
#endif
    for_each(sorted_cells.begin(), sorted_cells.end(), circular_check_handler(m_context));

    if (thread_count > 0)
    {
        // Interpret cells in topological order using threads.
        cell_queue_manager::init(thread_count, m_context);
        for_each(sorted_cells.begin(), sorted_cells.end(), thread_queue_handler());
        cell_queue_manager::terminate();
    }
    else
    {
        // Interpret cells using just a single thread.
        for_each(sorted_cells.begin(), sorted_cells.end(), cell_interpret_handler(*mp_names, m_context));
    }
}

void dependency_tracker::topo_sort_cells(vector<base_cell*>& sorted_cells) const
{
    cell_back_inserter handler(sorted_cells);
    vector<base_cell*> all_cells;
    all_cells.reserve(mp_names->size());
    cell_ptr_name_map_t::const_iterator itr = mp_names->begin(), itr_end = mp_names->end();
    for (; itr != itr_end; ++itr)
        all_cells.push_back(const_cast<base_cell*>(itr->first));

    dfs_type dfs(all_cells, m_deps.get(), handler);
    dfs.run();
}

}
