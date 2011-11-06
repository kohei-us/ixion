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

#ifndef __IXION_MATRIX_HPP__
#define __IXION_MATRIX_HPP__

#include <cstdlib>
#include <mdds/mixed_type_matrix.hpp>

namespace ixion {

/**
 * 2-dimensional matrix consisting of elements of variable types.  Each
 * element can be numeric, string, or empty.  This class is used to
 * represent range values or in-line matrices.
 */
class matrix
{
    typedef ::mdds::mixed_type_matrix< ::std::string, unsigned char> store_type;
public:
    typedef store_type::size_pair_type size_pair_type;

    matrix(size_t rows, size_t cols, ::mdds::matrix_density_t density_type = ::mdds::matrix_density_sparse_empty);
    matrix(const matrix& other);
    ~matrix();

    double get_numeric(size_t row, size_t col) const;
    void set(size_t row, size_t col, double val);
    size_pair_type size() const;

private:
    store_type m_data;
};

}

#endif
