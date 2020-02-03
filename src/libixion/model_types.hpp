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

struct named_expression_t
{
    abs_address_t origin;
    formula_tokens_t tokens;

    named_expression_t(const abs_address_t& _origin, formula_tokens_t _tokens);
};

typedef std::map<std::string, named_expression_t> named_expressions_t;

}}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
