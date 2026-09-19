/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once
#include "types.hpp"
#include "address.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <variant>

namespace ixion {

class cell_access;
class model_context;

/**
 * Higher level document representation designed to handle both cell value
 * storage as well as formula cell calculations.
 */
class IXION_DLLPUBLIC document
{
    struct impl;
    std::unique_ptr<impl> mp_impl;
public:
    /**
     * Constructor.  Cell addresses given as strings, as well as formula
     * expressions, use the Excel A1 syntax.
     */
    document();

    /**
     * Constructor with custom cell address type.
     *
     * @param cell_address_type cell address type to use for cell addresses
     *                          represented by string values, as well as for
     *                          formula expressions.
     */
    document(formula_name_resolver_t cell_address_type);
    ~document();

    /**
     * Position of a cell, given either as a string in the cell address
     * syntax the document was constructed with, such as "Sheet1!A1", or as
     * an absolute address.  A string without a sheet name refers to the
     * first sheet.
     */
    struct IXION_DLLPUBLIC cell_pos
    {
        /** How the position is given. */
        enum class cp_type { string, address };
        /** How the position is given. */
        cp_type type;

        /** The position, as a string or as an address depending on type. */
        std::variant<std::string_view, ixion::abs_address_t> value;

        cell_pos() = delete;
        /** Position as a null-terminated string. */
        cell_pos(const char* p);
        /** Position as a string. */
        cell_pos(std::string_view s);
        /** Position as a string. */
        cell_pos(const std::string& s);
        /** Position as an absolute address. */
        cell_pos(const abs_address_t& addr);
        /** Copy constructor. */
        cell_pos(const cell_pos& other);

        /** Copy assignment. */
        cell_pos& operator=(const cell_pos& other);
    };

    /**
     * Append a new sheet to the document.
     *
     * @param name Name of the sheet.  It must be unique within the document.
     */
    void append_sheet(std::string name);

    /**
     * Append a new sheet to the document as a copy of an existing sheet.
     * All cells of the source sheet get copied over to the new sheet, and
     * the calculation results of the copied formula cells carry over as
     * well.  Those formula cells whose carried-over results may no longer
     * be valid on the new sheet get marked for re-calculation, which the
     * next calculate() call picks up.
     *
     * @param src Index of the sheet to copy.
     * @param name Name of the sheet to be inserted.  It must be unique
     *             within the document, else a model_context_error exception
     *             gets thrown.
     *
     * @return Sheet index of the inserted sheet.
     *
     * @throw model_context_error When the sheet name already exists.
     * @throw std::invalid_argument When the source sheet index is invalid.
     */
    sheet_t append_sheet_copy(sheet_t src, std::string name);

    /**
     * Set a new name to an existing sheet.
     *
     * @param sheet 0-based sheet index.
     * @param name New name of a sheet.
     */
    void set_sheet_name(sheet_t sheet, std::string name);

    /**
     * Get read-only access to the underlying model context, for instance to
     * dump the content of a sheet or to iterate over its cells.
     *
     * @return Model context this document stores its cells in.
     */
    const model_context& get_model_context() const;

    /**
     * Get an accessor for a cell, for repeated queries on the same cell.
     *
     * @param pos Position of the cell.
     *
     * @return Accessor for the cell.
     */
    cell_access get_cell_access(const cell_pos& pos) const;

    /**
     * Set a numeric value to a cell, replacing its current content.  The
     * formula cells depending on the cell get re-calculated by the next
     * calculate() call.
     *
     * @param pos Position of the cell.
     * @param val Numeric value.
     */
    void set_numeric_cell(const cell_pos& pos, double val);

    /**
     * Set a string value to a cell, replacing its current content.  The
     * formula cells depending on the cell get re-calculated by the next
     * calculate() call.
     *
     * @param pos Position of the cell.
     * @param s String value.
     */
    void set_string_cell(const cell_pos& pos, std::string_view s);

    /**
     * Set a boolean value to a cell, replacing its current content.  The
     * formula cells depending on the cell get re-calculated by the next
     * calculate() call.
     *
     * @param pos Position of the cell.
     * @param val Boolean value.
     */
    void set_boolean_cell(const cell_pos& pos, bool val);

    /**
     * Empty a cell, discarding whatever value it holds.  The formula cells
     * depending on the cell get re-calculated by the next calculate() call.
     *
     * @param pos Position of the cell.
     */
    void empty_cell(const cell_pos& pos);

    /**
     * Get a numeric representation of the cell value.  For a formula cell,
     * this is the numeric value of its result.
     *
     * @param pos Position of the cell.
     *
     * @return Numeric representation of the cell value.
     */
    double get_numeric_value(const cell_pos& pos) const;

    /**
     * Get the string value of a cell.  Only a string cell, or a formula cell
     * with a string result, has one.
     *
     * @param pos Position of the cell.
     *
     * @return String value of the cell, or an empty view if the cell holds
     *         no string.
     */
    std::string_view get_string_value(const cell_pos& pos) const;

    /**
     * Set a formula to a cell, replacing its current content.  The formula
     * gets parsed in the cell address syntax the document was constructed
     * with, and the cell gets calculated by the next calculate() call.
     *
     * @param pos Position of the cell.
     * @param formula Formula expression, without a leading '='.
     */
    void set_formula_cell(const cell_pos& pos, std::string_view formula);

    /**
     * Calculate all the "dirty" formula cells in the document.
     *
     * @param thread_count number of threads to use to perform calculation.
     *                     When 0 is specified, it only uses the main thread.
     */
    void calculate(size_t thread_count);
};

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
