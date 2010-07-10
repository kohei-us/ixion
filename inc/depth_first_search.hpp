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

#include <boost/ptr_container/ptr_map.hpp>

namespace ixion {

class base_cell;

class depth_first_search
{
    typedef ::std::unordered_map<base_cell*, size_t> cell_index_map_type;

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

    struct celldata
    {
        cell_color_type color;
        base_cell*      ptr;
        base_cell*      parent;
        size_t          time_visited;
        size_t          time_finished;

        celldata() : color(white), ptr(NULL), parent(NULL), time_visited(0), time_finished(0) {}
    };

public:
    typedef ::std::set<base_cell*>                             depend_cells_type;
    typedef ::boost::ptr_map<base_cell*, depend_cells_type>    depend_map_type;

    class cell_handler : public ::std::unary_function<base_cell*, void>
    {
    public:
        virtual void operator() (base_cell* cell) = 0;
    };

    depth_first_search(
        const ::std::vector<base_cell*>& cells, 
        const depend_map_type& depend_map, cell_handler& handler);

    void init();
    void run();

private:
    void visit(size_t cell_index);
    size_t get_cell_index(base_cell* p) const;
    const depend_cells_type* get_depend_cells(base_cell* cell);

private:
    const depend_map_type&  m_depend_map;
    cell_handler&           m_handler;
    size_t                  m_cell_count;
    cell_index_map_type     m_cell_indices;

    size_t                  m_time_stamp;
    ::std::vector<celldata> m_cells;
};

}

#endif
