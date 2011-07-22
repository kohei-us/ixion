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

#include "ixion/range_listener_tracker.hpp"

#define DEBUG_RANGE_LISTENER_TRACKER 1

#if DEBUG_RANGE_LISTENER_TRACKER
#include <iostream>
#endif

using namespace std;

namespace ixion {

range_listener_tracker::range_listener_tracker(model_context& cxt) :
    m_context(cxt) {}

range_listener_tracker::~range_listener_tracker()
{
    // Delete all the listener set instances.
    range_store_type::iterator itr = m_data.begin(), itr_end = m_data.end();
    for (; itr != itr_end; ++itr)
        delete itr->second;
}

void range_listener_tracker::add(const abs_address_t& cell, const abs_range_t& range)
{
#if DEBUG_RANGE_LISTENER_TRACKER
    cout << "range_listener_tracker: adding" << endl;
#endif
    range_store_type::iterator itr = m_data.find(range);
    if (itr == m_data.end())
    {
        // No container for this range yet.  Create one.
        pair<range_store_type::iterator, bool> r = 
            m_data.insert(range_store_type::value_type(range, new address_set_type));
        if (!r.second)
            throw general_error("failed to insert new address set to range listener tracker.");
        itr = r.first;

        // Insert the container to the rectangle set as well (for lookup).
#if DEBUG_RANGE_LISTENER_TRACKER
        cout << "x1=" << range.first.column << ",y1=" << range.first.row
            << ",x2=" << range.last.column << ",y2=" << range.last.row << ",p=" << itr->second << endl;
#endif
        m_query_set.insert(
            range.first.column, range.first.row, range.last.column+1, range.last.row+1, itr->second);
    }
    itr->second->insert(cell);
}

void range_listener_tracker::remove(const abs_address_t& cell, const abs_range_t& range)
{
#if DEBUG_RANGE_LISTENER_TRACKER
    cout << "range_listener_tracker: removing" << endl;
#endif
    range_store_type::iterator itr = m_data.find(range);
    if (itr == m_data.end())
        // No listeners for this range.  Bail out.
        return;

    address_set_type* p = itr->second;
    p->erase(cell);
    if (p->empty())
    {
        // This list is empty.  Remove it from the containers and destroy the instance.
        m_data.erase(itr);
        m_query_set.remove(p);
        delete p;
    }
}

void range_listener_tracker::get_all_listeners(
    const abs_address_t& target, dirty_cells_t& listeners) const
{
#if DEBUG_RANGE_LISTENER_TRACKER
    cout << "range_listener_tracker: get all listeners recursively" << endl;
#endif
}

}
