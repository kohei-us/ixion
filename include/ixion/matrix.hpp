/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_MATRIX_HPP
#define INCLUDED_IXION_MATRIX_HPP

#include "types.hpp"

#include <memory>
#include <vector>
#include <string>
#include <variant>

namespace ixion {

class numeric_matrix;

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

    enum class element_type { numeric, string, boolean, error, empty };

    struct element
    {
        using value_type = std::variant<double, bool, std::string_view, formula_error_t>;

        element_type type;
        value_type value;
    };

    matrix();
    matrix(size_t rows, size_t cols);
    matrix(size_t rows, size_t cols, double numeric);
    matrix(size_t rows, size_t cols, bool boolean);
    matrix(size_t rows, size_t cols, const std::string& str);
    matrix(size_t rows, size_t cols, formula_error_t error);
    matrix(const matrix& other);
    matrix(matrix&& other);
    matrix(const numeric_matrix& other);
    ~matrix();

    matrix& operator= (matrix other);

    /**
     * Determine if the entire matrix consists only of numeric value elements.
     *
     * @return true if the entire matrix consits only of numeric value
     *         elements, false otherwise.
     */
    bool is_numeric() const;

    bool get_boolean(size_t row, size_t col) const;
    bool is_numeric(size_t row, size_t col) const;
    double get_numeric(size_t row, size_t col) const;
    void set(size_t row, size_t col, double val);
    void set(size_t row, size_t col, bool val);
    void set(size_t row, size_t col, const std::string& str);
    void set(size_t row, size_t col, formula_error_t val);

    element get(size_t row, size_t col) const;

    size_t row_size() const;
    size_t col_size() const;

    void swap(matrix& r);

    numeric_matrix as_numeric() const;

    bool operator== (const matrix& r) const;
    bool operator!= (const matrix& r) const;
};

class IXION_DLLPUBLIC numeric_matrix
{
    friend class matrix;

    struct impl;
    std::unique_ptr<impl> mp_impl;

public:
    numeric_matrix();
    numeric_matrix(size_t rows, size_t cols);

    /**
     * Constructor with initial values.
     *
     * @param array  Array of initial values stored in column-major order.
     * @param rows Number of rows.
     * @param cols Number of columns.
     */
    numeric_matrix(std::vector<double> array, size_t rows, size_t cols);
    numeric_matrix(numeric_matrix&& r);
    ~numeric_matrix();

    numeric_matrix& operator= (numeric_matrix other);

    double& operator() (size_t row, size_t col);
    const double& operator() (size_t row, size_t col) const;

    void swap(numeric_matrix& r);

    size_t row_size() const;
    size_t col_size() const;
};

}

#endif
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
