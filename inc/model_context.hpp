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

#ifndef __IXION_MODEL_CONTEXT_HPP__
#define __IXION_MODEL_CONTEXT_HPP__

#include "cell.hpp"

#include <string>
#include <memory>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/noncopyable.hpp>

namespace ixion {

class formula_name_resolver_base;

/**
 * This class stores all data relevant to current session.  You can think of 
 * this like a document model for each formula calculation run.
 */
class model_context : public ::boost::noncopyable
{
    typedef ::boost::ptr_map< ::std::string, formula_cell> named_expressions_t;
public:
    model_context();
    ~model_context();
    
    const formula_name_resolver_base& get_name_resolver() const;

    void set_named_expression(const ::std::string& name, ::std::auto_ptr<formula_cell>& cell);
    formula_cell* get_named_expression(const ::std::string& name);
    const formula_cell* get_named_expression(const ::std::string& name) const;
    const ::std::string* get_named_expression_name(const formula_cell* expr) const;

private:
    formula_name_resolver_base* mp_name_resolver;
    named_expressions_t m_named_expressions;
};

}

#endif
