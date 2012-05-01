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

typedef mdds::gridmap::noncopyable_managed_cell_block<celltype_formula, ixion::formula_cell> formula_cell_block;

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

struct ixion_cell_block_func : public mdds::gridmap::cell_block_func_base
{
    template<typename T>
    static mdds::gridmap::cell_t get_cell_type(const T& cell)
    {
        return mdds::gridmap::get_cell_type(cell);
    }

    template<typename T>
    static void set_value(mdds::gridmap::base_cell_block& block, size_t pos, const T& val)
    {
        mdds::gridmap::set_value(block, pos, val);
    }

    template<typename T>
    static void set_values(mdds::gridmap::base_cell_block& block, size_t pos, const T& it_begin, const T& it_end)
    {
        assert(it_begin != it_end);
        mdds::gridmap::set_values(block, pos, *it_begin, it_begin, it_end);
    }

    template<typename T>
    static void get_value(const mdds::gridmap::base_cell_block& block, size_t pos, T& val)
    {
        mdds::gridmap::get_value(block, pos, val);
    }

    template<typename T>
    static void append_value(mdds::gridmap::base_cell_block& block, const T& val)
    {
        mdds::gridmap::append_value(block, val);
    }

    template<typename T>
    static void insert_values(
        mdds::gridmap::base_cell_block& block, size_t pos, const T& it_begin, const T& it_end)
    {
        assert(it_begin != it_end);
        mdds::gridmap::insert_values(block, pos, *it_begin, it_begin, it_end);
    }

    template<typename T>
    static void append_values(mdds::gridmap::base_cell_block& block, const T& it_begin, const T& it_end)
    {
        assert(it_begin != it_end);
        mdds::gridmap::append_values(block, *it_begin, it_begin, it_end);
    }

    template<typename T>
    static void assign_values(mdds::gridmap::base_cell_block& dest, const T& it_begin, const T& it_end)
    {
        assert(it_begin != it_end);
        mdds::gridmap::assign_values(dest, *it_begin, it_begin, it_end);
    }

    template<typename T>
    static void prepend_value(mdds::gridmap::base_cell_block& block, const T& val)
    {
        mdds::gridmap::prepend_value(block, val);
    }

    template<typename T>
    static void prepend_values(mdds::gridmap::base_cell_block& block, const T& it_begin, const T& it_end)
    {
        assert(it_begin != it_end);
        mdds::gridmap::prepend_values(block, *it_begin, it_begin, it_end);
    }

    template<typename T>
    static void get_empty_value(T& val)
    {
        mdds::gridmap::get_empty_value(val);
    }

    static mdds::gridmap::base_cell_block* create_new_block(
        mdds::gridmap::cell_t type, size_t init_size)
    {
        switch (type)
        {
            case celltype_formula:
                return formula_cell_block::create_block(init_size);
            default:
                ;
        }

        return cell_block_func_base::create_new_block(type, init_size);
    }

    static mdds::gridmap::base_cell_block* clone_block(const mdds::gridmap::base_cell_block& block)
    {
        switch (gridmap::get_block_type(block))
        {
            case celltype_formula:
                // This call throws; formula cell block is non-copyable.
                return formula_cell_block::clone_block(block);
            default:
                ;
        }

        return cell_block_func_base::clone_block(block);
    }

    static void delete_block(mdds::gridmap::base_cell_block* p)
    {
        if (!p)
            return;

        switch (gridmap::get_block_type(*p))
        {
            case celltype_formula:
                formula_cell_block::delete_block(p);
            break;
            default:
                cell_block_func_base::delete_block(p);
        }
    }

    static void resize_block(mdds::gridmap::base_cell_block& block, size_t new_size)
    {
        switch (gridmap::get_block_type(block))
        {
            case celltype_formula:
                formula_cell_block::resize_block(block, new_size);
            break;
            default:
                cell_block_func_base::resize_block(block, new_size);
        }
    }

    static void print_block(const mdds::gridmap::base_cell_block& block)
    {
        switch (gridmap::get_block_type(block))
        {
            case celltype_formula:
                formula_cell_block::print_block(block);
            break;
            default:
                cell_block_func_base::print_block(block);
        }
    }

    static void erase(mdds::gridmap::base_cell_block& block, size_t pos)
    {
        switch (gridmap::get_block_type(block))
        {
            case celltype_formula:
                formula_cell_block::erase_block(block, pos);
            break;
            default:
                cell_block_func_base::erase(block, pos);
        }
    }

    static void erase(mdds::gridmap::base_cell_block& block, size_t pos, size_t size)
    {
        switch (gridmap::get_block_type(block))
        {
            case celltype_formula:
                formula_cell_block::erase_block(block, pos, size);
            break;
            default:
                cell_block_func_base::erase(block, pos, size);
        }
    }

    static void append_values_from_block(
        mdds::gridmap::base_cell_block& dest, const mdds::gridmap::base_cell_block& src)
    {
        switch (gridmap::get_block_type(dest))
        {
            case celltype_formula:
                formula_cell_block::append_values_from_block(dest, src);
            break;
            default:
                cell_block_func_base::append_values_from_block(dest, src);
        }
    }

    static void append_values_from_block(
        mdds::gridmap::base_cell_block& dest, const mdds::gridmap::base_cell_block& src,
        size_t begin_pos, size_t len)
    {
        switch (gridmap::get_block_type(dest))
        {
            case celltype_formula:
                formula_cell_block::append_values_from_block(dest, src, begin_pos, len);
            break;
            default:
                cell_block_func_base::append_values_from_block(dest, src, begin_pos, len);
        }
    }

    static void assign_values_from_block(
        mdds::gridmap::base_cell_block& dest, const mdds::gridmap::base_cell_block& src,
        size_t begin_pos, size_t len)
    {
        switch (gridmap::get_block_type(dest))
        {
            case celltype_formula:
                formula_cell_block::assign_values_from_block(dest, src, begin_pos, len);
            break;
            default:
                cell_block_func_base::assign_values_from_block(dest, src, begin_pos, len);
        }
    }

    static bool equal_block(
        const mdds::gridmap::base_cell_block& left, const mdds::gridmap::base_cell_block& right)
    {
        if (gridmap::get_block_type(left) == celltype_formula)
        {
            if (gridmap::get_block_type(right) != celltype_formula)
                return false;

            return formula_cell_block::get(left) == formula_cell_block::get(right);
        }
        else if (gridmap::get_block_type(right) == celltype_formula)
            return false;

        return cell_block_func_base::equal_block(left, right);
    }

    static void overwrite_cells(mdds::gridmap::base_cell_block& block, size_t pos, size_t len)
    {
        switch (gridmap::get_block_type(block))
        {
            case celltype_formula:
                formula_cell_block::overwrite_cells(block, pos, len);
            break;
            default:
                cell_block_func_base::overwrite_cells(block, pos, len);
        }
    }
};

}}

namespace ixion {

struct grid_map_trait
{
    typedef sheet_t sheet_key_type;
    typedef row_t row_key_type;
    typedef col_t col_key_type;

    typedef mdds::gridmap::ixion_cell_block_func cell_block_func;
};

}

#endif
