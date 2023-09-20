/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "impl_types.hpp"

#include <stdexcept>

namespace ixion {

matrix_or_numeric_t::matrix_or_numeric_t(matrix v) : m_value(std::move(v)) {}
matrix_or_numeric_t::matrix_or_numeric_t(double v) : m_value(v) {}

matrix_or_numeric_t::value_type matrix_or_numeric_t::type() const
{
    switch (m_value.index())
    {
        case 0:
            return value_type::matrix;
        case 1:
            return value_type::numeric;
    }

    throw std::logic_error{"invalid type index"};
}

const matrix& matrix_or_numeric_t::get_matrix() const
{
    return std::get<matrix>(m_value);
}

double matrix_or_numeric_t::get_numeric() const
{
    return std::get<double>(m_value);
}

} // namespace ixion

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
