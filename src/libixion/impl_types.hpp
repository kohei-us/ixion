/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <ixion/matrix.hpp>
#include <ixion/types.hpp>

#include <variant>

namespace ixion {

class matrix;

class matrix_or_numeric_t
{
    using store_type = std::variant<matrix, double>;
    store_type m_value;
public:

    enum class value_type { matrix, numeric };

    matrix_or_numeric_t(matrix v);
    matrix_or_numeric_t(double v);

    value_type type() const;

    const matrix& get_matrix() const;
    double get_numeric() const;
};

template<typename T>
class formula_op_result
{
    using store_type = std::variant<T, formula_error_t>;
    store_type m_value;

public:
    formula_op_result(T v) : m_value(std::move(v)) {}
    formula_op_result(formula_error_t err) : m_value(err) {}

    operator bool() const { return m_value.index() == 0; }

    T operator*() const
    {
        return std::get<T>(m_value);
    }

    formula_error_t error() const
    {
        return std::get<formula_error_t>(m_value);
    }
};

} // namespace ixion

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
