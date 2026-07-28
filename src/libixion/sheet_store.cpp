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

sheet_store::sheet_store(sheet_store&& other) = default;

sheet_store::sheet_store(size_t row_size, size_t col_size)
{
    for (size_t i = 0; i < col_size; ++i)
        m_columns.emplace_back(row_size);

    m_pos_hints.resize(col_size); // default-constructed hints
}

sheet_store::~sheet_store() = default;

sheet_store sheet_store::clone() const
{
    sheet_store cloned;

    for (const column_store_t& col : m_columns)
        cloned.m_columns.push_back(col.clone());

    cloned.m_pos_hints.resize(m_columns.size()); // default-constructed hints

    // named_expression_t is move-only; reconstruct each entry with a copy of
    // its tokens.
    for (const auto& [name, exp] : m_named_expressions)
        cloned.m_named_expressions.emplace(name, named_expression_t(exp.origin, exp.tokens));

    return cloned;
}

}}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
