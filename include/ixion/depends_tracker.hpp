/*************************************************************************
 *
 * Copyright (c) 2010, 2011 Kohei Yoshida
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

#ifndef __DEPENDS_TRACKER_HPP__
#define __DEPENDS_TRACKER_HPP__

#include "ixion/formula_parser.hpp"
#include "ixion/depth_first_search.hpp"

#include <set>
#include <string>
#include <vector>
#include <boost/ptr_container/ptr_map.hpp>

namespace ixion {

class base_cell;

namespace interface {

class model_context;

}

/** 
 * This class keeps track of inter-cell dependencies.  Each formula cell 
 * item stores pointers to other cells that it depends on (precedent cells).
 * This information is used to build a complete dependency tree. 
 */
class dependency_tracker
{
    class cell_back_inserter : public ::std::unary_function<base_cell*, void>
    {
    public:
        cell_back_inserter(::std::vector<base_cell*>& sorted_cells);
        void operator() (base_cell* cell);
    private:
        ::std::vector<base_cell*>& m_sorted_cells;
    };

    typedef depth_first_search<base_cell*, cell_back_inserter> dfs_type;

public:
    dependency_tracker(const dirty_cells_t& dirty_cells, const interface::model_context& cxt);
    ~dependency_tracker();

    /** 
     * Insert a single dependency relationship.
     *
     * @param origin_cell* cell that depends on <code>depend_cell</code>.
     * @param depend_cell* cell that <code>origin_cell</code> depends on.
     */
    void insert_depend(base_cell* origin_cell, base_cell* depend_cell);

    void interpret_all_cells(size_t thread_count);

    /** 
     * Perform topological sort on all cell instances, and returns an array of 
     * cells that are sorted in order of dependency. 
     */
    void topo_sort_cells(::std::vector<base_cell*>& sorted_cells) const;

private:
    dfs_type::precedent_set m_deps;
    const dirty_cells_t& m_dirty_cells;
    const interface::model_context& m_context;
};

}

#endif
