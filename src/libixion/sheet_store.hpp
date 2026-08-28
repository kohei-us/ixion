/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "column_store_type.hpp"
#include "model_types.hpp"

#include <ixion/address.hpp>

#include <vector>

namespace ixion { namespace detail {

class sheet_store
{
public:
    typedef column_store_t::size_type size_type;

    sheet_store();
    sheet_store(sheet_store&& other);
    sheet_store(size_type row_size, size_type col_size);
    ~sheet_store();

    /**
     * Create a copy of this sheet store.
     *
     * This may clone the column stores with copy-on-write enabled.
     *
     * The returned store gets its own fresh set of position hints. The named
     * expressions are copied verbatim i.e. their origins still reference
     * whatever sheet the originals reference.
     */
    sheet_store clone() const;

    column_store_t& operator[](size_type n) { return m_columns[n]; }
    const column_store_t& operator[](size_type n) const { return m_columns[n]; }

    column_store_t& at(size_type n) { return m_columns.at(n); }
    const column_store_t& at(size_type n) const { return m_columns.at(n); }

    mdds::mtv::position_hint& get_pos_hint(size_type n) { return m_pos_hints.at(n); }

    /**
     * Return the number of columns.
     *
     * @return number of columns.
     */
    size_type size() const { return m_columns.size(); }

    const column_stores_t& get_columns() const { return m_columns; }

    /**
     * Get the range that spans all the non-empty cells of the sheet.
     *
     * @return Range spanning the non-empty cells, or an invalid range when
     *         the sheet has no content.
     */
    abs_rc_range_t get_data_range() const;

    detail::named_expressions_t& get_named_expressions() { return m_named_expressions; }
    const detail::named_expressions_t& get_named_expressions() const { return m_named_expressions; }

private:
    column_stores_t m_columns;
    std::vector<mdds::mtv::position_hint> m_pos_hints;
    detail::named_expressions_t m_named_expressions;
};

}}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
