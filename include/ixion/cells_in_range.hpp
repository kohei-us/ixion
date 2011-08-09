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

#include "ixion/model_context.hpp"

namespace ixion {

class model_context;
class abs_range_t;
class base_cell;

/**
 * Provides iterator for iterating through cell instances in a given range.
 * Empty cells are skipped.
 */
class cells_in_range
{
    cells_in_range(); // disabled
public:
    cells_in_range(const model_context& cxt, const abs_range_t& range);
    cells_in_range(const cells_in_range& r);

    const base_cell* first();
    const base_cell* next();

private:
    void find_next();

    const model_context& m_context;
    const abs_range_t& m_range;
    model_context::cell_store_type::const_iterator m_cur;
    model_context::cell_store_type::const_iterator m_end;
};

}

#endif
