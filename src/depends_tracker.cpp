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

#include <vector>
#include <unordered_map>
#include <iostream>
#include <fstream>

using namespace std;

namespace ixion {

depends_tracker::depends_tracker(const ptr_name_map_type* names) :
    mp_names(names)
{
}

depends_tracker::~depends_tracker()
{
}

void depends_tracker::insert_depend(const formula_cell* origin_cell, const base_cell* depend_cell)
{
    cout << "origin cell: " << origin_cell << "  depend cell: " << depend_cell << endl;
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
    cout << "map count: " << m_map.size() << "  depend count: " << itr->second->size() << endl;
}

void depends_tracker::topo_sort_cells()
{
    cout << "depth first search ---------------------------------------------------" << endl;
    depth_first_search dfs(m_map, mp_names);
    dfs.run();
    dfs.print_result();
}

void depends_tracker::print_dot_graph(const string& dotpath) const
{
    ofstream file(dotpath.c_str());
    depend_map_type::const_iterator itr = m_map.begin(), itr_end = m_map.end();
    file << "digraph G {" << endl;
    for (; itr != itr_end; ++itr)
    {
        const formula_cell* origin_cell = itr->first;
        string origin = get_cell_name(static_cast<const base_cell*>(origin_cell));
        print_dot_graph_depend(file, origin, *itr->second);
    }
    file << "}" << endl;
}

void depends_tracker::print_dot_graph_depend(
    ofstream& file, const string& origin, const depend_cells_type& cells) const
{
    depend_cells_type::const_iterator itr = cells.begin(), itr_end = cells.end();
    for (; itr != itr_end; ++itr)
        file << "    " << origin << " -> " << get_cell_name(*itr) << ";" << endl;
}

string depends_tracker::get_cell_name(const base_cell* pcell) const
{
    ptr_name_map_type::const_iterator itr_name = mp_names->find(pcell);
    if (itr_name == mp_names->end())
        throw general_error("name not found for the given cell pointer");

    return itr_name->second;
}

}
