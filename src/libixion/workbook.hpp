/*************************************************************************
 *
 * Copyright (c) 2012 Kohei Yoshida
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

#ifndef __IXION_WORKBOOK_HPP__
#define __IXION_WORKBOOK_HPP__

#include "multi_type_vector_trait.hpp"

#include <mdds/multi_type_vector.hpp>
#include <mdds/multi_type_vector_trait.hpp>

#include <vector>

namespace ixion {

class worksheet
{
public:
    typedef mdds::multi_type_vector<ixion_element_block_func> column_type;
    typedef column_type::size_type size_type;

    worksheet();
    worksheet(size_type row_size, size_type col_size);
    ~worksheet();

    column_type& operator[](size_type n) { return *m_columns[n]; }
    const column_type& operator[](size_type n) const { return *m_columns[n]; }

    column_type& at(size_type n) { return *m_columns.at(n); }
    const column_type& at(size_type n) const { return *m_columns.at(n); }

    /**
     * Return the number of columns.
     *
     * @return number of columns.
     */
    size_type size() const { return m_columns.size(); }

private:
    std::vector<column_type*> m_columns;
};

class workbook
{
public:
    workbook();
    workbook(size_t sheet_size, size_t row_size, size_t col_size);
    ~workbook();

    worksheet& operator[](size_t n) { return *m_sheets[n]; }
    const worksheet& operator[](size_t n) const { return *m_sheets[n]; }

    worksheet& at(size_t n) { return *m_sheets.at(n); }
    const worksheet& at(size_t n) const { return *m_sheets.at(n); }

    void push_back(size_t row_size, size_t col_size);

    size_t size() const { return m_sheets.size(); }

private:
    std::vector<worksheet*> m_sheets;
};

}

#endif
