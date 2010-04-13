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

#include <vector>
#include <unordered_map>
#include <iostream>
#include <fstream>

using namespace std;

namespace ixion {

namespace {

class depth_first_search
{
    typedef unordered_map<const base_cell*, size_t> cell_index_map_type;

    enum vertex_color { white, gray, black };

    class dfs_error : public general_error
    {
    public:
        dfs_error(const string& msg) : general_error(msg) {}
    };
public:
    depth_first_search(const depends_tracker::depend_map_type& depend_map, const depends_tracker::ptr_name_map_type* cell_names) :
        m_depend_map(depend_map),
        m_cell_names(cell_names),
        m_cell_count(cell_names->size()),
        m_time_stamp(0),
        m_cell_colors(m_cell_count, white),
        m_parents(m_cell_count, NULL),
        m_visited(m_cell_count, 0),
        m_finished(m_cell_count, 0)

    {
        m_cells.reserve(m_cell_count);
        depends_tracker::ptr_name_map_type::const_iterator itr = cell_names->begin(), itr_end = cell_names->end();
        for (size_t index = 0; itr != itr_end; ++itr, ++index)
        {
            m_cells.push_back(itr->first);
            m_cell_indices.insert(
                cell_index_map_type::value_type(itr->first, index));
        }
    }

    void run()
    {
        cout << "cell count: " << m_cell_count << endl;
        try
        {
            for (size_t i = 0; i < m_cell_count; ++i)
                if (m_cell_colors[i] == white)
                    visit(i);
        }
        catch(const dfs_error& e)
        {
            cout << "dfs error: " << e.what() << endl;
        }
    }

    void print_result()
    {
        cout << "result -----------------------------------------------------" << endl;
        for (size_t i = 0; i < m_cell_count; ++i)
        {
            const base_cell* p = m_cells[i];
            cout << get_cell_name(p) << ": finished: " << m_finished[i] << endl;
        }
    }

private:
    void visit(size_t cell_index)
    {
        cout << "visit (start) ----------------------------------------------" << endl;
        const base_cell* p = m_cells[cell_index];
        if (p->get_celltype() != celltype_formula)
            return;
        const formula_cell* fcell = static_cast<const formula_cell*>(p);
        cout << "visit cell index: " << cell_index << "  name: " << get_cell_name(fcell) << endl;
        m_cell_colors[cell_index] = gray;
        m_visited[cell_index] = ++m_time_stamp;

        const depends_tracker::depend_cells_type* depends = get_depend_cells(fcell);
        if (!depends)
            return;

        cout << "depend cell count: " << depends->size() << endl;
        depends_tracker::depend_cells_type::const_iterator itr = depends->begin(), itr_end = depends->end();
        for (; itr != itr_end; ++itr)
        {
            const base_cell* dcell = *itr;
            cout << "depend cell: " << get_cell_name(dcell) << " (" << dcell << ")" << endl;
            size_t dcell_id = get_cell_index(dcell);
            if (m_cell_colors[dcell_id] == white)
            {
                m_parents[dcell_id] = p;
                visit(dcell_id);
            }
        }

        m_cell_colors[cell_index] = black;
        m_finished[cell_index] = ++m_time_stamp;
        cout << "visit (end) ------------------------------------------------" << endl;
    }

    string get_cell_name(const base_cell* p) const
    {
        depends_tracker::ptr_name_map_type::const_iterator itr = m_cell_names->find(p);
        if (itr == m_cell_names->end())
            throw dfs_error("failed to retrieve cell name from the ptr.");

        return itr->second;
    }
    size_t get_cell_index(const base_cell* p) const
    {
        unordered_map<const base_cell*, size_t>::const_iterator itr = m_cell_indices.find(p);
        if (itr == m_cell_indices.end())
            throw dfs_error("cell ptr to index mapping failed.");
        return itr->second;
    }

    const depends_tracker::depend_cells_type* get_depend_cells(const formula_cell* cell)
    {
        depends_tracker::depend_map_type::const_iterator itr = m_depend_map.find(cell);
        if (itr == m_depend_map.end())
            // This cell has no dependent cells.
            return NULL;

        return itr->second;
    }

private:
    const depends_tracker::depend_map_type&     m_depend_map;
    const depends_tracker::ptr_name_map_type*   m_cell_names;
    size_t                                      m_cell_count;

    size_t                      m_time_stamp;
    vector<vertex_color>        m_cell_colors;
    vector<const base_cell*>    m_parents;
    vector<size_t>              m_visited;
    vector<size_t>              m_finished;

    cell_index_map_type         m_cell_indices;
    vector<const base_cell*>    m_cells;
};


}

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
