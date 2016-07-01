/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_MATRIX_HPP
#define INCLUDED_IXION_MATRIX_HPP

#include "ixion/env.hpp"

#include <memory>

namespace ixion {

/**
 * 2-dimensional matrix consisting of elements of variable types.  Each
 * element can be numeric, string, or empty.  This class is used to
 * represent range values or in-line matrices.
 */
class IXION_DLLPUBLIC matrix
{
    struct impl;
    std::unique_ptr<impl> mp_impl;

public:

    matrix(size_t rows, size_t cols);
    matrix(const matrix& other);
    ~matrix();

    bool is_numeric(size_t row, size_t col) const;
    double get_numeric(size_t row, size_t col) const;
    void set(size_t row, size_t col, double val);
    size_t row_size() const;
    size_t col_size() const;
};

}

#endif
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
