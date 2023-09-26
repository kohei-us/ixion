/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "impl_types.hpp"

#include <stdexcept>

namespace ixion {

resolved_stack_value::resolved_stack_value(matrix v) : m_value(std::move(v)) {}
resolved_stack_value::resolved_stack_value(double v) : m_value(v) {}
resolved_stack_value::resolved_stack_value(std::string v) : m_value(std::move(v)) {}

resolved_stack_value::value_type resolved_stack_value::type() const
{
    return value_type(m_value.index());
}

const matrix& resolved_stack_value::get_matrix() const
{
    return std::get<matrix>(m_value);
}

double resolved_stack_value::get_numeric() const
{
    return std::get<double>(m_value);
}

const std::string& resolved_stack_value::get_string() const
{
    return std::get<std::string>(m_value);
}

} // namespace ixion

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
