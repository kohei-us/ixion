/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_MODEL_TYPES_HPP
#define INCLUDED_IXION_MODEL_TYPES_HPP

#include <string>
#include <map>
#include <memory>

#include "ixion/formula_tokens.hpp"

namespace ixion {

class formula_cell;

namespace detail {

typedef std::map<std::string, named_expression_t> named_expressions_t;

extern const std::string empty_string;

}}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
