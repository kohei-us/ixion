/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once
#include "ixion/types.hpp"
#include "column_store_type.hpp"

#include <memory>
#include <sstream>

namespace ixion { namespace detail {

/**
 * Use this to ensure access to a pimpl object as const if it's wrapped inside
 * `std::unique_ptr<T>`.
 *
 * If you directly access pimpl object to get its method via `mp_impl->`,
 * it will match a non-const overload if the method being accessed has both
 * const and non-const overloads even if the `mp_impl` itself is inside a
 * const member method.  It's because in such a situation the const qualifier
 * gets applied to the std::unique_ptr<T> wrapper but the wrapper itself still
 * returns a non-const pointer of T...
 */
template<typename T>
const T& cimpl(const std::unique_ptr<T>& p)
{
    return *p;
}

cell_t to_celltype(mdds::mtv::element_t mtv_type);

cell_value_t to_cell_value_type(
    const column_store_t::const_position_type& pos, formula_result_wait_policy_t policy);

template<std::size_t S, typename T>
void ensure_max_size(const T& v)
{
    static_assert(sizeof(T) <= S, "The size of the value exceeded allowed size limit.");
}

template<typename T>
class const_element_block_range
{
    const T* m_begin;
    const T* m_end;

public:
    const_element_block_range(const T* begin, const T* end) : m_begin(begin), m_end(end) {}

    const T* begin() const { return m_begin; }
    const T* end() const { return m_end; }
};

/* Specialization for bool in order to handle std::vector<bool>. */
template<>
class const_element_block_range<bool>
{
    using iterator_type = boolean_element_block::const_iterator;
    iterator_type m_begin;
    iterator_type m_end;

public:
    const_element_block_range(const iterator_type& begin, const iterator_type& end) :
        m_begin(begin), m_end(end) {}

    iterator_type begin() const { return m_begin; }
    iterator_type end() const { return m_end; }
};

template<column_block_t BlockT>
struct make_element_range;

template<>
struct make_element_range<column_block_t::boolean>
{
    const_element_block_range<bool> operator()(const column_block_shape_t& node, std::size_t length) const
    {
        // NB: special treatment for std::vector<bool> which underlies boolean_element_block.
        const auto* blk = reinterpret_cast<const boolean_element_block*>(node.data);
        auto it = boolean_element_block::cbegin(*blk);
        it = std::next(it, node.offset);
        length = std::min(node.size - node.offset, length);
        auto it_end = std::next(it, length);

        return {it, it_end};
    }
};

template<>
struct make_element_range<column_block_t::numeric>
{
    const_element_block_range<double> operator()(const column_block_shape_t& node, std::size_t length) const
    {
        const auto* blk = reinterpret_cast<const numeric_element_block*>(node.data);
        const double* p = &numeric_element_block::at(*blk, node.offset);
        length = std::min(node.size - node.offset, length);
        const double* p_end = p + length;

        return {p, p_end};
    }
};

template<>
struct make_element_range<column_block_t::formula>
{
    const_element_block_range<const formula_cell*> operator()(const column_block_shape_t& node, std::size_t length) const
    {
        const auto* blk = reinterpret_cast<const formula_element_block*>(node.data);
        auto p = &formula_element_block::at(*blk, node.offset);
        length = std::min(node.size - node.offset, length);
        auto p_end = p + length;

        return {p, p_end};
    }
};

// TODO : add specialization for the other block types as needed.

}}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
