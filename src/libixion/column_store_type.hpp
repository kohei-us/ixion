/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once
#include "ixion/types.hpp"
#include "ixion/cell.hpp"

#include <mdds/multi_type_vector/types.hpp>
#include <mdds/multi_type_vector/macro.hpp>
#include <mdds/multi_type_vector/block_funcs.hpp>
#include <mdds/multi_type_vector.hpp>
#include <mdds/multi_type_matrix.hpp>

#include <cassert>
#include <deque>
#include <memory>
#include <string_view>

namespace ixion {

// Element types

constexpr mdds::mtv::element_t element_type_empty   = mdds::mtv::element_type_empty;
constexpr mdds::mtv::element_t element_type_boolean = mdds::mtv::element_type_boolean;
constexpr mdds::mtv::element_t element_type_numeric = mdds::mtv::element_type_double;
constexpr mdds::mtv::element_t element_type_string  = mdds::mtv::element_type_uint32;
constexpr mdds::mtv::element_t element_type_formula = mdds::mtv::element_type_user_start;
constexpr mdds::mtv::element_t element_type_inline_string = mdds::mtv::element_type_user_start + 1;

// Element block types

using boolean_element_block = mdds::mtv::boolean_element_block;
using numeric_element_block = mdds::mtv::double_element_block;
using string_element_block  = mdds::mtv::uint32_element_block;

using formula_element_block =
    mdds::mtv::noncopyable_managed_element_block<element_type_formula, ixion::formula_cell>;

MDDS_MTV_DEFINE_ELEMENT_CALLBACKS_PTR(
    formula_cell, element_type_formula, nullptr, formula_element_block)

} // namespace ixion

namespace mdds { namespace mtv {

/**
 * Specialization for cloning the formula cells stored in a formula element
 * block.
 *
 * Each formula cell instance stored in the block gets cloned, and the cells
 * belonging to the same formula group share the same cloned calc status
 * instance, which formula_cell::cloner keeps track of.  The generic
 * clone_block uses one instance of this function object per block clone and
 * applies it to the cells in stored order, which makes this work.
 *
 * Since a formula group never spans multiple columns, and the cells of a
 * group are contiguous within a column, an entire group always gets cloned as
 * a whole within a single block.
 */
template<>
struct clone_value<ixion::formula_cell*>
{
    ixion::formula_cell::cloner clone_cell;

    ixion::formula_cell* operator()(const ixion::formula_cell* p)
    {
        // A bug-free formula element block never stores a null cell.
        assert(p);
        return clone_cell(*p).release();
    }
};

}} // namespace mdds::mtv

namespace ixion {

/**
 * Thin wrapper over std::string_view in ixion namespace in order for ADL to
 * work properly.  Using std::string_view directly would cause ADL to fail.
 */
struct string_view_store
{
    std::string_view view;

    constexpr string_view_store() noexcept = default;
    constexpr string_view_store(std::string_view v) noexcept : view(v) {}
    constexpr operator std::string_view() const noexcept { return view; }
};

constexpr bool operator==(string_view_store lhs, string_view_store rhs) noexcept
{
    return lhs.view == rhs.view;
}

using inline_string_element_block =
    mdds::mtv::default_element_block<element_type_inline_string, string_view_store>;

MDDS_MTV_DEFINE_ELEMENT_CALLBACKS(
    string_view_store, element_type_inline_string, string_view_store{}, inline_string_element_block)

struct column_store_traits : mdds::mtv::default_traits
{
    // Controlled by the build systems; disabled when built without the flag.
#if IXION_COW
    static constexpr bool enable_cow = true;
#else
    static constexpr bool enable_cow = false;
#endif

    using block_funcs = mdds::mtv::element_block_funcs<
        boolean_element_block,
        numeric_element_block,
        string_element_block,
        formula_element_block,
        inline_string_element_block>;
};

/** Type that represents a whole column. */
using column_store_t = mdds::multi_type_vector<column_store_traits>;

/** Type that represents a collection of columns. */
using column_stores_t = std::deque<column_store_t>;

/**
 * The integer element blocks are used to store string ID's.  The actual
 * string element blocks are not used in the matrix store in ixion.
 */
struct matrix_store_traits
{
    typedef mdds::mtv::int64_element_block integer_element_block;
    typedef mdds::mtv::string_element_block string_element_block;
};

using matrix_store_t = mdds::multi_type_matrix<matrix_store_traits>;

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
