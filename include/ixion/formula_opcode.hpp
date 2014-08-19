/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __IXION_FORMULA_OPCODE_HPP__
#define __IXION_FORMULA_OPCODE_HPP__

namespace ixion {

/** formula opcode type */
enum fopcode_t {
    // data types
    fop_single_ref,
    fop_range_ref,
    fop_table_ref,
    fop_named_expression,
    fop_string,
    fop_value,
    fop_function,

    // arithmetic operators
    fop_plus,
    fop_minus,
    fop_divide,
    fop_multiply,

    // relational operators
    fop_equal,
    fop_not_equal,
    fop_less,
    fop_greater,
    fop_less_equal,
    fop_greater_equal,

    // parentheses, separators
    fop_open,
    fop_close,
    fop_sep,

    // error conditions
    fop_err_no_ref,

    fop_unknown
};

}

#endif
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
