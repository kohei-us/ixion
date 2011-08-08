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

cells_in_range::const_iterator cells_in_range::begin()
{
    return const_iterator(this, false);
}

cells_in_range::const_iterator cells_in_range::end()
{
    return const_iterator(this, true);
}

cells_in_range::const_iterator::const_iterator() :
    mp_parent(NULL) {}

cells_in_range::const_iterator::const_iterator(cells_in_range* parent, bool end_pos) :
    mp_parent(parent)
{
    const model_context::cell_store_type& cells = mp_parent->m_context.m_cells;
    m_beg = cells.begin();
    m_end = cells.end();
    if (end_pos)
    {
        m_cur = m_end;
        return;
    }

    m_cur = m_beg;
    if (!mp_parent->m_range.contains(m_cur->first))
        find_next();
}

bool cells_in_range::const_iterator::operator ==(const const_iterator& r) const
{
    return mp_parent == r.mp_parent && m_cur == r.m_cur && m_end == r.m_end;
}

bool cells_in_range::const_iterator::operator !=(const const_iterator& r) const
{
    return !operator==(r);
}

cells_in_range::const_iterator& cells_in_range::const_iterator::operator =(const const_iterator& r)
{
    mp_parent = r.mp_parent;
    m_cur = r.m_cur;
    m_beg = r.m_beg;
    m_end = r.m_end;
    return *this;
}

const base_cell* cells_in_range::const_iterator::operator++()
{
    find_next();
    return m_cur == m_end ? NULL : m_cur->second;
}

const base_cell* cells_in_range::const_iterator::operator--()
{
    find_prev();
    const abs_range_t& range = mp_parent->m_range;
    if (m_cur == m_beg)
        return range.contains(m_cur->first) ? m_cur->second : NULL;

    return m_cur->second;
}

const base_cell& cells_in_range::const_iterator::operator*() const
{
    return *m_cur->second;
}

const base_cell* cells_in_range::const_iterator::operator->() const
{
    return m_cur->second;
}

void cells_in_range::const_iterator::find_next()
{
    const abs_range_t& range = mp_parent->m_range;
    for (++m_cur; m_cur != m_end; ++m_cur)
    {
        const abs_address_t& addr = m_cur->first;
        if (range.contains(addr))
            break;
    }
}

void cells_in_range::const_iterator::find_prev()
{
    const abs_range_t& range = mp_parent->m_range;
    for (--m_cur; m_cur != m_beg; --m_cur)
    {
        const abs_address_t& addr = m_cur->first;
        if (range.contains(addr))
            break;
    }
}

}
