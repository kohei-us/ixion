/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/cell_access.hpp"
#include "ixion/global.hpp"
#include "ixion/model_context.hpp"

#include "model_context_impl.hpp"
#include "workbook.hpp"
#include "utils.hpp"

namespace ixion {

struct cell_access::impl
{
    column_store_t::const_position_type pos;
    impl() {}
};

cell_access::cell_access(const model_context& cxt, const abs_address_t& addr) :
    mp_impl(ixion::make_unique<impl>())
{
    mp_impl->pos = cxt.mp_impl->get_cell_position(addr);
}

cell_access::cell_access(cell_access&& other) :
    mp_impl(std::move(other.mp_impl))
{
    other.mp_impl = ixion::make_unique<impl>();
}

cell_access& cell_access::operator= (cell_access&& other)
{
    mp_impl = std::move(other.mp_impl);
    other.mp_impl = ixion::make_unique<impl>();
    return *this;
}

cell_access::~cell_access() {}

celltype_t cell_access::get_type() const
{
    return detail::to_celltype(mp_impl->pos.first->type);
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
