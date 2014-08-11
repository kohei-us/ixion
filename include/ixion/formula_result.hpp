/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __IXION_FORMULA_RESULT_HPP__
#define __IXION_FORMULA_RESULT_HPP__

#include "ixion/global.hpp"

#include <string>

namespace ixion {

/**
 * Store formula result which may be either numeric, textural, or error.  In
 * case the result is textural, it owns the instance of the string.
 */
class IXION_DLLPUBLIC formula_result
{
public:
    enum result_type { rt_value, rt_string, rt_error };

    formula_result();
    formula_result(const formula_result& r);
    formula_result(double v);
    formula_result(string_id_t strid);
    formula_result(formula_error_t e);
    ~formula_result();

    void reset();
    void set_value(double v);
    void set_string(string_id_t strid);
    void set_error(formula_error_t e);

    double get_value() const;
    string_id_t get_string() const;
    formula_error_t get_error() const;

    result_type get_type() const;

    std::string str(const iface::model_context& cxt) const;

    /**
     * Parse a textural representation of a formula result, and set result
     * value of appropriate type.
     */
    void parse(iface::model_context& cxt, const char* p, size_t n);

    formula_result& operator= (const formula_result& r);
    bool operator== (const formula_result& r) const;
    bool operator!= (const formula_result& r) const;

private:
    void parse_error(const char* p, size_t n);
    void parse_string(iface::model_context& cxt, const char* p, size_t n);

private:
    result_type m_type;
    union {
        string_id_t m_str_identifier;
        double m_value;
        formula_error_t m_error;
    };
};

}

#endif
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
