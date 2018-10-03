/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_TOPO_SORT_CALCULATOR_HPP
#define INCLUDED_IXION_TOPO_SORT_CALCULATOR_HPP

#include "formula_parser.hpp"
#include "ixion/depth_first_search.hpp"

#include <set>
#include <string>
#include <vector>

namespace ixion {

class formula_cell;

namespace iface {

class formula_model_access;

}

/**
 * This class determines the global dependency order via topological sorting
 * based on the reference relationships between formula cells, and perform
 * calculations of all specified formula cells in the correct linear order
 * with one or more threads.
 */
class topo_sort_calculator
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
    topo_sort_calculator(const abs_address_set_t& dirty_cells, iface::formula_model_access& cxt);
    ~topo_sort_calculator();

    /**
     * Define a reference relationship between two cells.
     * dependency relationship.
     *
     * @param src source cell that references the <code>dest</code> cell.
     * @param dest destination cell referenced by the source cell.
     */
    void set_reference_relation(const abs_address_t& src, const abs_address_t& dest);

    void interpret_all_cells(size_t thread_count);

    /**
     * Perform topological sort on all cell instances, and returns an array of
     * cells that are sorted in order of dependency.
     */
    std::vector<abs_address_t> sort_cells() const;

private:
    dfs_type::relations m_deps;
    const abs_address_set_t& m_dirty_cells;
    iface::formula_model_access& m_context;
};

}

#endif
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
