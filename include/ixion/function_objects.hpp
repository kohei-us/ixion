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

#ifndef __IXION_FUNCTION_OBJECTS_HPP__
#define __IXION_FUNCTION_OBJECTS_HPP__

#include "ixion/types.hpp"

#include <functional>

namespace ixion {

namespace interface {
    class model_context;
}

class cell_listener_tracker;
class dependency_tracker;
class formula_cell;
class formula_token_base;
struct abs_address_t;

class formula_cell_listener_handler : public std::unary_function<formula_token_base*, void>
{
public:
    enum mode_t { mode_add, mode_remove };

    explicit formula_cell_listener_handler(
        interface::model_context& cxt, const abs_address_t& addr, mode_t mode);

    void operator() (const formula_token_base* p) const;

private:
    interface::model_context& m_context;
    cell_listener_tracker& m_listener_tracker;
    const abs_address_t& m_addr;
    formula_cell* mp_cell;
    mode_t m_mode;
};

class cell_dependency_handler : public std::unary_function<formula_cell*, void>
{
public:
    explicit cell_dependency_handler(
        interface::model_context& cxt, dependency_tracker& dep_tracker, dirty_cells_t& dirty_cells);

    void operator() (formula_cell* fcell);

private:
    interface::model_context& m_context;
    dependency_tracker& m_dep_tracker;
    dirty_cells_t& m_dirty_cells;
};

}

#endif
