/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "debug.hpp"
#include "ixion/formula_name_resolver.hpp"
#include "ixion/formula_tokens.hpp"
#include "ixion/cell.hpp"
#include "ixion/formula.hpp"

#include <sstream>

namespace ixion { namespace detail {

std::string print_formula_expression(const iface::formula_model_access& cxt, const abs_address_t& pos, const formula_cell& cell)
{
    auto resolver = formula_name_resolver::get(formula_name_resolver_t::excel_a1, &cxt);
    assert(resolver);
    const formula_tokens_t& tokens = cell.get_tokens()->get();
    return print_formula_tokens(cxt, pos, *resolver, tokens);
}

std::string print_formula_token_repr(const formula_token& t)
{
    std::ostringstream os;
    os << t;
    return os.str();
}

}}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
