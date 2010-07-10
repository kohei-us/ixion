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

#include "global.hpp"
#include "depends_tracker.hpp"

#include <unordered_map>
#include <vector>

namespace ixion {

class base_cell;

class depth_first_search
{
    typedef ::std::unordered_map<const base_cell*, size_t> cell_index_map_type;

    enum cell_color_type { white, gray, black };

    class dfs_error : public general_error
    {
    public:
        dfs_error(const ::std::string& msg) : general_error(msg) {}
    };

    struct celldata
    {
        cell_color_type     color;
        const base_cell*    ptr;
        const base_cell*    parent;
        size_t              time_visited;
        size_t              time_finished;

        celldata() : color(white), ptr(NULL), parent(NULL), time_visited(0), time_finished(0) {}
    };

public:
    class cell_handler : public ::std::unary_function<base_cell*, void>
    {
    public:
        virtual void operator() (base_cell* cell) = 0;
    };

    depth_first_search(const depends_tracker::depend_map_type& depend_map, 
                       const cell_ptr_name_map_t& cell_names, cell_handler& handler);
    void init();
    void run();

private:
    void visit(size_t cell_index);
    size_t get_cell_index(const base_cell* p) const;
    const depends_tracker::depend_cells_type* get_depend_cells(const formula_cell* cell);

private:
    const depends_tracker::depend_map_type&     m_depend_map;
    const cell_ptr_name_map_t&                  m_cell_names;
    cell_handler&                               m_handler;
    size_t                                      m_cell_count;
    cell_index_map_type                         m_cell_indices;

    size_t                          m_time_stamp;
    ::std::vector<celldata>         m_cells;
};

}

#endif
