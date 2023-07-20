/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_TYPES_HPP
#define INCLUDED_IXION_TYPES_HPP

#include "env.hpp"

#include <cstdlib>
#include <cstdint>
#include <string_view>
#include <functional>

namespace ixion {

/** Column index type. */
using col_t = int32_t;

/** Row index type. */
using row_t = int32_t;

/** Sheet index type.*/
using sheet_t = int32_t;

/**
 * Integer type that is large enough to store either a row or a column
 * index.
 */
using rc_t = row_t;

/**
 * String ID type.
 *
 * All string values are converted into integer tokens. You need to call the
 * get_string() method of ixion::model_context to get the
 * actual string value.
 */
using string_id_t = uint32_t;

/**
 * Special sheet ID that represents a global scope, as opposed to a
 * sheet-local scope.
 */
IXION_DLLPUBLIC_VAR const sheet_t global_scope;

/**
 * Special sheet ID that represents an invalid sheet.
 */
IXION_DLLPUBLIC_VAR const sheet_t invalid_sheet;

/**
 * Determine whether or not a given sheet index is valid.
 *
 * @param sheet sheet index to test.
 *
 * @return true if the sheet index is valid, false otherwise.
 */
IXION_DLLPUBLIC bool is_valid_sheet(sheet_t sheet);

/** Global string ID representing an empty string. */
IXION_DLLPUBLIC_VAR const string_id_t empty_string_id;

/**
 * This type represents a raw cell type as stored in ixion::model_context.
 */
enum class celltype_t : uint8_t
{
    /** unknown cell type.*/
    unknown = 0,
    /** cell contains a raw string value.*/
    string,
    /** cell contains a raw numeric value.*/
    numeric,
    /** cell contains a formula object. */
    formula,
    /** cell contains a raw boolean value. */
    boolean,
    /** cell is empty and contains absolutely nothing. */
    empty
};

/**
 * Similar to celltype_t, except that it does not include a formula type.
 * Instead it uses the formula result type to classify its type. The error
 * type refers to an error value in formula cell.
 */
enum class cell_value_t : uint8_t
{
    /** unknown cell value type. */
    unknown = 0,
    /**
     * either the cell contains a raw string value, or a calculated formula
     * cell whose result is of string type.
     */
    string,
    /**
     * either the cell contains a raw numeric value, or a calculated formula
     * cell whose result is of numeric type.
     */
    numeric,
    /**
     * this type corresponds with a formula cell whose result contains an
     * error.
     */
    error,
    /**
     * either the cell contains a raw boolean value type, or a calculated
     * formula cell whose result is of boolean type.
     */
    boolean,
    /**
     * the cell is empty and contains nothing whatsoever.
     */
    empty
};

enum value_t
{
    value_none    = 0x00,
    value_string  = 0x01,
    value_numeric = 0x02,
    value_boolean = 0x04,
    value_empty   = 0x08
};

/** type that stores a mixture of value_t values. */
class values_t
{
    int m_val;
public:
    values_t(int val) : m_val(val) {}
    bool is_numeric() const { return (m_val & value_numeric) == value_numeric; }
    bool is_boolean() const { return (m_val & value_boolean) == value_boolean; }
    bool is_string() const { return (m_val & value_string) == value_string; }
    bool is_empty() const { return (m_val & value_empty) == value_empty; }
};

/** Value that specifies the area inside a table. */
enum table_area_t
{
    table_area_none    = 0x00,
    table_area_data    = 0x01,
    table_area_headers = 0x02,
    table_area_totals  = 0x04,
    table_area_all     = 0x07
};

/** type that stores a mixture of ixion::table_area_t values. */
using table_areas_t = int32_t;

/**
 * Formula name resolver type specifies how name tokens are resolved.
 */
enum class formula_name_resolver_t
{
    /** Unknown syntax. */
    unknown    = 0,
    /** Default A1 syntax used in Excel */
    excel_a1   = 1,
    /** R1C1 syntax available in Excel */
    excel_r1c1 = 2,
    /** Default A1 syntax used in Calc */
    calc_a1    = 3,
    /** OpenFormula syntax */
    odff       = 4,
    /** ODF cell-range-address syntax */
    odf_cra    = 5,
};

/**
 * Formula error types.  Note that only the official (i.e. non-internal)
 * error types have their corresponding error strings.  Use the
 * get_formula_error_name() function to convert an enum member value of this
 * type to its string representation.
 */
enum class formula_error_t : uint8_t
{
    no_error                 = 0,
    ref_result_not_available = 1,
    division_by_zero         = 2,
    invalid_expression       = 3,
    name_not_found           = 4,
    no_range_intersection    = 5,
    invalid_value_type       = 6,
    no_value_available       = 7,

