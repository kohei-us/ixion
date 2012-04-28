/*************************************************************************
 *
 * Copyright (c) 2012 Kohei Yoshida
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

#ifndef __IXION_GRID_MAP_TRAIT_HPP__
#define __IXION_GRID_MAP_TRAIT_HPP__

#include "ixion/types.hpp"
#include "ixion/cell.hpp"

#include <mdds/grid_map_trait.hpp>

namespace mdds { namespace gridmap {

const mdds::gridmap::cell_t celltype_formula = mdds::gridmap::celltype_user_start;

typedef mdds::gridmap::managed_cell_block<celltype_formula, ixion::formula_cell> formula_cell_block;

cell_t get_cell_type(const ixion::formula_cell*)
{
    return celltype_formula;
}

void set_value(base_cell_block& block, size_t pos, ixion::formula_cell* p)
{
    formula_cell_block::set_value(block, pos, p);
}

template<typename _Iter>
void set_values(
    base_cell_block& block, size_t pos, ixion::formula_cell*, const _Iter& it_begin, const _Iter& it_end)
{
    formula_cell_block::set_values(block, pos, it_begin, it_end);
}

void get_value(const base_cell_block& block, size_t pos, ixion::formula_cell*& val)
{
    formula_cell_block::get_value(block, pos, val);
}

void append_value(base_cell_block& block, ixion::formula_cell* val)
{
    formula_cell_block::append_value(block, val);
}

void prepend_value(base_cell_block& block, ixion::formula_cell* val)
{
    formula_cell_block::prepend_value(block, val);
}

template<typename _Iter>
void append_values(mdds::gridmap::base_cell_block& block, ixion::formula_cell*, const _Iter& it_begin, const _Iter& it_end)
{
    formula_cell_block::append_values(block, it_begin, it_end);
}

template<typename _Iter>
void prepend_values(mdds::gridmap::base_cell_block& block, ixion::formula_cell*, const _Iter& it_begin, const _Iter& it_end)
{
    formula_cell_block::prepend_values(block, it_begin, it_end);
}

template<typename _Iter>
void assign_values(mdds::gridmap::base_cell_block& dest, ixion::formula_cell*, const _Iter& it_begin, const _Iter& it_end)
{
    formula_cell_block::assign_values(dest, it_begin, it_end);
}

template<typename _Iter>
void insert_values(
    mdds::gridmap::base_cell_block& block, size_t pos, ixion::formula_cell*, const _Iter& it_begin, const _Iter& it_end)
{
    formula_cell_block::insert_values(block, pos, it_begin, it_end);
}

void get_empty_value(ixion::formula_cell*& val)
{
    val = NULL;
}

}}

namespace ixion {

struct grid_map_trait
{
    typedef sheet_t sheet_key_type;
    typedef row_t row_key_type;
    typedef col_t col_key_type;

    typedef mdds::gridmap::cell_block_func cell_block_func;
};

}

#endif
