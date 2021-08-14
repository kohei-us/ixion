/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_DETAIL_UTILS_HPP
#define INCLUDED_IXION_DETAIL_UTILS_HPP

#include "ixion/types.hpp"
#include "column_store_type.hpp"

#include <sstream>

namespace ixion { namespace detail {

celltype_t to_celltype(mdds::mtv::element_t mtv_type);

template<std::size_t S, typename T>
void ensure_max_size(const T& v)
{
    static_assert(sizeof(T) <= S, "The size of the value exceeded allowed size limit.");
}

}}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
