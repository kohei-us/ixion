/*************************************************************************
 *
 * Copyright (c) 2013 Kohei Yoshida
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 ************************************************************************/

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

MDDS_MTV_DEFINE_ELEMENT_CALLBACKS_PTR(formula_cell, element_type_formula, NULL, formula_element_block)

typedef mdds::mtv::custom_block_func1<formula_element_block> ixion_element_block_func;
typedef mdds::multi_type_vector<ixion_element_block_func> column_store_t;

}

#endif
