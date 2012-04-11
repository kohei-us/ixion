/*************************************************************************
 *
 * Copyright (c) 2011 Kohei Yoshida
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 ************************************************************************/

#ifndef __IXION_TYPES_HPP__
#define __IXION_TYPES_HPP__

#include "ixion/hash_container/set.hpp"
#include "ixion/env.hpp"

namespace ixion {

typedef int col_t;
typedef int row_t;
typedef int sheet_t;

IXION_DLLPUBLIC extern const sheet_t global_scope;
IXION_DLLPUBLIC extern const sheet_t invalid_sheet;

IXION_DLLPUBLIC extern const size_t empty_string_id;

class formula_cell;

/**
 * Dirty cells are those formula cells that have been modified or formula
 * cells that reference other modified cells.
 */
typedef _ixion_unordered_set_type<formula_cell*> dirty_cells_t;

enum celltype_t
{
    celltype_unknown = 0x0000,
    celltype_string  = 0x0001,
    celltype_numeric = 0x0002,
    celltype_formula = 0x0003,
    celltype_mask    = 0x000F
};

}

#endif
