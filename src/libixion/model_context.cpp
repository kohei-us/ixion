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

#include "model_context.hpp"
#include "formula_name_resolver.hpp"

using namespace std;

namespace ixion {

model_context::model_context() :
    mp_name_resolver(new formula_name_resolver_a1)
{}

const formula_name_resolver_base& model_context::get_name_resolver() const
{
    return *mp_name_resolver;
}

void model_context::set_cell(const address_t& addr, auto_ptr<base_cell>& cell)
{
}

const base_cell* model_context::get_cell(const address_t& addr) const
{
    return NULL;
}

base_cell* model_context::get_cell(const address_t& addr)
{
    return NULL;
}

void model_context::set_named_expression(const string& name, auto_ptr<formula_cell>& cell)
{
    m_named_expressions.insert(name, cell);
}

formula_cell* model_context::get_named_expression(const string& name)
{
    named_expressions_t::iterator itr = m_named_expressions.find(name);
    return itr == m_named_expressions.end() ? NULL : itr->second;
}

const formula_cell* model_context::get_named_expression(const string& name) const
{
    named_expressions_t::const_iterator itr = m_named_expressions.find(name);
    return itr == m_named_expressions.end() ? NULL : itr->second;
}

const string* model_context::get_named_expression_name(const formula_cell* expr) const
{
    named_expressions_t::const_iterator itr = m_named_expressions.begin(), itr_end = m_named_expressions.end();
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
