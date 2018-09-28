/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __IXION_FUNCTION_OBJECTS_HPP__
#define __IXION_FUNCTION_OBJECTS_HPP__

#include "ixion/address.hpp"

#include <functional>

namespace ixion {

namespace iface {

class formula_model_access;

}

class dependency_tracker;
class formula_cell;
class formula_token;
struct abs_address_t;

class cell_dependency_handler : public std::unary_function<abs_address_t, void>
{
public:
    explicit cell_dependency_handler(
        iface::formula_model_access& cxt, dependency_tracker& dep_tracker, abs_address_set_t& dirty_cells);

    void operator() (const abs_address_t& fcell);

private:
    iface::formula_model_access& m_context;
    dependency_tracker& m_dep_tracker;
    abs_address_set_t& m_dirty_cells;
};

}

#endif
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
