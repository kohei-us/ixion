/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef IXION_DEBUG_HPP
#define IXION_DEBUG_HPP

#include <string>

namespace ixion {

namespace iface { class formula_model_access; }

class formula_cell;
struct abs_address_t;

namespace detail {

std::string print_formula_expression(const iface::formula_model_access& cxt, const abs_address_t& pos, const formula_cell& cell);

}}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
