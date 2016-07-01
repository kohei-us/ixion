/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/matrix.hpp"
#include "ixion/global.hpp"

#include <mdds/multi_type_matrix.hpp>

namespace ixion {

typedef mdds::multi_type_matrix<mdds::mtm::std_string_trait> store_type;

struct matrix::impl
{
    store_type m_data;

    impl(size_t rows, size_t cols) : m_data(rows, cols) {}
    impl(const impl& other) : m_data(other.m_data) {}
};

matrix::matrix(size_t rows, size_t cols) :
    mp_impl(ixion::make_unique<impl>(rows, cols)) {}

matrix::matrix(const matrix& other) :
    mp_impl(ixion::make_unique<impl>(*other.mp_impl)) {}

matrix::~matrix() {}

bool matrix::is_numeric(size_t row, size_t col) const
{
    switch (mp_impl->m_data.get_type(row, col))
    {
        case mdds::mtm::element_numeric:
        case mdds::mtm::element_boolean:
            return true;
        default:
            ;
    }

    return false;
}

double matrix::get_numeric(size_t row, size_t col) const
{
    return mp_impl->m_data.get_numeric(row, col);
}

void matrix::set(size_t row, size_t col, double val)
{
    mp_impl->m_data.set(row, col, val);
}

size_t matrix::row_size() const
{
    return mp_impl->m_data.size().row;
}

size_t matrix::col_size() const
{
    return mp_impl->m_data.size().column;
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

