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

#include "ixion/formula_tokens_fwd.hpp"
#include "ixion/types.hpp"
#include "ixion/exceptions.hpp"

#include <string>
#include <vector>
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

class model_context_error : public general_error
{
public:
    model_context_error(const std::string& msg) : general_error(msg) {}
};

namespace iface {

class cells_in_range;
class const_cells_in_range;
class session_handler;

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
    virtual cell_listener_tracker& get_cell_listener_tracker() = 0;
    virtual const base_cell* get_cell(const abs_address_t& addr) const = 0;
    virtual base_cell* get_cell(const abs_address_t& addr) = 0;

    virtual bool is_empty(const abs_address_t& addr) const = 0;
    virtual celltype_t get_celltype(const abs_address_t& addr) const = 0;
    virtual double get_numeric_value(const abs_address_t& addr) const = 0;
    virtual const formula_cell* get_formula_cell(const abs_address_t& addr) const = 0;
    virtual formula_cell* get_formula_cell(const abs_address_t& addr) = 0;

    /**
     * Get an iterator that iterates through non-empty cells in a given range.
     *
     * @param range range for which to retrieve the iterator instance.
     *
     * @return pointer to the iterator instance. Note that the caller is
     *         responsible for deleting the instance.
     */
    virtual cells_in_range* get_cells_in_range(const abs_range_t& range) = 0;

    /**
     * Get a const iterator that iterates through non-empty cells in a given
     * range.
     *
     * @param range range for which to retrieve the iterator instance.
     *
     * @return pointer to the const iterator instance. Note that the caller is
     *         responsible for deleting the instance.
     */
    virtual const_cells_in_range* get_cells_in_range(const abs_range_t& range) const = 0;
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

    /**
     * Session handler instance receives various events from the formula
     * interpretation run, in order to respond to those events.  This is
     * optional; the model context implementation is not required to provide a
     * handler.
     *
     * @return non-NULL pointer of the model context provides a session
     *         handler, otherwise a NULL pointer is returned.
     */
    virtual session_handler* get_session_handler() const = 0;

    virtual const formula_tokens_t* get_formula_tokens(sheet_t sheet, size_t identifier) const = 0;
    virtual const formula_tokens_t* get_shared_formula_tokens(sheet_t sheet, size_t identifier) const = 0;
    virtual abs_range_t get_shared_formula_range(sheet_t sheet, size_t identifier) const = 0;

    virtual size_t add_string(const char* p, size_t n) = 0;
    virtual const std::string* get_string(size_t identifier) const = 0;

    /**
     * Get the index of sheet from sheet name.
     *
     * @param p pointer to the first character of the sheet name string.
     * @param n length of the sheet name string.
     *
     * @return sheet index
     */
    virtual sheet_t get_sheet_index(const char* p, size_t n) const = 0;

    virtual std::string get_sheet_name(sheet_t sheet) const = 0;
};

}}

#endif
