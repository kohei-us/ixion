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

#ifndef __IXION_INTERFACE_MODEL_CONTEXT_HPP__
#define __IXION_INTERFACE_MODEL_CONTEXT_HPP__

#include <string>
#include <boost/noncopyable.hpp>

namespace ixion {

class base_cell;
class formula_cell;
class formula_name_resolver;
class cell_listener_tracker;
class matrix;
struct abs_address_t;
struct abs_range_t;
struct config;

namespace interface {

class cells_in_range;

/**
 * Interface for model context.  The client code needs to provide concrete
 * implementation of this interface in order to provide access to its
 * content.
 */
class model_context : boost::noncopyable
{
public:
    virtual ~model_context() {}

    virtual const config& get_config() const = 0;
    virtual const formula_name_resolver& get_name_resolver() const = 0;
    virtual const base_cell* get_cell(const abs_address_t& addr) const = 0;
    virtual base_cell* get_cell(const abs_address_t& addr) = 0;
    virtual cells_in_range* get_cells_in_range(const abs_range_t& range) const = 0;
    virtual ::std::string get_cell_name(const base_cell* p) const = 0;
    virtual abs_address_t get_cell_position(const base_cell* p) const= 0;
    virtual const formula_cell* get_named_expression(const ::std::string& name) const = 0;
    virtual const ::std::string* get_named_expression_name(const formula_cell* expr) const = 0;

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
    virtual matrix get_range_value(const abs_range_t& range) const = 0;
};

}}

#endif
