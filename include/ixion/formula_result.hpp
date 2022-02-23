/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_FORMULA_RESULT_HPP
#define INCLUDED_IXION_FORMULA_RESULT_HPP

#include "global.hpp"

#include <string>
#include <memory>
#include <iosfwd>

namespace ixion {

class matrix;
class model_context;

/**
 * Store formula result which may be either numeric, textural, or error.  In
 * case the result is textural, it owns the instance of the string.
 */
class IXION_DLLPUBLIC formula_result
{
    struct impl;
    std::unique_ptr<impl> mp_impl;

public:
    enum class result_type { boolean, value, string, error, matrix };

    formula_result();
    formula_result(const formula_result& r);
    formula_result(formula_result&& r);
    formula_result(bool b);
    formula_result(double v);
    formula_result(std::string str);
    formula_result(formula_error_t e);
    formula_result(matrix mtx);
    ~formula_result();

    void reset();
    void set_boolean(bool b);
    void set_value(double v);
    void set_string_value(std::string str);
    void set_error(formula_error_t e);
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
     * value of appropriate type.
     *
     * @param s formula result as a string.
     */
    void parse(std::string_view s);

    formula_result& operator= (formula_result r);
    bool operator== (const formula_result& r) const;
    bool operator!= (const formula_result& r) const;
};

IXION_DLLPUBLIC std::ostream& operator<< (std::ostream& os, formula_result::result_type v);

}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