    no_result_error          = 253, // internal only error
    stack_error              = 254, // internal only error
    general_error            = 255, // internal only error
};

/**
 * Type of policy on what to do when querying for the result of a formula
 * cell whose result has not yet been calculated.
 */
enum class formula_result_wait_policy_t
{
    /**
     * Querying for the result of a formula cell will block until the
     * calculation is complete.
     */
    block_until_done,

    /**
     * Querying for the result of a formula cell that has not yet been
     * calculated will throw an exception.
     */
    throw_exception,
};

/**
 * Formula event type used for event notification during calculation of
 * formula cells.
 *
 * @see ixion::model_context::notify
 */
enum class formula_event_t
{
    /** Start of the calculations of formula cells. */
    calculation_begins,
    /** End of the calculations of formula cells. */
    calculation_ends,
};

/**
 * Specifies iterator direction of a ixion::model_context.
 */
enum class rc_direction_t
{
    /** Flows left to right first then top to bottom. */
    horizontal,
    /** Flows top to bottom first then left to right. */
    vertical
};

/**
 * This structure stores a 2-dimensional size information.
 */
struct IXION_DLLPUBLIC rc_size_t
{
    row_t row;
    col_t column;

    rc_size_t();
    rc_size_t(const rc_size_t& other);
    rc_size_t(row_t _row, col_t _column);
    ~rc_size_t();

    rc_size_t& operator= (const rc_size_t& other);
};

/**
 * This strcuture stores information about grouped formula cells.  All
 * formula cells belonging to the same group shares the same set of values.
 */
struct IXION_DLLPUBLIC formula_group_t
{
    /** Size of the formula group. */
    rc_size_t size;
    /**
     * Unique value identifying the group a cell belongs to.  Cells belonging
     * to the same formula group should have the same value.
     */
    uintptr_t identity;
    /** Boolean value indicating whether or not a cell is grouped.   */
    bool grouped;

    formula_group_t();
    formula_group_t(const formula_group_t& r);
    formula_group_t(const rc_size_t& _group_size, uintptr_t _identity, bool _grouped);
    ~formula_group_t();

    formula_group_t& operator= (const formula_group_t& other);
};

/**
 * Get a string representation of a formula error type.
 *
 * @param fe enum value representing a formula error type.
 * @return null-terminated string representation of the formula error type.
 */
IXION_DLLPUBLIC std::string_view get_formula_error_name(formula_error_t fe);

/**
 * Parse a formula error string and convert it to a corresponding enum value.
 *
 * @param s string representation of a formula error type.
 * @return enum value for a formula error type.
 */
IXION_DLLPUBLIC formula_error_t to_formula_error_type(std::string_view s);

using column_block_handle = void*;

/**
 * Type of a column block that stores a series of adjacent cell values of the
 * same type.
 */
enum class column_block_t : int
{
    unknown = 0,
    empty,
    boolean,
    numeric,
    string,
    formula
};

/**
 * Data that represents the shape of a column block.
 */
struct IXION_DLLPUBLIC column_block_shape_t
{
    std::size_t position;
    std::size_t size;
    std::size_t offset;
    column_block_t type;
    column_block_handle data;

    column_block_shape_t();

    column_block_shape_t(
        std::size_t _position, std::size_t _size, std::size_t _offset,
        column_block_t _type, column_block_handle _data);

    column_block_shape_t(const column_block_shape_t& other);

    column_block_shape_t& operator=(const column_block_shape_t& other);
};

/**
 * Callback function type to be used during traversal of column data.
 */
using column_block_callback_t = std::function<bool(col_t, row_t, row_t, const column_block_shape_t&)>;

IXION_DLLPUBLIC std::ostream& operator<< (std::ostream& os, const column_block_shape_t& v);

/**
 * Specifies how to determine whether or not to display a sheet name of a cell
 * or range reference.
 */
enum class display_sheet_t
{
    /** Sheet name display preference is not specified. */
    unspecified,
    /** Sheet name should be always displayed. */
    always,
    /** Sheet name should never be displayed. */
    never,
    /**
     * Sheet name should be displayed only when the sheet index of a reference
     * is different from that of the position of a referencing cell.
     */
    only_if_different,
};

struct IXION_DLLPUBLIC print_config
{
    display_sheet_t display_sheet = display_sheet_t::only_if_different;

    print_config();
    print_config(const print_config& other);
    ~print_config();
};

} // namespace ixion

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
