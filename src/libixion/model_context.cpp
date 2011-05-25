/*************************************************************************
 *
 * Copyright (c) 2011 Kohei Yoshida
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

#include "ixion/model_context.hpp"
#include "ixion/formula_name_resolver.hpp"
#include "ixion/matrix.hpp"

using namespace std;

namespace ixion {

model_context::model_context() :
    mp_name_resolver(new formula_name_resolver_a1)
{}

const formula_name_resolver_base& model_context::get_name_resolver() const
{
    return *mp_name_resolver;
}

void model_context::set_cell(const abs_address_t& addr, auto_ptr<base_cell>& cell)
{
    m_cells.erase(addr);
    m_cells.insert(addr, cell);
}

void model_context::set_cell(const abs_address_t& addr, base_cell* cell)
{
    auto_ptr<base_cell> p(cell);
    set_cell(addr, p);
}

const base_cell* model_context::get_cell(const abs_address_t& addr) const
{
    cell_store_type::const_iterator itr = m_cells.find(addr);
    return itr == m_cells.end() ? NULL : itr->second;
}

base_cell* model_context::get_cell(const abs_address_t& addr)
{
    cell_store_type::iterator itr = m_cells.find(addr);
    return itr == m_cells.end() ? NULL : itr->second;
}

string model_context::get_cell_name(const base_cell* p) const
{
    cell_store_type::const_iterator itr = m_cells.begin(), itr_end = m_cells.end();
    for (; itr != itr_end; ++itr)
    {
        if (itr->second == p)
            return get_name_resolver().get_name(itr->first);
    }

    // Cell not found.  Return an empty string.
    return string();
}

abs_address_t model_context::get_cell_position(const base_cell* p) const
{
    cell_store_type::const_iterator itr = m_cells.begin(), itr_end = m_cells.end();
    for (; itr != itr_end; ++itr)
    {
        if (itr->second == p)
            return itr->first;
    }

    throw general_error("cell instance not found");
}

matrix model_context::get_range_value(const abs_range_t& range) const
{
    if (range.first.sheet != range.last.sheet)
        throw general_error("multi-sheet range is not allowed.");

    size_t rows = range.last.row - range.first.row + 1;
    size_t cols = range.last.column - range.first.column + 1;

    matrix ret(rows, cols);
    for (row_t i = range.first.row; i <= range.last.row; ++i)
    {
        for (col_t j = range.first.column; j <= range.last.column; ++j)
        {
            const base_cell* p = get_cell(abs_address_t(i, j, range.first.sheet));
            if (!p)
                // empty cell.
                continue;

            // TODO: we need to handle string types when that becomes available.
            ret.set(i, j, p->get_value());
        }
    }

    return ret;
}

void model_context::set_named_expression(const string& name, auto_ptr<formula_cell>& cell)
{
    m_named_expressions.insert(name, cell);
}

formula_cell* model_context::get_named_expression(const string& name)
{
    named_expressions_type::iterator itr = m_named_expressions.find(name);
    return itr == m_named_expressions.end() ? NULL : itr->second;
}

const formula_cell* model_context::get_named_expression(const string& name) const
{
    named_expressions_type::const_iterator itr = m_named_expressions.find(name);
    return itr == m_named_expressions.end() ? NULL : itr->second;
}

const string* model_context::get_named_expression_name(const formula_cell* expr) const
{
    named_expressions_type::const_iterator itr = m_named_expressions.begin(), itr_end = m_named_expressions.end();
    for (; itr != itr_end; ++itr)
    {
        if (itr->second == expr)
            return &itr->first;
    }
    return NULL;
}

model_context::~model_context()
{
    delete mp_name_resolver;
}

}
