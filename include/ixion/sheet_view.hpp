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
struct abs_rc_range_t;

namespace detail {

class model_context_impl;
class sheet_store;

}

/**
 * A named view of a sheet.  A view takes a snapshot of the content of its
 * base sheet when created; later edits to the base sheet do not show up in
 * the view.  The rows of the view can get sorted independently of the base
 * sheet, and the view keeps track of which base row each of its rows shows.
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

    /**
     * Sort the rows of a range of this view in place.  The rows of the range
     * move as units across all of its columns; the cells outside the range
     * never move.  The base sheet stays untouched.
     *
     * The sort is stable, and orders cells of different types as numeric
     * values first, then strings, then false, then true, then error values,
     * with empty cells always last regardless of the direction.  Formula
     * cells sort by their cached results.
     *
     * A later sort, such as by another key column or in the other direction,
     * re-orders the rows as the view currently shows them, and the row
     * mapping reported by to_base_row() and to_view_row() reflects the
     * combined effect of all the sorts.
     *
     * @param range Range to sort.
     * @param keys Sort keys in order of precedence.  Every key column must
     *             lie within the columns of the range.
     *
     * @throw std::invalid_argument When the range does not fit within the
     *        sheet, no keys are given, or a key column lies outside the
     *        range.
     */
    void sort(const abs_rc_range_t& range, const sort_keys_t& keys);

    /**
     * Get the base sheet row that a row of this view shows.
     *
     * @param view_row Row position in this view.
     *
     * @return Row position in the base sheet.
     */
    row_t to_base_row(row_t view_row) const;

    /**
     * Get the row of this view that shows a base sheet row.
     *
     * @param base_row Row position in the base sheet.
     *
     * @return Row position in this view.
     */
    row_t to_view_row(row_t base_row) const;

    /**
     * Sort the data rows of a table by one of its columns.  The header row
     * and the totals rows of the table stay in place; only the rows of its
     * data area move, as units across all the columns of the table.  The
     * base sheet stays untouched.
     *
     * @param table_name Name of the table.  The table must lie on the base
     *                   sheet of this view.
     * @param column Name of the table column to sort by.
     * @param ascending True to sort in ascending order, false to sort in
     *                  descending order.
     *
     * @throw std::invalid_argument When no table of that name exists, the
     *        table lies on another sheet, or the table has no column of that
     *        name.
     */
    void sort_table(std::string_view table_name, std::string_view column, bool ascending);
};

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
