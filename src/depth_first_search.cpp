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

#include "depth_first_search.hpp"
#include "cell.hpp"

#include <iostream>

using namespace std;

namespace ixion {

depth_first_search::depth_first_search(
    const depends_tracker::depend_map_type& depend_map, 
    const depends_tracker::ptr_name_map_type* cell_names) :
    m_depend_map(depend_map),
    m_cell_names(cell_names),
    m_cell_count(cell_names->size()),
    m_time_stamp(0),
    m_cells(m_cell_count)
{
    depends_tracker::ptr_name_map_type::const_iterator itr = m_cell_names->begin(), itr_end = m_cell_names->end();
    for (size_t index = 0; itr != itr_end; ++itr, ++index)
        m_cell_indices.insert(
            cell_index_map_type::value_type(itr->first, index));
}

void depth_first_search::init()
{
    vector<celldata> cells(m_cell_count);
    depends_tracker::ptr_name_map_type::const_iterator itr = m_cell_names->begin(), itr_end = m_cell_names->end();
    for (size_t index = 0; itr != itr_end; ++itr, ++index)
        cells[index].ptr = itr->first;
    m_cells.swap(cells);
    m_time_stamp = 0;
}

void depth_first_search::run()
{
    init();
    cout << "cell count: " << m_cell_count << endl;
    try
    {
        for (size_t i = 0; i < m_cell_count; ++i)
            if (m_cells[i].color == white)
                visit(i);
    }
    catch(const dfs_error& e)
    {
        cout << "dfs error: " << e.what() << endl;
    }
}

void depth_first_search::print_result()
{
    cout << "result -----------------------------------------------------" << endl;
    for (size_t i = 0; i < m_cell_count; ++i)
    {
        const base_cell* p = m_cells[i].ptr;
        cout << get_cell_name(p) << ": finished: " << m_cells[i].time_finished << endl;
    }
}

void depth_first_search::visit(size_t cell_index)
{
    cout << "visit (start) ----------------------------------------------" << endl;
    const base_cell* p = m_cells[cell_index].ptr;
    if (p->get_celltype() != celltype_formula)
        return;
    const formula_cell* fcell = static_cast<const formula_cell*>(p);
    cout << "visit cell index: " << cell_index << "  name: " << get_cell_name(fcell) << endl;
    m_cells[cell_index].color = gray;
    m_cells[cell_index].time_visited = ++m_time_stamp;

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
        if (m_cells[dcell_id].color == white)
        {
            m_cells[dcell_id].parent = p;
            visit(dcell_id);
        }
    }

    m_cells[cell_index].color = black;
    m_cells[cell_index].time_finished = ++m_time_stamp;
    cout << "visit (end) ------------------------------------------------" << endl;
}

string depth_first_search::get_cell_name(const base_cell* p) const
{
    depends_tracker::ptr_name_map_type::const_iterator itr = m_cell_names->find(p);
    if (itr == m_cell_names->end())
        throw dfs_error("failed to retrieve cell name from the ptr.");

    return itr->second;
}

size_t depth_first_search::get_cell_index(const base_cell* p) const
{
    unordered_map<const base_cell*, size_t>::const_iterator itr = m_cell_indices.find(p);
    if (itr == m_cell_indices.end())
        throw dfs_error("cell ptr to index mapping failed.");
    return itr->second;
}

const depends_tracker::depend_cells_type* depth_first_search::get_depend_cells(
    const formula_cell* cell)
{
    depends_tracker::depend_map_type::const_iterator itr = m_depend_map.find(cell);
    if (itr == m_depend_map.end())
        // This cell has no dependent cells.
        return NULL;

    return itr->second;
}

}
