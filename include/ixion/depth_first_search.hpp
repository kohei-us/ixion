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
    typedef std::unordered_map<value_type, size_t, value_hash_type> value_index_map_type;

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

    /**
     * Stores all precedent-dependent relations which are to be used to
     * perform topological sort.
     */
    class relations
    {
        friend class depth_first_search;

    public:
        void insert(value_type pre, value_type dep)
        {
            typename precedent_map_type::iterator itr = m_map.find(pre);
            if (itr == m_map.end())
            {
                // First dependent for this precedent.
                std::pair<typename precedent_map_type::iterator, bool> r =
                    m_map.insert(
                        typename precedent_map_type::value_type(pre, precedent_cells_type()));

                if (!r.second)
                    throw dfs_error("failed to insert a new set instance");

                itr = r.first;
            }

            itr->second.insert(dep);
        }

    private:
        const precedent_map_type& get() const { return m_map; }

        precedent_map_type m_map;
    };

    depth_first_search(
        const std::vector<value_type>& values,
        const relations& rels, cell_handler_type& handler);

    void init();
    void run();

private:
    void visit(size_t cell_index);
    size_t get_cell_index(const value_type& p) const;
    const precedent_cells_type* get_precedent_cells(value_type cell);

private:
    const precedent_map_type& m_precedent_map;
    cell_handler_type& m_handler;
    size_t m_value_count;
    value_index_map_type m_value_indices;

    size_t m_time_stamp;
    std::vector<node_data> m_values;
};

template<typename _ValueType, typename _CellHandlerType, typename _ValueHashType>
depth_first_search<_ValueType,_CellHandlerType,_ValueHashType>::depth_first_search(
    const std::vector<value_type>& values,
    const relations& rels, cell_handler_type& handler) :
    m_precedent_map(rels.get()),
    m_handler(handler),
    m_value_count(values.size()),
    m_time_stamp(0),
    m_values(m_value_count)
{
    // Construct value node to index mapping.
    for (size_t i = 0; i < m_value_count; ++i)
        m_value_indices.insert(
            typename value_index_map_type::value_type(values[i], i));
}

template<typename _ValueType, typename _CellHandlerType, typename _ValueHashType>
void depth_first_search<_ValueType,_CellHandlerType,_ValueHashType>::init()
{
    std::vector<node_data> values(m_value_count);

    // Now, construct index to cell node mapping.
    for (const auto& vi : m_value_indices)
        values[vi.second].node = vi.first;

    m_values.swap(values);
    m_time_stamp = 0;
}

template<typename _ValueType, typename _CellHandlerType, typename _ValueHashType>
void depth_first_search<_ValueType,_CellHandlerType,_ValueHashType>::run()
{
    init();

    try
    {
        for (size_t i = 0; i < m_value_count; ++i)
            if (m_values[i].color == white)
                visit(i);
    }
    catch(const dfs_error& e)
    {
        std::cout << "dfs error: " << e.what() << std::endl;
    }
}

template<typename _ValueType, typename _CellHandlerType, typename _ValueHashType>
void depth_first_search<_ValueType,_CellHandlerType,_ValueHashType>::visit(size_t cell_index)
{
    value_type p = m_values[cell_index].node;
    m_values[cell_index].color = gray;
    m_values[cell_index].time_visited = ++m_time_stamp;

    do
    {
        const precedent_cells_type* depends = get_precedent_cells(p);
        if (!depends)
            // No dependent cells.
            break;

        for (const value_type& dcell : *depends)
        {
            size_t dcell_id = get_cell_index(dcell);
            if (m_values[dcell_id].color == white)
            {
                visit(dcell_id);
            }
        }
    }
    while (false);

    m_values[cell_index].color = black;
    m_values[cell_index].time_finished = ++m_time_stamp;
    m_handler(m_values[cell_index].node);
}

template<typename _ValueType, typename _CellHandlerType, typename _ValueHashType>
size_t depth_first_search<_ValueType,_CellHandlerType,_ValueHashType>::get_cell_index(const value_type& p) const
{
    typename value_index_map_type::const_iterator itr = m_value_indices.find(p);
    if (itr == m_value_indices.end())
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
        return nullptr;

    return &itr->second;
}

}

#endif
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
