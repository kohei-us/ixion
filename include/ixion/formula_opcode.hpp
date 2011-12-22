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

#ifndef __IXION_FORMULA_OPCODE_HPP__
#define __IXION_FORMULA_OPCODE_HPP__

namespace ixion {

/** formula opcode type */
enum fopcode_t {
    // data types
    fop_single_ref,
    fop_range_ref,
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
    fop_less,
    fop_greater,

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
