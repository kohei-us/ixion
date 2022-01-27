/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_DETAIL_UTILS_HPP
#define INCLUDED_IXION_DETAIL_UTILS_HPP

#include "ixion/types.hpp"
#include "column_store_type.hpp"

#include <sstream>

namespace ixion { namespace detail {

celltype_t to_celltype(mdds::mtv::element_t mtv_type);

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

template<column_block_t BlockT>
struct make_element_range;

template<>
struct make_element_range<column_block_t::numeric>
{
    const_element_block_range<double> operator()(const column_block_shape_t& node, std::size_t length) const
    {
        const auto* blk = reinterpret_cast<const numeric_element_block*>(node.data);
        const double* p = &numeric_element_block::at(*blk, node.offset);
        length = std::min(node.size - node.offset, length);
        const double* p_end = p + length;

        return { p, p_end };
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

        return { p, p_end };
    }
};

// TODO : add specialization for the other block types as needed.

}}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
