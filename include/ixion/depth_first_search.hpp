/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_DEPTH_FIRST_SEARCH_HPP
#define INCLUDED_IXION_DEPTH_FIRST_SEARCH_HPP

#include "ixion/exceptions.hpp"

#include <vector>
#include <set>
#include <map>
#include <exception>
#include <iostream>
#include <unordered_map>

namespace ixion {

template<typename _ValueType, typename _CellHandlerType, typename _ValueHashType>
class depth_first_search
{
public:
    typedef _ValueType          value_type;
    typedef _CellHandlerType    cell_handler_type;
    typedef _ValueHashType      value_hash_type;

private:
    typedef std::unordered_map<value_type, size_t, value_hash_type> cell_index_map_type;

    enum cell_color_type { white, gray, black };

    class dfs_error : public general_error
    {
    public:
        dfs_error(const std::string& msg) : general_error(msg) {}
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
    typedef std::set<value_type> precedent_cells_type;
    typedef std::map<value_type, precedent_cells_type> precedent_map_type;

    class precedent_set
    {
    public:
        void insert(value_type cell, value_type dep)
        {
            typename precedent_map_type::iterator itr = m_map.find(cell);
            if (itr == m_map.end())
            {
                // First dependent for this cell.
                std::pair<typename precedent_map_type::iterator, bool> r =
                    m_map.insert(
                        typename precedent_map_type::value_type(cell, precedent_cells_type()));

                if (!r.second)
                    throw dfs_error("failed to insert a new set instance");

                itr = r.first;
            }

            itr->second.insert(dep);
        }

        const precedent_map_type& get() const { return m_map; }

    private:
        precedent_map_type m_map;
    };

    depth_first_search(
        const ::std::vector<value_type>& cells,
        const precedent_map_type& precedent_map, cell_handler_type& handler);

    void init();
    void run();

private:
    void visit(size_t cell_index);
    size_t get_cell_index(value_type p) const;
    const precedent_cells_type* get_precedent_cells(value_type cell);

private:
    const precedent_map_type&  m_precedent_map;
    cell_handler_type&      m_handler;
    size_t                  m_cell_count;
    cell_index_map_type     m_cell_indices;

    size_t                  m_time_stamp;
    ::std::vector<node_data> m_cells;
};

template<typename _ValueType, typename _CellHandlerType, typename _ValueHashType>
depth_first_search<_ValueType,_CellHandlerType,_ValueHashType>::depth_first_search(
    const ::std::vector<value_type>& cells,
    const precedent_map_type& precedent_map, cell_handler_type& handler) :
    m_precedent_map(precedent_map),
    m_handler(handler),
    m_cell_count(cells.size()),
    m_time_stamp(0),
    m_cells(m_cell_count)
{
    typename ::std::vector<value_type>::const_iterator
        itr = cells.begin(), itr_end = cells.end();

    // Construct cell node to index mapping.
    for (size_t index = 0; itr != itr_end; ++itr, ++index)
        m_cell_indices.insert(
            typename cell_index_map_type::value_type(*itr, index));
}

template<typename _ValueType, typename _CellHandlerType, typename _ValueHashType>
void depth_first_search<_ValueType,_CellHandlerType,_ValueHashType>::init()
{
    ::std::vector<node_data> cells(m_cell_count);
    typename cell_index_map_type::const_iterator
        itr = m_cell_indices.begin(), itr_end = m_cell_indices.end();

    // Now, construct index to cell node mapping.
    for (; itr != itr_end; ++itr)
        cells[itr->second].node = itr->first;
    m_cells.swap(cells);
    m_time_stamp = 0;
}

template<typename _ValueType, typename _CellHandlerType, typename _ValueHashType>
void depth_first_search<_ValueType,_CellHandlerType,_ValueHashType>::run()
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

template<typename _ValueType, typename _CellHandlerType, typename _ValueHashType>
void depth_first_search<_ValueType,_CellHandlerType,_ValueHashType>::visit(size_t cell_index)
{
    value_type p = m_cells[cell_index].node;
    m_cells[cell_index].color = gray;
    m_cells[cell_index].time_visited = ++m_time_stamp;

    do
    {
        const precedent_cells_type* depends = get_precedent_cells(p);
        if (!depends)
            // No dependent cells.
            break;

        typename precedent_cells_type::const_iterator itr = depends->begin(), itr_end = depends->end();
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

template<typename _ValueType, typename _CellHandlerType, typename _ValueHashType>
size_t depth_first_search<_ValueType,_CellHandlerType,_ValueHashType>::get_cell_index(value_type p) const
{
    typename cell_index_map_type::const_iterator itr = m_cell_indices.find(p);
    if (itr == m_cell_indices.end())
        throw dfs_error("cell ptr to index mapping failed.");
    return itr->second;
}

template<typename _ValueType, typename _CellHandlerType, typename _ValueHashType>
const typename depth_first_search<_ValueType,_CellHandlerType,_ValueHashType>::precedent_cells_type*
depth_first_search<_ValueType,_CellHandlerType,_ValueHashType>::get_precedent_cells(value_type cell)
{
    typename precedent_map_type::const_iterator itr = m_precedent_map.find(cell);
    if (itr == m_precedent_map.end())
        // This cell has no dependent cells.
        return NULL;

    return &itr->second;
}

}

#endif
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
