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

#include <mdds/multi_type_vector_trait.hpp>
#include <mdds/multi_type_vector_types.hpp>
#include <mdds/multi_type_vector.hpp>
#include <mdds/multi_type_vector_macro.hpp>
#include <mdds/multi_type_vector_custom_func1.hpp>

namespace ixion {

// Element types

const mdds::mtv::element_t element_type_empty = mdds::mtv::element_type_empty;
const mdds::mtv::element_t element_type_boolean = mdds::mtv::element_type_boolean;
const mdds::mtv::element_t element_type_numeric = mdds::mtv::element_type_numeric;
const mdds::mtv::element_t element_type_string = mdds::mtv::element_type_ulong;
const mdds::mtv::element_t element_type_formula = mdds::mtv::element_type_user_start;

// Element block types

typedef mdds::mtv::boolean_element_block boolean_element_block;
typedef mdds::mtv::numeric_element_block numeric_element_block;
typedef mdds::mtv::ulong_element_block string_element_block;

typedef mdds::mtv::noncopyable_managed_element_block<
    element_type_formula, ixion::formula_cell> formula_element_block;

MDDS_MTV_DEFINE_ELEMENT_CALLBACKS_PTR(formula_cell, element_type_formula, nullptr, formula_element_block)

typedef mdds::mtv::custom_block_func1<formula_element_block> ixion_element_block_func;

/** Type that represents a whole column. */
typedef mdds::multi_type_vector<ixion_element_block_func> column_store_t;

/** Type that represents a collection of columns. */
typedef std::vector<column_store_t*> column_stores_t;

}

#endif
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
