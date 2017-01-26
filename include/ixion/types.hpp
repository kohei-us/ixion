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

namespace ixion {

/** Column index type. */
typedef int col_t;

/** Row index type. */
typedef int row_t;

/** Sheet index type.*/
typedef int sheet_t;

/**
 * String ID type.
 *
 * All string values are converted into integer tokens. You need to call the
 * get_string() method of ixion::iface::formula_model_access to get the
 * actual string value.
 */
typedef unsigned long string_id_t;

/**
 * Special sheet ID that represents a global scope, as opposed to a
 * sheet-local scope.
 */
IXION_DLLPUBLIC_VAR const sheet_t global_scope;

/**
 * Special sheet ID that represents an invalid sheet.
 */
IXION_DLLPUBLIC_VAR const sheet_t invalid_sheet;

/** Global string ID representing an empty string. */
IXION_DLLPUBLIC_VAR const string_id_t empty_string_id;

enum class celltype_t
{
    unknown = 0,
    string,
    numeric,
    formula,
    empty
};

enum value_t
{
    value_none    = 0x00,
    value_string  = 0x01,
    value_numeric = 0x02,
    value_empty   = 0x04
};

/** type that stores a mixture of {@link value_t} values. */
class values_t
{
    int m_val;
public:
    values_t(int val) : m_val(val) {}
    bool is_numeric() const { return (m_val & value_numeric) == value_numeric; }
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

/** type that stores a mixture of {@link table_area_t} values. */
typedef int table_areas_t;

/**
 * Formula name resolver type specifies how name tokens are resolved.
 */
enum class formula_name_resolver_t
{
    unknown    = 0,
    excel_a1   = 1,
    excel_r1c1 = 2,
    calc_a1    = 3,
    odff       = 4
};

/**
 * Formula error types.
 */
enum class formula_error_t
{
    no_error = 0,
    ref_result_not_available = 1,
    division_by_zero = 2,
    invalid_expression = 3,
    stack_error = 4,
    general_error = 5,
};

struct IXION_DLLPUBLIC sheet_size_t
{
    row_t row;
    col_t column;

    sheet_size_t();
    sheet_size_t(const sheet_size_t& other);
    sheet_size_t(row_t _row, col_t _column);
};

IXION_DLLPUBLIC const char* get_formula_error_name(formula_error_t fe);

}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
