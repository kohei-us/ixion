/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "column_store_type.hpp"
#include "model_types.hpp"

#include <vector>

namespace ixion { namespace detail {

class sheet_store
{
public:
    typedef column_store_t::size_type size_type;

    sheet_store();
    sheet_store(size_type row_size, size_type col_size);
    ~sheet_store();

    column_store_t& operator[](size_type n) { return m_columns[n]; }
    const column_store_t& operator[](size_type n) const { return m_columns[n]; }

    column_store_t& at(size_type n) { return m_columns.at(n); }
    const column_store_t& at(size_type n) const { return m_columns.at(n); }

    column_store_t::iterator& get_pos_hint(size_type n) { return m_pos_hints.at(n); }

    /**
     * Return the number of columns.
     *
     * @return number of columns.
     */
    size_type size() const { return m_columns.size(); }

    const column_stores_t& get_columns() const { return m_columns; }

    detail::named_expressions_t& get_named_expressions() { return m_named_expressions; }
    const detail::named_expressions_t& get_named_expressions() const { return m_named_expressions; }

private:
    column_stores_t m_columns;
    std::vector<column_store_t::iterator> m_pos_hints;
    detail::named_expressions_t m_named_expressions;
};

}}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
