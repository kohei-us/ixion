/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <ixion/table.hpp>

#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace ixion { namespace detail {

/**
 * Stores all the tables of a document model, keyed by their names, and
 * resolves table references against them.
 */
class table_store
{
public:
    using store_type = std::map<std::string, table_t, std::less<>>;

    /** Cloned tables paired with the names of their source tables. */
    using cloned_tables_type = std::vector<std::pair<std::string, table_t>>;

    /**
     * Insert a new table.  The table must have a non-empty name unique
     * within the store, a valid sheet index and a valid range.
     *
     * @throw std::invalid_argument When the name is empty, the sheet index
     *        is invalid, or the range is invalid.
     * @throw model_context_error When a table by the same name already
     *        exists in the store.
     */
    void insert(table_t tab);

    const table_t* get(std::string_view name) const;

    /**
     * Get all tables on a specified sheet, sorted by name in ascending
     * order.
     */
    std::vector<const table_t*> get_by_sheet(sheet_t sheet) const;

    /**
     * Resolve a named table reference to the range it references.
     *
     * @return referenced range, or an invalid range when the reference
     *         does not resolve.
     */
    abs_range_t get_range(
        std::string_view name, std::string_view column_first,
        std::string_view column_last, table_areas_t areas) const;

    /**
     * Resolve an unnamed table reference to the range it references.  The
     * position of the referencing cell determines which table the
     * reference is for.
     *
     * @return referenced range, or an invalid range when the reference
     *         does not resolve.
     */
    abs_range_t get_range(
        const abs_address_t& pos, std::string_view column_first,
        std::string_view column_last, table_areas_t areas) const;

    /**
     * Create copies of all the tables on a source sheet, with the copies
     * re-anchored to a destination sheet and their names auto-renamed to
     * next-available unique names. The copies do not get inserted into the
     * store.
     *
     * @return Cloned tables, each paired with the name of the table it was
     *         cloned from.
     */
    cloned_tables_type clone_sheet_tables(sheet_t src, sheet_t dst) const;

private:
    store_type m_tables;
};

}}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
