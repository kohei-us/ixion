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

IXION_DLLPUBLIC_VAR const string_id_t empty_string_id;

enum celltype_t
{
    celltype_unknown = 0,
    celltype_string,
    celltype_numeric,
    celltype_formula,
    celltype_empty
};

enum table_area_t
{
    table_area_none   = 0x00,
    table_area_data   = 0x01,
    table_area_header = 0x02,
    table_area_totals = 0x04,
    table_area_all    = 0x07
};

/** type that stores a mixture of {@link table_area_t} values. */
typedef int table_areas_t;

/**
 * Formula name resolver type specifies how name tokens are resolved.
 */
enum formula_name_resolver_t
{
    formula_name_resolver_unknown = 0,
    formula_name_resolver_excel_a1,
    formula_name_resolver_calc_a1,
    formula_name_resolver_excel_r1c1,
    formula_name_resolver_odff
};

}

#endif
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
