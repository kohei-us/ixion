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

namespace ixion {

class model_context;
class formula_cell;
class formula_result;
struct abs_address_t;

/**
 * This class provides a read-only access to a single cell.  It's more
 * efficient to use this class if you need to make multiple successive
 * queries to the same cell.
 *
 * Note that an instance of this class will get invalidated when the content
 * of ixion::model_context is modified.
 */
class IXION_DLLPUBLIC cell_access
{
    friend class model_context;

    struct impl;
    std::unique_ptr<impl> mp_impl;

    cell_access(const model_context& cxt, const abs_address_t& addr);
public:
    /** Move constructor.  The moved-from instance no longer refers to a cell. */
    cell_access(cell_access&& other);
    /** Move assignment.  The moved-from instance no longer refers to a cell. */
    cell_access& operator= (cell_access&& other);
    ~cell_access();

    /**
     * Get the type of the cell.  A formula cell is reported as a formula
     * cell regardless of its result.
     *
     * @return Type of the cell.
     */
    cell_t get_type() const;

    /**
     * Get the type of the value the cell holds.  Unlike get_type(), a formula
     * cell is classified by the type of its result, which may be an error.
     *
     * @return Type of the cell value.
     */
    cell_value_t get_value_type() const;

    /**
     * Get the formula cell.
     *
     * @return Pointer to the formula cell, or nullptr if the cell is not a
     *         formula cell.
     */
    const formula_cell* get_formula_cell() const;

    /**
     * Get the cached result of the formula cell.  For a grouped formula
     * cell, the result is the single value assigned to the position of the
     * cell within its group.
     *
     * @return Cached result of the formula cell.
     *
     * @throw general_error If the cell is not a formula cell.
     *
     * @note A non-grouped formula cell whose result is a matrix gets the
     *       matrix back as is.
     */
    formula_result get_formula_result() const;

    /**
     * Get a numeric representation of the cell value.  A boolean cell yields
     * 1 or 0, and a string or empty cell yields 0.  For a formula cell, this
     * is the numeric value of its result.
     *
     * @return Numeric representation of the cell value.
     *
     * @throw formula_error If the cell is a formula cell whose result is an
     *                      error or cannot be converted to a number.
     *
     * @note When the result is a matrix, the value of one element is returned:
     *       the element at the cell's position within its group for a grouped
     *       formula cell, or the top-left element for a non-grouped formula
     *       cell.
     */
    double get_numeric_value() const;

    /**
     * Get a boolean representation of the cell value.  A numeric value, or
     * the numeric result of a formula cell, is true when it is not zero.  A
     * string or empty cell is false.
     *
     * @return Boolean representation of the cell value.
     *
     * @throw formula_error If the cell is a formula cell whose result is an
     *                      error or cannot be converted to a number.
     */
    bool get_boolean_value() const;

    /**
     * Get the string value of the cell.  A string cell yields its text
     * whether the string is stored inline or by its identifier in the
     * indexed string pool, and a formula cell yields its string result.
     *
     * @return String value of the cell, or an empty view if the cell is
     *         neither a string cell nor a formula cell.
     *
     * @throw formula_error If the cell is a formula cell whose result is an
     *                      error or not a string.
     *
     * @note When the result is a matrix, the value of one element is returned:
     *       the element at the cell's position within its group for a grouped
     *       formula cell, or the top-left element for a non-grouped formula
     *       cell.
     */
    std::string_view get_string_value() const;

    /**
     * Get the string identifier of the cell.  Only a cell whose string was
     * set by its identifier, via the string_id_t overload of
     * model_context::set_string_cell(), has one.
     *
     * @return String identifier of the cell, or empty_string_id.
     */
    string_id_t get_string_identifier() const;

    /**
     * Get the error value of the cell.  Only a formula cell whose result is
     * an error has one.
     *
     * @return Error value of the cell, or formula_error_t::no_error if the
     *         cell is not a formula cell or its result is not an error.
     */
    formula_error_t get_error_value() const;
};

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
