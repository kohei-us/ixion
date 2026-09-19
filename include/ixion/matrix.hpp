/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once
#include "types.hpp"

#include <memory>
#include <vector>
#include <string>
#include <variant>

namespace ixion {

class numeric_matrix;

/**
 * 2-dimensional matrix consisting of elements of variable types.  Each
 * element can be numeric, boolean, string, error, or empty.  This class is
 * used to represent range values or in-line matrices.
 */
class IXION_DLLPUBLIC matrix
{
    struct impl;
    std::unique_ptr<impl> mp_impl;

public:

    /** Type of a matrix element. */
    enum class element_type { numeric, string, boolean, error, empty };

    /**
     * Type and value of one matrix element, as returned by get().
     */
    struct element
    {
        /** Type of the stored value.  An empty element stores no value. */
        using value_type = std::variant<double, bool, std::string_view, formula_error_t>;

        /** Type of the element. */
        element_type type;
        /**
         * Value of the element.  A string value is a view into the matrix,
         * valid as long as the matrix is alive and the element is not
         * overwritten.
         */
        value_type value;
    };

    /** Construct an empty matrix with no rows and no columns. */
    matrix();
    /** Construct a matrix of the given size, with all elements empty. */
    matrix(size_t rows, size_t cols);
    /** Construct a matrix of the given size, with all elements set to a number. */
    matrix(size_t rows, size_t cols, double numeric);
    /** Construct a matrix of the given size, with all elements set to a boolean. */
    matrix(size_t rows, size_t cols, bool boolean);
    /** Construct a matrix of the given size, with all elements set to a string. */
    matrix(size_t rows, size_t cols, std::string str);
    /** Construct a matrix of the given size, with all elements set to an error. */
    matrix(size_t rows, size_t cols, formula_error_t error);
    /** Copy constructor. */
    matrix(const matrix& other);
    /** Move constructor. */
    matrix(matrix&& other);
    /** Construct a matrix with the values of a numeric matrix. */
    matrix(const numeric_matrix& other);
    ~matrix();

    /** Assignment. */
    matrix& operator= (matrix other);

    /**
     * Determine if the entire matrix consists only of numeric value elements.
     *
     * @return true if the entire matrix consits only of numeric value
     *         elements, false otherwise.
     */
    bool is_numeric() const;

    /**
     * Get the value of an element as a boolean.  A numeric element is true
     * when it is not zero, and a string or empty element is false.
     *
     * @param row Row position of the element.
     * @param col Column position of the element.
     *
     * @return Boolean value of the element.
     */
    bool get_boolean(size_t row, size_t col) const;

    /**
     * Determine if an element holds a numeric or boolean value.
     *
     * @param row Row position of the element.
     * @param col Column position of the element.
     *
     * @return True if the element is numeric or boolean, false otherwise.
     */
    bool is_numeric(size_t row, size_t col) const;

    /**
     * Get the value of an element as a number.  A boolean element yields 1
     * or 0, and a string or empty element yields 0.
     *
     * @param row Row position of the element.
     * @param col Column position of the element.
     *
     * @return Numeric value of the element.
     */
    double get_numeric(size_t row, size_t col) const;

    /**
     * Set a numeric value to an element, replacing its current value.
     *
     * @param row Row position of the element.
     * @param col Column position of the element.
     * @param val Value to set.
     */
    void set(size_t row, size_t col, double val);
    /** @copydoc set(size_t, size_t, double) */
    void set(size_t row, size_t col, bool val);
    /**
     * Set a string value to an element, replacing its current value.
     *
     * @param row Row position of the element.
     * @param col Column position of the element.
     * @param str Value to set.
     */
    void set(size_t row, size_t col, std::string str);
    /** @copydoc set(size_t, size_t, double) */
    void set(size_t row, size_t col, formula_error_t val);

    /**
     * Get the type and value of an element.
     *
     * @param row Row position of the element.
     * @param col Column position of the element.
     *
     * @return Type and value of the element.
     */
    element get(size_t row, size_t col) const;

    /** Get the number of rows. */
    size_t row_size() const;
    /** Get the number of columns. */
    size_t col_size() const;

    /** Swap the content with another matrix. */
    void swap(matrix& r);

    /**
     * Convert the matrix to a numeric matrix of the same size.  A boolean
     * element becomes 1 or 0, an error element becomes 0, and a string or
     * empty element becomes NaN.
     *
     * @return Numeric matrix with the converted values.
     */
    numeric_matrix as_numeric() const;

    /**
     * Compare two matrices.  They are equal when they have the same size
     * and all their elements have the same type and value.
     *
     * @param r Matrix to compare with.
     *
     * @return True if the two matrices are equal, false otherwise.
     */
    bool operator== (const matrix& r) const;
};

/**
 * 2-dimensional matrix whose elements are all numeric, stored in
 * column-major order.
 */
class IXION_DLLPUBLIC numeric_matrix
{
    friend class matrix;

    struct impl;
    std::unique_ptr<impl> mp_impl;

public:
    /** Construct an empty matrix with no rows and no columns. */
    numeric_matrix();
    /** Construct a matrix of the given size, with all elements set to 0. */
    numeric_matrix(size_t rows, size_t cols);

    /**
     * Constructor with initial values.
     *
     * @param array  Array of initial values stored in column-major order.
     * @param rows Number of rows.
     * @param cols Number of columns.
     */
    numeric_matrix(std::vector<double> array, size_t rows, size_t cols);
    /** Move constructor. */
    numeric_matrix(numeric_matrix&& r);
    ~numeric_matrix();

    /** Assignment. */
    numeric_matrix& operator= (numeric_matrix other);

    /**
     * Access an element.  The position is not range-checked.
     *
     * @param row Row position of the element.
     * @param col Column position of the element.
     *
     * @return Reference to the element.
     */
    double& operator() (size_t row, size_t col);
    /** @copydoc operator()(size_t, size_t) */
    const double& operator() (size_t row, size_t col) const;

    /** Swap the content with another matrix. */
    void swap(numeric_matrix& r);

    /** Get the number of rows. */
    size_t row_size() const;
    /** Get the number of columns. */
    size_t col_size() const;
};

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
