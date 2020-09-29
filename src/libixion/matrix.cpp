/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/matrix.hpp"
#include "ixion/global.hpp"
#include "column_store_type.hpp"

#include <limits>
#include <cstring>
#include <functional>

namespace ixion {

const double nan = std::numeric_limits<double>::quiet_NaN();

struct matrix::impl
{
    matrix_store_t m_data;

    impl() {}

    impl(size_t rows, size_t cols) : m_data(rows, cols) {}

    impl(size_t rows, size_t cols, double numeric) :
        m_data(rows, cols, numeric) {}

    impl(size_t rows, size_t cols, bool boolean) :
        m_data(rows, cols, boolean) {}

    impl(size_t rows, size_t cols, const std::string& str) :
        m_data(rows, cols, str) {}

    impl(size_t rows, size_t cols, formula_error_t error) :
        m_data(rows, cols, -static_cast<int64_t>(error)) {}

    impl(const std::vector<double>& array, size_t rows, size_t cols) :
        m_data(rows, cols, array.begin(), array.end()) {}

    impl(const impl& other) : m_data(other.m_data) {}
};

struct numeric_matrix::impl
{
    std::vector<double> m_array;
    size_t m_rows;
    size_t m_cols;

    impl() : m_rows(0), m_cols(0) {}

    impl(size_t rows, size_t cols) :
        m_array(rows * cols, 0.0), m_rows(rows), m_cols(cols) {}

    impl(std::vector<double> array, size_t rows, size_t cols) :
        m_array(std::move(array)), m_rows(rows), m_cols(cols) {}

