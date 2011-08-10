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

#include "ixion/cells_in_range.hpp"
#include "ixion/cell.hpp"

namespace ixion {

cells_in_range::cells_in_range(const model_context& cxt, const abs_range_t& range) :
    m_context(cxt), m_range(range) {}

cells_in_range::cells_in_range(const cells_in_range& r) :
    m_context(r.m_context), m_range(r.m_range) {}

cells_in_range::~cells_in_range() {}

const base_cell* cells_in_range::first()
{
    const model_context::cell_store_type& cells = m_context.m_cells;
    m_end = cells.end();
    m_cur = cells.begin();
    if (!m_range.contains(m_cur->first))
        find_next();

    return m_cur == m_end ? NULL : m_cur->second;
}

const base_cell* cells_in_range::next()
{
    find_next();
    return m_cur == m_end ? NULL : m_cur->second;
}

void cells_in_range::find_next()
{
    for (++m_cur; m_cur != m_end; ++m_cur)
    {
        const abs_address_t& addr = m_cur->first;
        if (m_range.contains(addr))
            break;
    }
}

}
