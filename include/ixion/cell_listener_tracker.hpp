/*************************************************************************
 *
 * Copyright (c) 2011 Kohei Yoshida
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

#ifndef __IXION_RANGE_LISTENER_TRACKER_HPP__
#define __IXION_RANGE_LISTENER_TRACKER_HPP__

#include "ixion/global.hpp"
#include "ixion/address.hpp"
#include "ixion/hash_container/map.hpp"
#include "ixion/hash_container/set.hpp"

#include <mdds/rectangle_set.hpp>

namespace ixion {

namespace iface {
    class model_context;
}

/**
 * Track all single and range references being listened to by individual
 * cells.
 */
class cell_listener_tracker
{
    cell_listener_tracker(); // disabled
public:
    cell_listener_tracker(iface::model_context& cxt);

    typedef _ixion_unordered_set_type<abs_address_t, abs_address_t::hash> address_set_type;
    typedef ::mdds::rectangle_set<row_t, address_set_type> range_query_set_type;
    typedef _ixion_unordered_map_type<abs_address_t, address_set_type*, abs_address_t::hash> cell_store_type;
    typedef _ixion_unordered_map_type<abs_range_t, address_set_type*, abs_range_t::hash> range_store_type;

    ~cell_listener_tracker();

    /**
     * Add a reference relationship between two single cells.
     *
     * @param src cell that references (therefore listens to) dest cell.
     * @param dest cell being referenced (therefore being listened to) by dest
     *             cell.
     */
    void add(const abs_address_t& src, const abs_address_t& dest);

    /**
     * Add a reference relationship from a single cell to a range.  The cell
     * references the range.  Duplicates are silently ignored.
     *
     * @param cell cell that includes reference to the range.
     * @param range range referenced by the cell.
     */
    void add(const abs_address_t& cell, const abs_range_t& range);

    void remove(const abs_address_t& src, const abs_address_t& dest);

    /**
     * Remove an existing reference relationship from a single cell to a
     * range.  If no such relationship exists, it does nothing.
     *
     * @param cell cell that includes reference to the range.
     * @param range range referenced by the cell.
     */
    void remove(const abs_address_t& cell, const abs_range_t& range);

    void add_volatile(const abs_address_t& pos);
    void remove_volatile(const abs_address_t& pos);

    void get_all_cell_listeners(const abs_address_t& target, dirty_cells_t& listeners) const;

    /**
     * Given a modified cell (target), get all formula cells that need to be
     * re-calculated based on the range-to-cells listener relationships.
     *
     * @param target
     * @param listeners
     */
    void get_all_range_listeners(const abs_address_t& target, dirty_cells_t& listeners) const;

    void print_cell_listeners(const abs_address_t& target) const;
private:
    void get_all_range_listeners_re(
        const abs_address_t& origin_target, const abs_address_t& target,
        dirty_cells_t& listeners, address_set_type& listeners_addr) const;

    iface::model_context& m_context;
    mutable range_query_set_type m_query_set; ///< used for fast lookup of range listeners.
    cell_store_type m_cell_listeners;         ///< store listeners for single cells.
    range_store_type m_range_listeners;       ///< store listeners for ranges.
    address_set_type m_volatile_cells;
};

}

#endif
