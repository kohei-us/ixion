/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <ixion/global.hpp>
#include "sheet_store.hpp"

namespace ixion { namespace detail {

sheet_store::sheet_store() = default;

sheet_store::sheet_store(size_t row_size, size_t col_size)
{
    m_pos_hints.reserve(col_size);
    for (size_t i = 0; i < col_size; ++i)
    {
        m_columns.emplace_back(row_size);
        m_pos_hints.push_back(m_columns.back().begin());
    }
}

sheet_store::~sheet_store() = default;

}}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
