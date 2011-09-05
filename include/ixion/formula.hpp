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

#ifndef __IXION_FORMULA_HPP__
#define __IXION_FORMULA_HPP__

#include "ixion/formula_tokens.hpp"
#include "ixion/interface/model_context.hpp"

#include <string>

namespace ixion {

/**
 * Parse a raw formula expression string into formula tokens.
 *
 * @param cxt model context.
 * @param p pointer to the first character of raw formula expression string.
 * @param n size of the raw formula expression string.
 * @param pos address of the cell that has the formula expression.
 * @param tokens formula tokens representing the parsed formula expression.
 */
void parse_formula_string(
    const interface::model_context& cxt, const char* p, size_t n, const abs_address_t& pos,
    formula_tokens_t& tokens);

/**
 * Convert formula tokens into a human-readable string representation.
 *
 * @param tokens formula tokens.
 * @param str string representation of the formula tokens.
 */
void print_formula_tokens(
    const interface::model_context& cxt, const formula_tokens_t& tokens, const abs_address_t& pos,
    std::string& str);

}

#endif
