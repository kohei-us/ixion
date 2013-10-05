/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __IXION_FUNCTION_OBJECTS_HPP__
#define __IXION_FUNCTION_OBJECTS_HPP__

#include "ixion/env.hpp"
#include "ixion/address.hpp"

#include <functional>

namespace ixion {

namespace iface {
    class model_context;
}

class cell_listener_tracker;
class dependency_tracker;
class formula_cell;
class formula_token_base;
struct abs_address_t;

class IXION_DLLPUBLIC formula_cell_listener_handler : public std::unary_function<formula_token_base*, void>
{
public:
    enum mode_t { mode_add, mode_remove };

    explicit formula_cell_listener_handler(
        iface::model_context& cxt, const abs_address_t& addr, mode_t mode);

    void operator() (const formula_token_base* p) const;

private:
    iface::model_context& m_context;
    cell_listener_tracker& m_listener_tracker;
    const abs_address_t& m_addr;
    formula_cell* mp_cell;
    mode_t m_mode;
};

class cell_dependency_handler : public std::unary_function<abs_address_t, void>
{
public:
    explicit cell_dependency_handler(
        iface::model_context& cxt, dependency_tracker& dep_tracker, dirty_formula_cells_t& dirty_cells);

    void operator() (const abs_address_t& fcell);

private:
    iface::model_context& m_context;
    dependency_tracker& m_dep_tracker;
    dirty_formula_cells_t& m_dirty_cells;
};

}

#endif
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
