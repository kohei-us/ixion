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

#include <mdds/multi_type_vector_trait.hpp>
#include <mdds/multi_type_vector_types.hpp>

namespace ixion {

class formula_cell;

const mdds::mtv::element_t element_type_formula = mdds::mtv::element_type_user_start;

typedef mdds::mtv::noncopyable_managed_element_block<
    element_type_formula, ixion::formula_cell> formula_element_block;

inline mdds::mtv::element_t mdds_mtv_get_element_type(const formula_cell*)
{
    return element_type_formula;
}

inline void mdds_mtv_set_value(mdds::mtv::base_element_block& block, size_t pos, formula_cell* val)
{
    formula_element_block::set_value(block, pos, val);
}

inline void mdds_mtv_get_value(const mdds::mtv::base_element_block& block, size_t pos, formula_cell*& val)
{
    formula_element_block::get_value(block, pos, val);
}

template<typename _Iter>
void mdds_mtv_set_values(
    mdds::mtv::base_element_block& block, size_t pos, const formula_cell*&, const _Iter& it_begin, const _Iter& it_end)
{
    formula_element_block::set_values(block, pos, it_begin, it_end);
}

inline void mdds_mtv_append_value(mdds::mtv::base_element_block& block, formula_cell* val)
{
    formula_element_block::append_value(block, val);
}

inline void mdds_mtv_prepend_value(mdds::mtv::base_element_block& block, formula_cell* val)
{
    formula_element_block::prepend_value(block, val);
}

template<typename _Iter>
void mdds_mtv_prepend_values(mdds::mtv::base_element_block& block, const formula_cell*, const _Iter& it_begin, const _Iter& it_end)
{
    formula_element_block::prepend_values(block, it_begin, it_end);
}

template<typename _Iter>
void mdds_mtv_append_values(mdds::mtv::base_element_block& block, const formula_cell*, const _Iter& it_begin, const _Iter& it_end)
{
    formula_element_block::append_values(block, it_begin, it_end);
}

template<typename _Iter>
void mdds_mtv_assign_values(mdds::mtv::base_element_block& dest, const formula_cell*, const _Iter& it_begin, const _Iter& it_end)
{
    formula_element_block::assign_values(dest, it_begin, it_end);
}

inline void mdds_mtv_get_empty_value(formula_cell*& val)
{
    val = NULL;
}

template<typename _Iter>
void mdds_mtv_insert_values(
    mdds::mtv::base_element_block& block, size_t pos, const formula_cell*&, const _Iter& it_begin, const _Iter& it_end)
{
    formula_element_block::insert_values(block, pos, it_begin, it_end);
}

inline mdds::mtv::base_element_block* mdds_mtv_create_new_block(size_t init_size, formula_cell* val)
{
    assert(init_size <= 1);
    return formula_element_block::create_block_with_value(init_size, val);
}

struct ixion_element_block_func
{
    inline static mdds::mtv::base_element_block* create_new_block(
        mdds::mtv::element_t type, size_t init_size)
    {
        switch (type)
        {
            case element_type_formula:
                return formula_element_block::create_block(init_size);
            default:
                return mdds::mtv::element_block_func::create_new_block(type, init_size);
        }
    }

    inline static mdds::mtv::base_element_block* clone_block(const mdds::mtv::base_element_block& block)
    {
        switch (mdds::mtv::get_block_type(block))
        {
            case element_type_formula:
                return formula_element_block::clone_block(block);
            default:
                return mdds::mtv::element_block_func::clone_block(block);
        }
    }

    inline static void delete_block(mdds::mtv::base_element_block* p)
    {
        if (!p)
            return;

        switch (mdds::mtv::get_block_type(*p))
        {
            case element_type_formula:
                formula_element_block::delete_block(p);
            break;
            default:
                mdds::mtv::element_block_func::delete_block(p);
        }
    }

    inline static void resize_block(mdds::mtv::base_element_block& block, size_t new_size)
    {
        switch (mdds::mtv::get_block_type(block))
        {
            case element_type_formula:
                formula_element_block::resize_block(block, new_size);
            break;
            default:
                mdds::mtv::element_block_func::resize_block(block, new_size);
        }
    }

    inline static void print_block(const mdds::mtv::base_element_block& block)
    {
        switch (mdds::mtv::get_block_type(block))
        {
            case element_type_formula:
                formula_element_block::print_block(block);
            break;
            default:
                mdds::mtv::element_block_func::print_block(block);
        }
    }

    inline static void erase(mdds::mtv::base_element_block& block, size_t pos)
    {
        switch (mdds::mtv::get_block_type(block))
        {
            case element_type_formula:
                formula_element_block::erase_block(block, pos);
            break;
            default:
                mdds::mtv::element_block_func::erase(block, pos);
        }
    }

    inline static void erase(mdds::mtv::base_element_block& block, size_t pos, size_t size)
    {
        switch (mdds::mtv::get_block_type(block))
        {
            case element_type_formula:
                formula_element_block::erase_block(block, pos, size);
            break;
            default:
                mdds::mtv::element_block_func::erase(block, pos, size);
        }
    }

    inline static void append_values_from_block(
        mdds::mtv::base_element_block& dest, const mdds::mtv::base_element_block& src)
    {
        switch (mdds::mtv::get_block_type(dest))
        {
            case element_type_formula:
                formula_element_block::append_values_from_block(dest, src);
            break;
            default:
                mdds::mtv::element_block_func::append_values_from_block(dest, src);
        }
    }

    inline static void append_values_from_block(
        mdds::mtv::base_element_block& dest, const mdds::mtv::base_element_block& src,
        size_t begin_pos, size_t len)
    {
        switch (mdds::mtv::get_block_type(dest))
        {
            case element_type_formula:
                formula_element_block::append_values_from_block(dest, src, begin_pos, len);
            break;
            default:
                mdds::mtv::element_block_func::append_values_from_block(dest, src, begin_pos, len);
        }
    }

    inline static void assign_values_from_block(
        mdds::mtv::base_element_block& dest, const mdds::mtv::base_element_block& src,
        size_t begin_pos, size_t len)
    {
        switch (mdds::mtv::get_block_type(dest))
        {
            case element_type_formula:
                formula_element_block::assign_values_from_block(dest, src, begin_pos, len);
            break;
            default:
                mdds::mtv::element_block_func::assign_values_from_block(dest, src, begin_pos, len);
        }
    }

    inline static bool equal_block(
        const mdds::mtv::base_element_block& left, const mdds::mtv::base_element_block& right)
    {
        if (mdds::mtv::get_block_type(left) == element_type_formula)
        {
            if (mdds::mtv::get_block_type(right) != element_type_formula)
                return false;

            return formula_element_block::get(left) == formula_element_block::get(right);
        }
        else if (mdds::mtv::get_block_type(right) == element_type_formula)
            return false;

        return mdds::mtv::element_block_func::equal_block(left, right);
    }

    inline static void overwrite_values(mdds::mtv::base_element_block& block, size_t pos, size_t len)
    {
        switch (mdds::mtv::get_block_type(block))
        {
            case element_type_formula:
                // Do nothing.  The client code manages the life cycle of these cells.
            break;
            default:
                mdds::mtv::element_block_func::overwrite_values(block, pos, len);
        }
    }
};

}

#endif
