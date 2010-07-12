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

#ifndef __IXION_DEPTH_FIRST_SEARCH_HPP__
#define __IXION_DEPTH_FIRST_SEARCH_HPP__

#include <unordered_map>
#include <vector>
#include <set>
#include <exception>
#include <iostream>

#include <boost/ptr_container/ptr_map.hpp>

namespace ixion {

template<typename _ValueType, typename _CellHandlerType>
class depth_first_search
{
public:
    typedef _ValueType          value_type;
    typedef _CellHandlerType    cell_handler_type;

private:
    typedef ::std::unordered_map<value_type, size_t> cell_index_map_type;

    enum cell_color_type { white, gray, black };

    class dfs_error : public ::std::exception
    {
    public:
        dfs_error(const ::std::string& msg) : m_msg(msg) {}
        virtual ~dfs_error() throw() {}

        virtual const char* what() const throw()
        {
            return m_msg.c_str();
        }
    private:
        ::std::string m_msg;
    };

    struct node_data
    {
        cell_color_type color;
        value_type      node;
        size_t          time_visited;
        size_t          time_finished;

        node_data() : color(white), time_visited(0), time_finished(0) {}
    };

public:
    typedef ::std::set<value_type>                             depend_cells_type;
    typedef ::boost::ptr_map<value_type, depend_cells_type>    depend_map_type;

    class depend_set
    {
    public:
        void insert(value_type cell, value_type dep)
        {
            typename depend_map_type::iterator itr = m_map.find(cell);
            if (itr == m_map.end())
            {
                // First dependent for this cell.
                ::std::pair<typename depend_map_type::iterator, bool> r = m_map.insert(cell, new depend_cells_type);
                if (!r.second)
                    throw dfs_error("failed to insert a new set instance");

                itr = r.first;
            }

            itr->second->insert(dep);
        }

        const depend_map_type& get() const { return m_map; }

    private:
        depend_map_type m_map;
    };

    depth_first_search(
        const ::std::vector<value_type>& cells, 
        const depend_map_type& depend_map, cell_handler_type& handler);

    void init();
    void run();

private:
    void visit(size_t cell_index);
    size_t get_cell_index(value_type p) const;
    const depend_cells_type* get_depend_cells(value_type cell);

private:
    const depend_map_type&  m_depend_map;
    cell_handler_type&      m_handler;
    size_t                  m_cell_count;
    cell_index_map_type     m_cell_indices;

    size_t                  m_time_stamp;
    ::std::vector<node_data> m_cells;
};

template<typename _ValueType, typename _CellHandlerType>
depth_first_search<_ValueType,_CellHandlerType>::depth_first_search(
    const ::std::vector<value_type>& cells,
    const depend_map_type& depend_map, cell_handler_type& handler) :
    m_depend_map(depend_map),
    m_handler(handler),
    m_cell_count(cells.size()),
    m_time_stamp(0),
    m_cells(m_cell_count)
{
    typename ::std::vector<value_type>::const_iterator 
        itr = cells.begin(), itr_end = cells.end();

    // Construct cell pointer to index mapping.
    for (size_t index = 0; itr != itr_end; ++itr, ++index)
        m_cell_indices.insert(
            typename cell_index_map_type::value_type(*itr, index));
}

template<typename _ValueType, typename _CellHandlerType>
void depth_first_search<_ValueType,_CellHandlerType>::init()
{
    ::std::vector<node_data> cells(m_cell_count);
    typename cell_index_map_type::const_iterator 
        itr = m_cell_indices.begin(), itr_end = m_cell_indices.end();

    for (size_t index = 0; itr != itr_end; ++itr, ++index)
        cells[index].node = itr->first;
    m_cells.swap(cells);
    m_time_stamp = 0;
}

template<typename _ValueType, typename _CellHandlerType>
void depth_first_search<_ValueType,_CellHandlerType>::run()
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
        using namespace std;
        cout << "dfs error: " << e.what() << endl;
    }
}

template<typename _ValueType, typename _CellHandlerType>
void depth_first_search<_ValueType,_CellHandlerType>::visit(size_t cell_index)
{
    value_type p = m_cells[cell_index].node;
    m_cells[cell_index].color = gray;
    m_cells[cell_index].time_visited = ++m_time_stamp;

    do
    {
        const depend_cells_type* depends = get_depend_cells(p);
        if (!depends)
            // No dependent cells.
            break;
    
        typename depend_cells_type::const_iterator itr = depends->begin(), itr_end = depends->end();
        for (; itr != itr_end; ++itr)
        {
            value_type dcell = *itr;
            size_t dcell_id = get_cell_index(dcell);
            if (m_cells[dcell_id].color == white)
            {
                visit(dcell_id);
            }
        }
    }
    while (false);

    m_cells[cell_index].color = black;
    m_cells[cell_index].time_finished = ++m_time_stamp;
    m_handler(m_cells[cell_index].node);
}

template<typename _ValueType, typename _CellHandlerType>
size_t depth_first_search<_ValueType,_CellHandlerType>::get_cell_index(value_type p) const
{
    typename ::std::unordered_map<value_type, size_t>::const_iterator itr = m_cell_indices.find(p);
    if (itr == m_cell_indices.end())
        throw dfs_error("cell ptr to index mapping failed.");
    return itr->second;
}

template<typename _ValueType, typename _CellHandlerType>
const typename depth_first_search<_ValueType,_CellHandlerType>::depend_cells_type*
depth_first_search<_ValueType,_CellHandlerType>::get_depend_cells(value_type cell)
{
    typename depend_map_type::const_iterator itr = m_depend_map.find(cell);
    if (itr == m_depend_map.end())
        // This cell has no dependent cells.
        return NULL;

    return itr->second;
}

}

#endif
