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
    const vector<value_type>& cells,
    const depend_map_type& depend_map, cell_handler& handler) :
    m_depend_map(depend_map),
    m_handler(handler),
    m_cell_count(cells.size()),
    m_time_stamp(0),
    m_cells(m_cell_count)
{
    vector<value_type>::const_iterator 
        itr = cells.begin(), itr_end = cells.end();

    // Construct cell pointer to index mapping.
    for (size_t index = 0; itr != itr_end; ++itr, ++index)
        m_cell_indices.insert(
            cell_index_map_type::value_type(*itr, index));
}

void depth_first_search::init()
{
    vector<node_data> cells(m_cell_count);
    cell_index_map_type::const_iterator 
        itr = m_cell_indices.begin(), itr_end = m_cell_indices.end();

    for (size_t index = 0; itr != itr_end; ++itr, ++index)
        cells[index].node = itr->first;
    m_cells.swap(cells);
    m_time_stamp = 0;
}

void depth_first_search::run()
{
    init();
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

void depth_first_search::visit(size_t cell_index)
{
    value_type p = m_cells[cell_index].node;
    m_cells[cell_index].color = gray;
    m_cells[cell_index].time_visited = ++m_time_stamp;

    do
    {
        if (p->get_celltype() != celltype_formula)
            // No formula cell, no dependent cells.
            break;

        const depend_cells_type* depends = get_depend_cells(p);
        if (!depends)
            // No dependent cells.
            break;
    
        depend_cells_type::const_iterator itr = depends->begin(), itr_end = depends->end();
        for (; itr != itr_end; ++itr)
        {
            value_type dcell = *itr;
            size_t dcell_id = get_cell_index(dcell);
            if (m_cells[dcell_id].color == white)
            {
                m_cells[dcell_id].parent = p;
                visit(dcell_id);
            }
        }
    }
    while (false);

    m_cells[cell_index].color = black;
    m_cells[cell_index].time_finished = ++m_time_stamp;
    m_handler(m_cells[cell_index].node);
}

size_t depth_first_search::get_cell_index(value_type p) const
{
    unordered_map<value_type, size_t>::const_iterator itr = m_cell_indices.find(p);
    if (itr == m_cell_indices.end())
        throw dfs_error("cell ptr to index mapping failed.");
    return itr->second;
}

const depth_first_search::depend_cells_type* depth_first_search::get_depend_cells(
    value_type cell)
{
    depend_map_type::const_iterator itr = m_depend_map.find(cell);
    if (itr == m_depend_map.end())
        // This cell has no dependent cells.
        return NULL;

    return itr->second;
}

}
