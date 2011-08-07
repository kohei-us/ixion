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

#ifndef __IXION_CELL_RANGE_ITERATOR_HPP__
#define __IXION_CELL_RANGE_ITERATOR_HPP__

namespace ixion {

class model_context;
class abs_range_t;
class base_cell;

class cells_in_range
{
    friend class model_context;

    const model_context& m_context;
    const abs_range_t& m_range;

    cells_in_range(); // disabled
    cells_in_range(const model_context& cxt, const abs_range_t& range);
public:
    class const_iterator
    {
    public:
        const_iterator();
        bool operator== (const const_iterator& r) const;
        bool operator!= (const const_iterator& r) const;
        const_iterator& operator= (const const_iterator& r);
        const base_cell* operator++();
        const base_cell* operator--();
        const base_cell& operator*() const;
        const base_cell* operator->() const;
    };

    cells_in_range(const cells_in_range& r);
    const_iterator begin();
    const_iterator end();
};

}

#endif
