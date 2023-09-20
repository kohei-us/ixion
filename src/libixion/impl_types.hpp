/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <ixion/matrix.hpp>

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

} // namespace ixion

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
