/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef IXION_COLUMN_STORE_TYPE_HPP
#define IXION_COLUMN_STORE_TYPE_HPP

#include "ixion/types.hpp"
#include "ixion/cell.hpp"

#include <mdds/multi_type_vector/types.hpp>
#include <mdds/multi_type_vector/macro.hpp>
#include <mdds/multi_type_vector/block_funcs.hpp>
#include <mdds/multi_type_vector.hpp>
#include <mdds/multi_type_matrix.hpp>

#include <deque>

namespace ixion {

// Element types

constexpr mdds::mtv::element_t element_type_empty   = mdds::mtv::element_type_empty;
constexpr mdds::mtv::element_t element_type_boolean = mdds::mtv::element_type_boolean;
constexpr mdds::mtv::element_t element_type_numeric = mdds::mtv::element_type_double;
constexpr mdds::mtv::element_t element_type_string  = mdds::mtv::element_type_uint32;
constexpr mdds::mtv::element_t element_type_formula = mdds::mtv::element_type_user_start;

// Element block types

using boolean_element_block = mdds::mtv::boolean_element_block;
using numeric_element_block = mdds::mtv::double_element_block;
using string_element_block  = mdds::mtv::uint32_element_block;

using formula_element_block =
    mdds::mtv::noncopyable_managed_element_block<element_type_formula, ixion::formula_cell>;

MDDS_MTV_DEFINE_ELEMENT_CALLBACKS_PTR(formula_cell, element_type_formula, nullptr, formula_element_block)

struct column_store_traits : mdds::mtv::default_traits
{
    using block_funcs = mdds::mtv::element_block_funcs<
        boolean_element_block,
        numeric_element_block,
        string_element_block,
        formula_element_block>;
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

#endif
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
