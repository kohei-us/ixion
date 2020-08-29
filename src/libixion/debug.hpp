/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_DEBUG_HPP
#define INCLUDED_IXION_DEBUG_HPP

#include <string>

#if defined(IXION_DEBUG_ON) || defined(IXION_TRACE_ON)
#include <iostream>
#endif

#ifdef IXION_DEBUG_ON
#define IXION_DEBUG(stream) \
    do { std::cout << "[DEBUG]" << __FUNCTION__ << ": " << stream << std::endl; } while (false)
#else
#define IXION_DEBUG(...)
#endif

#ifdef IXION_TRACE_ON
#define IXION_TRACE(stream) \
    do { std::cout << "[TRACE]" << __FUNCTION__ << ": " << stream << std::endl; } while (false)
#else
#define IXION_TRACE(...)
#endif

namespace ixion {

namespace iface { class formula_model_access; }

class formula_cell;
class formula_token;
struct abs_address_t;

namespace detail {

std::string print_formula_expression(const iface::formula_model_access& cxt, const abs_address_t& pos, const formula_cell& cell);
std::string print_formula_token_repr(const formula_token& t);

}}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
