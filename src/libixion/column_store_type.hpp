/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once
#include "ixion/types.hpp"
#include "ixion/cell.hpp"

#include "calc_status.hpp"

#include <mdds/multi_type_vector/types.hpp>
#include <mdds/multi_type_vector/macro.hpp>
#include <mdds/multi_type_vector/block_funcs.hpp>
#include <mdds/multi_type_vector.hpp>
#include <mdds/multi_type_matrix.hpp>

#include <cstdint>
#include <deque>
#include <memory>
#include <string_view>
#include <unordered_map>

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
 * Specialization for cloning a formula element block.
 *
 * Each formula cell instance stored in the block gets cloned, and the cells
 * belonging to the same formula group share the same cloned calc status
 * instance.
 *
 * Since a formula group never spans multiple columns, and the cells of a
 * group are contiguous within a column, an entire group always gets cloned as
 * a whole within a single block.
 */
template<>
struct clone_block<ixion::formula_element_block>
{
    ixion::formula_element_block* operator()(const ixion::formula_element_block& src) const
    {
        auto dest = std::make_unique<ixion::formula_element_block>();
        auto& dest_store = dest->store();
        dest_store.reserve(src.store().size());

        std::unordered_map<std::uintptr_t, ixion::calc_status_ptr_t> group_status;

        for (const ixion::formula_cell* p : src.store())
        {
            if (!p)
            {
                dest_store.push_back(nullptr);
                continue;
            }

            // Create a null calc status on new group identity to let the cloned
            // formula cell create a new instance.  On second encounters it
            // reuses the previously created calc status for the same group.
            ixion::calc_status_ptr_t& cs = group_status[p->get_group_properties().identity];
            dest_store.push_back(p->clone(cs).release());
        }

        return dest.release();
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
