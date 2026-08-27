/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "types.hpp"

#include <memory>
#include <string>
#include <string_view>

namespace ixion {

class formula_cell;
struct abs_rc_address_t;

namespace detail {

class model_context_impl;
class sheet_store;

}

/**
 * A named, read-only view of a sheet.  A view takes a snapshot of the
 * content of its base sheet when created; later edits to the base sheet do
 * not show up in the view.
 *
 * Views get created and owned by model_context via its create_sheet_view()
 * method, and stay valid until removed or until the model context gets
 * destroyed.
 */
class IXION_DLLPUBLIC sheet_view
{
    friend class detail::model_context_impl;

    struct impl;
    std::unique_ptr<impl> mp_impl;

    sheet_view(
        const detail::model_context_impl& cxt, sheet_t sheet, std::string name,
        const detail::sheet_store& base);

public:
    sheet_view(const sheet_view&) = delete;
    sheet_view& operator=(const sheet_view&) = delete;
    ~sheet_view();

    /**
     * @return Index of the base sheet this view was created from.
     */
    sheet_t get_sheet() const;

    /**
     * @return Name of this view, unique among the views of its base sheet.
     */
    std::string_view get_name() const;

    cell_t get_celltype(const abs_rc_address_t& pos) const;

    /**
     * Get a numeric representation of the cell value at the specified
     * position of the view.  A formula cell yields its cached result.
     *
     * @param pos Position of the cell.
     *
     * @return Numeric representation of the cell value.
     */
    double get_numeric_value(const abs_rc_address_t& pos) const;

    bool get_boolean_value(const abs_rc_address_t& pos) const;

    /**
     * Get the string value of the cell at the specified position of the
     * view.  It returns a valid string only when the cell is a string cell,
     * or a formula cell with a cached string result.
     *
     * @param pos Position of the cell.
     *
     * @return String value of the cell, or an empty string if the cell has
     *         no string value.
     */
    std::string_view get_string_value(const abs_rc_address_t& pos) const;

    /**
     * @return Formula cell at the specified position of the view, or nullptr
     *         if the cell is not a formula cell.
     */
    const formula_cell* get_formula_cell(const abs_rc_address_t& pos) const;
};

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
