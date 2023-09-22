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
#include <string>

namespace ixion {

class matrix;

/**
 * Similar to stack_value but does not store a reference; it only stores a
 * static value.
 */
class resolved_stack_value
{
    // Keep the type ordering in sync with value_type's.
    using store_type = std::variant<matrix, double, std::string>;
    store_type m_value;
public:

    enum class value_type { matrix, numeric, string };

    resolved_stack_value(matrix v);
    resolved_stack_value(double v);
    resolved_stack_value(std::string v);

    value_type type() const;

    const matrix& get_matrix() const;
    double get_numeric() const;
    const std::string& get_string() const;
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
