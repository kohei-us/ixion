/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/named_expressions_iterator.hpp"
#include "ixion/global.hpp"
#include "model_types.hpp"
#include "model_context_impl.hpp"

namespace ixion {

struct named_expressions_iterator::impl
{
    const detail::named_expressions_t& named_exps;
    detail::named_expressions_t::const_iterator it;
    detail::named_expressions_t::const_iterator it_end;

    impl(const model_context& cxt, sheet_t scope) :
        named_exps(scope >=0 ? cxt.mp_impl->get_named_expressions(scope) : cxt.mp_impl->get_named_expressions()),
        it(named_exps.cbegin()),
        it_end(named_exps.cend()) {}
};

named_expressions_iterator::named_expressions_iterator(const model_context& cxt, sheet_t scope) :
    mp_impl(ixion::make_unique<impl>(cxt, scope)) {}

named_expressions_iterator::~named_expressions_iterator()
{
}

bool named_expressions_iterator::has() const
{
    return mp_impl->it != mp_impl->it_end;
}

void named_expressions_iterator::next()
{
    ++mp_impl->it;
}

named_expressions_iterator::named_expression named_expressions_iterator::get() const
{
    named_expression ret;
    ret.name = &mp_impl->it->first;
    ret.tokens = mp_impl->it->second.get();
    return ret;
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