    size_t to_array_pos(size_t row, size_t col) const
    {
        return m_rows * col + row;
    }
};

matrix::matrix() :
    mp_impl(ixion::make_unique<impl>()) {}

matrix::matrix(size_t rows, size_t cols) :
    mp_impl(ixion::make_unique<impl>(rows, cols)) {}

matrix::matrix(size_t rows, size_t cols, double numeric) :
    mp_impl(ixion::make_unique<impl>(rows, cols, numeric)) {}

matrix::matrix(size_t rows, size_t cols, bool boolean) :
    mp_impl(ixion::make_unique<impl>(rows, cols, boolean)) {}

matrix::matrix(size_t rows, size_t cols, const std::string& str) :
    mp_impl(ixion::make_unique<impl>(rows, cols, str)) {}

matrix::matrix(size_t rows, size_t cols, formula_error_t error) :
    mp_impl(ixion::make_unique<impl>(rows, cols, error)) {}

matrix::matrix(const matrix& other) :
    mp_impl(ixion::make_unique<impl>(*other.mp_impl))
{
}

matrix::matrix(matrix&& other) :
    mp_impl(std::move(other.mp_impl))
{
}

matrix::matrix(const numeric_matrix& other) :
    mp_impl(ixion::make_unique<impl>(
        other.mp_impl->m_array, other.row_size(), other.col_size()))
{
}

matrix::~matrix() {}

matrix& matrix::operator= (matrix other)
{
    matrix t(std::move(other));
    swap(t);
    return *this;
}

bool matrix::is_numeric() const
{
    return mp_impl->m_data.numeric();
}

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

void matrix::set(size_t row, size_t col, bool val)
{
    mp_impl->m_data.set(row, col, val);
}

void matrix::set(size_t row, size_t col, const std::string& str)
{
    mp_impl->m_data.set(row, col, str);
}

void matrix::set(size_t row, size_t col, formula_error_t val)
{
    int64_t encoded = -static_cast<uint8_t>(val);
    mp_impl->m_data.set(row, col, encoded);
}

matrix::element matrix::get(size_t row, size_t col) const
{
    element me;
    me.type = element_type::empty;

    switch (mp_impl->m_data.get_type(row, col))
    {
        case mdds::mtm::element_numeric:
            me.type = element_type::numeric;
            me.numeric = mp_impl->m_data.get_numeric(row, col);
            break;
        case mdds::mtm::element_integer:
        {
            // This is an error value, which must be negative.
            auto v = mp_impl->m_data.get_integer(row, col);
            if (v >= 0)
                break;

            me.type = element_type::error;
            me.error = static_cast<formula_error_t>(-v);
            break;
        }
        case mdds::mtm::element_string:
        {
            me.type = element_type::string;
            me.str = &mp_impl->m_data.get_string(row, col);
            break;
        }
        case mdds::mtm::element_boolean:
            me.type = element_type::boolean;
            me.boolean = mp_impl->m_data.get_boolean(row, col);
        default:
            ;
    }

    return me;
}

size_t matrix::row_size() const
{
    return mp_impl->m_data.size().row;
}

size_t matrix::col_size() const
{
    return mp_impl->m_data.size().column;
}

void matrix::swap(matrix& r)
{
    mp_impl.swap(r.mp_impl);
}

numeric_matrix matrix::as_numeric() const
{
    matrix_store_t::size_pair_type mtx_size = mp_impl->m_data.size();

    std::vector<double> num_array(mtx_size.row*mtx_size.column, nan);
    double* dest = num_array.data();

    std::function<void(const matrix_store_t::element_block_node_type&)> f =
        [&](const matrix_store_t::element_block_node_type& node)
        {
            assert(node.offset == 0);

            switch (node.type)
            {
                case mdds::mtm::element_integer:
                {
                    // String and error values will be handled as numeric values of 0.0.
#ifndef __STDC_IEC_559__
                    throw std::runtime_error("IEEE 754 is not fully supported.");
#endif
                    std::memset(dest, 0, sizeof(double)*node.size); // IEEE 754 defines 0.0 to be 8 zero bytes.
                    std::advance(dest, node.size);
                    break;
                }
                case mdds::mtm::element_boolean:
                {
                    using block_type = matrix_store_t::boolean_block_type;
                    auto it  = block_type::begin(*node.data);
                    auto ite = block_type::end(*node.data);

                    for (; it != ite; ++it)
                        *dest++ = *it ? 1.0 : 0.0;
                    break;
                }
                case mdds::mtm::element_numeric:
                {
                    using block_type = matrix_store_t::numeric_block_type;
                    const double* src = &block_type::at(*node.data, 0);
                    std::memcpy(dest, src, sizeof(double)*node.size);
                    std::advance(dest, node.size);
                    break;
                }
                case mdds::mtm::element_string:
                {
                    // Skip string blocks.
                    std::advance(dest, node.size);
                    break;
                }
                default:
                    ;
            }
        };

    mp_impl->m_data.walk(f);

    return numeric_matrix(std::move(num_array), mtx_size.row, mtx_size.column);
}

bool matrix::operator== (const matrix& r) const
{
    return mp_impl->m_data == r.mp_impl->m_data;
}

bool matrix::operator!= (const matrix& r) const
{
    return !operator==(r);
}

numeric_matrix::numeric_matrix() : mp_impl(ixion::make_unique<impl>()) {}
numeric_matrix::numeric_matrix(size_t rows, size_t cols) :
    mp_impl(ixion::make_unique<impl>(rows, cols)) {}
numeric_matrix::numeric_matrix(std::vector<double> array, size_t rows, size_t cols) :
    mp_impl(ixion::make_unique<impl>(std::move(array), rows, cols)) {}

numeric_matrix::numeric_matrix(numeric_matrix&& r) : mp_impl(std::move(r.mp_impl)) {}

numeric_matrix::~numeric_matrix() {}

numeric_matrix& numeric_matrix::operator= (numeric_matrix other)
{
    numeric_matrix t(std::move(other));
    swap(t);

    return *this;
}

double& numeric_matrix::operator() (size_t row, size_t col)
{
    size_t pos = mp_impl->to_array_pos(row, col);
    return mp_impl->m_array[pos];
}

const double& numeric_matrix::operator() (size_t row, size_t col) const
{
    size_t pos = mp_impl->to_array_pos(row, col);
    return mp_impl->m_array[pos];
}

void numeric_matrix::swap(numeric_matrix& r)
{
    mp_impl.swap(r.mp_impl);
}

size_t numeric_matrix::row_size() const
{
    return mp_impl->m_rows;
}

size_t numeric_matrix::col_size() const
{
    return mp_impl->m_cols;
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */

