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

#ifndef __IXION_INTERFACE_CELLS_IN_RANGE_HPP__
#define __IXION_INTERFACE_CELLS_IN_RANGE_HPP__

namespace ixion {

class base_cell;

namespace iface {

/**
 * Provides a means to iterate through non-empty cell instances in a range.
 */
class cells_in_range
{
public:
    virtual ~cells_in_range() {}

    /**
     * Return a pointer to the first non-empty cell instance in the range.
     * Calling this method resets the internal position of the current cell to
     * the first non-empty cell instance.
     *
     * @return pointer to the first non-empty cell instance in the range, or
     *         NULL if all cells in the range are empty.
     */
    virtual base_cell* first() = 0;

    /**
     * Return a pointer to the next cell instance in the range.  Calling this
     * method moves the internal position of the current cell to the next
     * non-empty cell if present.
     *
     * @return pointer to the next cell instance in the range, or NULL if no
     *         more cell instances are present.
     */
    virtual base_cell* next() = 0;
};

/**
 * Identical to cells_in_range, except that the first() and next() methods
 * both return const base_cell pointers.
 */
class const_cells_in_range
{
public:
    virtual ~const_cells_in_range() {}
    virtual const base_cell* first() = 0;
    virtual const base_cell* next() = 0;
};

}}

#endif
