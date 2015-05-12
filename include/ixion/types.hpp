/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __IXION_TYPES_HPP__
#define __IXION_TYPES_HPP__

#include "ixion/env.hpp"

#include <cstdlib>

namespace ixion {

typedef int col_t;
typedef int row_t;
typedef int sheet_t;
typedef unsigned long string_id_t;

IXION_DLLPUBLIC_VAR const sheet_t global_scope;
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

}

#endif
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
