/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __IXION_MATRIX_HPP__
#define __IXION_MATRIX_HPP__

#include "ixion/env.hpp"

#include <cstdlib>

#include <mdds/multi_type_matrix.hpp>

namespace ixion {

/**
 * 2-dimensional matrix consisting of elements of variable types.  Each
 * element can be numeric, string, or empty.  This class is used to
 * represent range values or in-line matrices.
 */
class IXION_DLLPUBLIC matrix
{
    typedef mdds::multi_type_matrix<mdds::mtm::std_string_trait> store_type;
public:
    typedef store_type::size_pair_type size_pair_type;

    matrix(size_t rows, size_t cols);
    matrix(const matrix& other);
    ~matrix();

    bool is_numeric(size_t row, size_t col) const;
    double get_numeric(size_t row, size_t col) const;
    void set(size_t row, size_t col, double val);
    size_pair_type size() const;

private:
    store_type m_data;
};

}

#endif
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
