/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_FORMULA_TOKENS_FWD_HPP
#define INCLUDED_IXION_FORMULA_TOKENS_FWD_HPP

#include <boost/intrusive_ptr.hpp>
#include <vector>
#include <memory>

namespace ixion {

class formula_token;
class formula_tokens_store;
using formula_tokens_store_ptr_t = boost::intrusive_ptr<formula_tokens_store>;
using formula_tokens_t = std::vector<formula_token>;
struct named_expression_t;

}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
