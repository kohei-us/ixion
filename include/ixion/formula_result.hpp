/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once
#include "global.hpp"

#include <string>
#include <memory>
#include <iosfwd>

namespace ixion {

class matrix;
class model_context;

/**
 * Store formula result which may be either boolean, numeric, textural, error,
 * or matrix.  In case the result is textural, it owns the instance of the
 * string.
 */
class IXION_DLLPUBLIC formula_result
{
    struct impl;
    std::unique_ptr<impl> mp_impl;

public:
    /** Type of the value a result holds. */
    enum class result_type { boolean, value, string, error, matrix };

    /** Construct a numeric result with a value of 0. */
    formula_result();
    /** Copy constructor. */
    formula_result(const formula_result& r);
    /** Move constructor. */
    formula_result(formula_result&& r);
    /** Construct a boolean result. */
    formula_result(bool b);
    /** Construct a numeric result. */
    formula_result(double v);
    /** Construct a string result. */
    formula_result(std::string str);
    /** Construct an error result. */
    formula_result(formula_error_t e);
    /** Construct a matrix result. */
    formula_result(matrix mtx);
    ~formula_result();

    /** Reset the result to a numeric value of 0. */
    void reset();
    /** Set a boolean value, replacing the current value and type. */
    void set_boolean(bool b);
    /** Set a numeric value, replacing the current value and type. */
    void set_value(double v);
    /** Set a string value, replacing the current value and type. */
    void set_string_value(std::string str);
    /** Set an error value, replacing the current value and type. */
    void set_error(formula_error_t e);
    /** Set a matrix value, replacing the current value and type. */
    void set_matrix(matrix mtx);

    /**
     * Get a boolean result value.  The caller must make sure the result is of
     * boolean type, else the behavior is undefined.
     *
     * @return boolean result value.
     */
    bool get_boolean() const;

    /**
     * Get a numeric result value.  The caller must make sure the result is of
     * numeric type, else the behavior is undefined.
     *
     * @return numeric result value.
     */
    double get_value() const;

    /**
     * Get a string value for textural result.  The caller must make
     * sure the result is of textural type, else the behavior is undefined.
     *
     * @return string value.
     */
    const std::string& get_string() const;

    /**
     * Get an error value of the result.  The caller must make sure that the
     * result is of error type, else the behavior is undefined.
     *
     * @return enum value representing the error.
     * @see ixion::get_formula_error_name
     */
    formula_error_t get_error() const;

    /**
     * Get a matrix value of the result.  The caller must make sure that the
     * result is of matrix type, else the behavior is undefined.
     *
     * @return matrix result value.
     */
    const matrix& get_matrix() const;

    /**
     * Get a matrix value of the result.  The caller must make sure that the
     * result is of matrix type, else the behavior is undefined.
     *
     * @return matrix result value.
     */
    matrix& get_matrix();

    /**
     * Get the type of result.
     *
     * @return enum value representing the result type.
     */
    result_type get_type() const;

    /**
     * Get a string representation of the result value no matter what the
     * result type is.
     *
     * @param cxt model context object.
     *
     * @return string representation of the result value.
     */
    std::string str(const model_context& cxt) const;

    /**
     * Parse a textural representation of a formula result, and set result
     * value of appropriate type.  A string starting with '#' is an error
     * name such as "#DIV/0!", a string enclosed in double quotes is a string
     * value, "true" and "false" are boolean values, and anything else is a
     * number.  An empty string leaves the result unchanged.  A matrix cannot
     * be parsed.
     *
     * @param s formula result as a string.
     *
     * @throw general_error If the string is a malformed error name or a
     *                      string value without a closing quote.
     */
    void parse(std::string_view s);

    /** Assignment. */
    formula_result& operator= (formula_result r);

    /**
     * Compare two results.  They are equal when they have the same type and
     * the same value.
     *
     * @param r Result to compare with.
     *
     * @return True if the two results are equal, false otherwise.
     */
    bool operator== (const formula_result& r) const;
};

/** Print the name of a result type, such as "value" or "matrix". */
IXION_DLLPUBLIC std::ostream& operator<< (std::ostream& os, formula_result::result_type v);

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
