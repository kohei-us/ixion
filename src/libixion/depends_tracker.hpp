/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __DEPENDS_TRACKER_HPP__
#define __DEPENDS_TRACKER_HPP__

#include "ixion/formula_parser.hpp"
#include "ixion/depth_first_search.hpp"

#include <set>
#include <string>
#include <vector>
#include <boost/ptr_container/ptr_map.hpp>

namespace ixion {

class formula_cell;

namespace iface {

class formula_model_access;

}

/**
 * This class keeps track of inter-cell dependencies.  Each formula cell
 * item stores pointers to other cells that it depends on (precedent cells).
 * This information is used to build a complete dependency tree.
 */
class dependency_tracker
{
    class cell_back_inserter : public std::unary_function<abs_address_t, void>
    {
    public:
        cell_back_inserter(std::vector<abs_address_t>& sorted_cells);
        void operator() (const abs_address_t& cell);
    private:
        ::std::vector<abs_address_t>& m_sorted_cells;
    };

    typedef depth_first_search<abs_address_t, cell_back_inserter, abs_address_t::hash> dfs_type;

public:
    dependency_tracker(const dirty_formula_cells_t& dirty_cells, iface::formula_model_access& cxt);
    ~dependency_tracker();

    /**
     * Insert a single dependency relationship.
     *
     * @param origin_cell* cell that depends on <code>depend_cell</code>.
     * @param depend_cell* cell that <code>origin_cell</code> depends on.
     */
    void insert_depend(const abs_address_t& origin_cell, const abs_address_t& depend_cell);

    void interpret_all_cells(size_t thread_count);

    /**
     * Perform topological sort on all cell instances, and returns an array of
     * cells that are sorted in order of dependency.
     */
    void topo_sort_cells(std::vector<abs_address_t>& sorted_cells) const;

private:
    dfs_type::precedent_set m_deps;
    const dirty_formula_cells_t& m_dirty_cells;
    iface::formula_model_access& m_context;
};

}

#endif
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
