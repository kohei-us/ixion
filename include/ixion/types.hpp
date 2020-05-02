/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_TYPES_HPP
#define INCLUDED_IXION_TYPES_HPP

#include "ixion/env.hpp"

#include <cstdlib>
#include <cstdint>

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
 * get_string() method of ixion::iface::formula_model_access to get the
 * actual string value.
 */
using string_id_t = uint64_t;

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

enum class celltype_t : uint8_t
{
    unknown = 0,
    string,
    numeric,
    formula,
    boolean,
    empty
};

/**
 * Similar to {@link celltype_t}, except that it does not include a formula
 * type, as it infers its type from the formula result. The error type
 * refers to an error value in formula cell.
 */
enum class cell_value_t : uint8_t
{
    unknown = 0,
    string,
    numeric,
    error,
    boolean,
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

/** type that stores a mixture of {@link value_t} values. */
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
    unknown    = 0,
    excel_a1   = 1,  // Default A1 syntax used in Excel
    excel_r1c1 = 2,  // R1C1 syntax available in Excel
    calc_a1    = 3,  // Default A1 syntax used in Calc
    odff       = 4,  // OpenFormula syntax
    odf_cra    = 5,  // ODF cell-range-address syntax
};

/**
 * Formula error types.  Note that only the official (i.e. non-internal)
 * error types have their corresponding error strings.
 *
 * @see ixion::get_formula_error_name
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

    no_result_error          = 253, // internal only error
    stack_error              = 254, // internal only error
    general_error            = 255, // internal only error
};

/**
 * Specifies iterator direction of a model_context.
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
 * formula cells belonging to the same group should return the same set of
 * values.
 */
struct IXION_DLLPUBLIC formula_group_t
{
    rc_size_t size;
    uintptr_t identity;
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
 * @return string representation of the formula error type.
 */
IXION_DLLPUBLIC const char* get_formula_error_name(formula_error_t fe);

}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
