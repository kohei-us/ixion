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

#include "ixion/env.hpp"

#include <cstdlib>

namespace ixion {

typedef int col_t;
typedef int row_t;
typedef int sheet_t;
typedef unsigned long string_id_t;

IXION_DLLPUBLIC extern const sheet_t global_scope;
IXION_DLLPUBLIC extern const sheet_t invalid_sheet;

IXION_DLLPUBLIC extern const string_id_t empty_string_id;

enum celltype_t
{
    celltype_unknown = 0,
    celltype_string,
    celltype_numeric,
    celltype_formula,
    celltype_empty
};

}

#endif
