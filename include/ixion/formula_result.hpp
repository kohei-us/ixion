/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_FORMULA_RESULT_HPP
#define INCLUDED_IXION_FORMULA_RESULT_HPP

#include "ixion/global.hpp"

#include <string>
#include <memory>

namespace ixion {

class matrix;

namespace iface {

class formula_model_access;

}

/**
 * Store formula result which may be either numeric, textural, or error.  In
 * case the result is textural, it owns the instance of the string.
 */
class IXION_DLLPUBLIC formula_result
{
    struct impl;
    std::unique_ptr<impl> mp_impl;

public:
    enum class result_type { value, string, error, matrix };

    formula_result();
    formula_result(const formula_result& r);
    formula_result(formula_result&& r);
    formula_result(double v);
    formula_result(string_id_t strid);
    formula_result(formula_error_t e);
    ~formula_result();

    void reset();
    void set_value(double v);
    void set_string(string_id_t strid);
    void set_error(formula_error_t e);
    void set_matrix(matrix mtx);

    /**
     * Get a numeric result value.  The caller must make sure the result is of
     * numeric type, else the behavior is undefined.
     *
     * @return numeric result value.
     */
    double get_value() const;

    /**
     * Get a string ID for textural result value.  The caller must make sure
     * the result is of textural type, else the behavior is undefined.
     *
     * @return string ID.
     */
    string_id_t get_string() const;

    /**
     * Get an error value of the result.  The caller must make sure that the
     * result is of error type, else the behavior is undefined.
     *
     * @return enum value representing the error.
     * @see ixion::get_formula_error_name
     */
    formula_error_t get_error() const;

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
    std::string str(const iface::formula_model_access& cxt) const;

    /**
     * Parse a textural representation of a formula result, and set result
     * value of appropriate type.
     */
    void parse(iface::formula_model_access& cxt, const char* p, size_t n);

    formula_result& operator= (formula_result r);
    bool operator== (const formula_result& r) const;
    bool operator!= (const formula_result& r) const;
};

}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
