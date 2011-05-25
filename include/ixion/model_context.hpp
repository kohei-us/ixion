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

#include "ixion/cell.hpp"

#include <string>
#include <memory>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/noncopyable.hpp>

namespace ixion {

class formula_name_resolver_base;
struct abs_address_t;
class matrix;

/**
 * This class stores all data relevant to current session.  You can think of 
 * this like a document model for each formula calculation run.
 */
class model_context : public ::boost::noncopyable
{
    typedef ::boost::ptr_map< ::std::string, formula_cell> named_expressions_type;
    typedef ::boost::ptr_map<abs_address_t, base_cell> cell_store_type;
public:
    model_context();
    ~model_context();
    
    const formula_name_resolver_base& get_name_resolver() const;

    void set_cell(const abs_address_t& addr, ::std::auto_ptr<base_cell>& cell);
    void set_cell(const abs_address_t& addr, base_cell* cell);
    const base_cell* get_cell(const abs_address_t& addr) const;
    base_cell* get_cell(const abs_address_t& addr);
    ::std::string get_cell_name(const base_cell* p) const;
    abs_address_t get_cell_position(const base_cell* p) const;

    /**
     * Obtain range value in matrix form.  Multi-sheet ranges are not 
     * supported.  If the specified range consists of multiple sheets, it 
     * throws an exception. 
     * 
     * @param range absolute, single-sheet range address.  Multi-sheet ranges 
     *              are not allowed.
     * 
     * @return range value represented as matrix.
     */
    matrix get_range_value(const abs_range_t& range) const;

    void set_named_expression(const ::std::string& name, ::std::auto_ptr<formula_cell>& cell);
    formula_cell* get_named_expression(const ::std::string& name);
    const formula_cell* get_named_expression(const ::std::string& name) const;
    const ::std::string* get_named_expression_name(const formula_cell* expr) const;

private:
    formula_name_resolver_base* mp_name_resolver;
    named_expressions_type m_named_expressions;
    cell_store_type m_cells; // TODO: This storage needs to be optimized.
};

}

#endif
