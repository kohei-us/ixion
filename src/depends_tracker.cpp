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
#include "depth_first_search.hpp"
#include "formula_interpreter.hpp"
#include "cell_queue_manager.hpp"

#include <vector>
#include <unordered_map>
#include <iostream>
#include <fstream>

#define DEBUG_DEPENDS_TRACKER 0

using namespace std;

namespace ixion {

namespace {

class cell_printer : public unary_function<const base_cell*, void>
{
public:
    cell_printer(const cell_ptr_name_map_t* names) : mp_names(names) {}

    void operator() (const base_cell* p) const
    {
        cell_ptr_name_map_t::const_iterator itr = mp_names->find(p);
        if (itr == mp_names->end())
            cout << "  unknown cell (" << p << ")" << endl;
        else
            cout << "  " << itr->second << endl;
    }

private:
    const cell_ptr_name_map_t* mp_names;
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

struct circular_check_handler : public unary_function<base_cell*, void>
{
    void operator() (base_cell* p) const
    {
        assert(p->get_celltype() == celltype_formula);
        static_cast<formula_cell*>(p)->check_circular();
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
    cell_interpret_handler(const cell_ptr_name_map_t& cell_names) :
        m_cell_names(cell_names) {}

    void operator() (base_cell* p) const
    {
        assert(p->get_celltype() == celltype_formula);
        static_cast<formula_cell*>(p)->interpret(m_cell_names);
    }
private:
    const cell_ptr_name_map_t& m_cell_names;
};

class cell_back_inserter : public depth_first_search::cell_handler
{
public:
    cell_back_inserter(vector<base_cell*>& sorted_cells) :
        m_sorted_cells(sorted_cells) {}

    virtual void operator() (base_cell* cell)
    {
        m_sorted_cells.push_back(cell);
    }
private:
    vector<base_cell*>& m_sorted_cells;
};

}

depends_tracker::depends_tracker(const cell_ptr_name_map_t* names) :
    mp_names(names)
{
}

depends_tracker::~depends_tracker()
{
}

void depends_tracker::insert_depend(const formula_cell* origin_cell, const base_cell* depend_cell)
{
//  cout << "origin cell: " << origin_cell << "  depend cell: " << depend_cell << endl;
    depend_map_type::iterator itr = m_map.find(origin_cell);
    if (itr == m_map.end())
    {
        // First dependent cell for this origin cell.
        pair<depend_map_type::iterator, bool> r = m_map.insert(origin_cell, new depend_cells_type);
        if (!r.second)
            throw general_error("failed to insert a new set instance");

        itr = r.first;
    }

    itr->second->insert(depend_cell);
//  cout << "map count: " << m_map.size() << "  depend count: " << itr->second->size() << endl;
}

void depends_tracker::interpret_all_cells(size_t thread_count)
{
    vector<base_cell*> sorted_cells;
    topo_sort_cells(sorted_cells);

#if DEBUG_DEPENDS_TRACKER
    cout << "Topologically sorted cells ---------------------------------" << endl;
    for_each(sorted_cells.begin(), sorted_cells.end(), cell_printer(mp_names));
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
    for_each(sorted_cells.begin(), sorted_cells.end(), circular_check_handler());

    if (thread_count > 0)
    {
        // Interpret cells in topological order using threads.
        cell_queue_manager::init(thread_count, *mp_names);
        for_each(sorted_cells.begin(), sorted_cells.end(), thread_queue_handler());
        cell_queue_manager::terminate();
    }
    else
    {
        // Interpret cells using just a single thread.
        for_each(sorted_cells.begin(), sorted_cells.end(), cell_interpret_handler(*mp_names));
    }
}

void depends_tracker::topo_sort_cells(vector<base_cell*>& sorted_cells) const
{
    cell_back_inserter handler(sorted_cells);
    depth_first_search dfs(m_map, *mp_names, handler);
    dfs.run();
}

void depends_tracker::print_dot_graph(const string& dotpath) const
{
    ofstream file(dotpath.c_str());
    depend_map_type::const_iterator itr = m_map.begin(), itr_end = m_map.end();
    file << "digraph G {" << endl;
    for (; itr != itr_end; ++itr)
    {
        const formula_cell* origin_cell = itr->first;
        string origin = global::get_cell_name(static_cast<const base_cell*>(origin_cell));
        print_dot_graph_depend(file, origin, *itr->second);
    }
    file << "}" << endl;
}

void depends_tracker::print_dot_graph_depend(
    ofstream& file, const string& origin, const depend_cells_type& cells) const
{
    depend_cells_type::const_iterator itr = cells.begin(), itr_end = cells.end();
    for (; itr != itr_end; ++itr)
        file << "    " << origin << " -> " << global::get_cell_name(*itr) << ";" << endl;
}

}
