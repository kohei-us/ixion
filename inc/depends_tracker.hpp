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

#ifndef __DEPENDS_TRACKER_HPP__
#define __DEPENDS_TRACKER_HPP__

#include <string>
#include <set>
#include <boost/ptr_container/ptr_map.hpp>

namespace ixion {

class base_cell;
class formula_cell;

/** 
 * This class keeps track of inter-cell dependencies.
 */
class depends_tracker
{
    typedef ::std::set<base_cell*> depend_cells_type;
    typedef ::boost::ptr_map<formula_cell*, depend_cells_type> depend_map_type;

public:
    typedef ::std::map<const base_cell*, ::std::string> ptr_name_map_t;

    depends_tracker(const ptr_name_map_t* names);
    ~depends_tracker();

    /** 
     * Insert a single dependency relationship.
     *
     * @param origin_cell* cell that depends on <code>depend_cell</code>.
     * @param depend_cell* cell that <code>origin_cell</code> depends on.
     */
    void insert_depend(formula_cell* origin_cell, base_cell* depend_cell);

    /** 
     * Create a file and write cell dependency graph in dot script.
     *
     * @param dotpath output file path.
     */
    void print_dot_graph(const ::std::string& dotpath) const;

private:
    void print_dot_graph_depend(::std::ofstream& file, const ::std::string& origin, const depend_cells_type& cells) const;

    ::std::string get_cell_name(const base_cell* pcell) const;

private:
    depend_map_type         m_map;
    const ptr_name_map_t*   mp_names;
};

}

#endif
