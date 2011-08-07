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
    return const_iterator();
}

cells_in_range::const_iterator cells_in_range::end()
{
    return const_iterator();
}

cells_in_range::const_iterator::const_iterator()
{
}

bool cells_in_range::const_iterator::operator ==(const const_iterator& r) const
{
    return true;
}

bool cells_in_range::const_iterator::operator !=(const const_iterator& r) const
{
    return !operator==(r);
}

cells_in_range::const_iterator& cells_in_range::const_iterator::operator =(const const_iterator& r)
{
    return *this;
}

const base_cell* cells_in_range::const_iterator::operator++()
{
    return NULL;
}

const base_cell* cells_in_range::const_iterator::operator--()
{
    return NULL;
}

const base_cell& cells_in_range::const_iterator::operator*() const
{
//  return base_cell();
}

const base_cell* cells_in_range::const_iterator::operator->() const
{
    return NULL;
}

}
