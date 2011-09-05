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

#ifndef __IXION_INTERFACE_FORMULA_HPP__
#define __IXION_INTERFACE_FORMULA_HPP__

#include "ixion/formula_tokens.hpp"

#include <string>

namespace ixion {

/**
 * Parse a raw formula expression string into formula tokens.
 *
 * @param p pointer to the first character of raw formula expression string.
 * @param n size of the raw formula expression string.
 * @param tokens formula tokens representing the parsed formula expression.
 */
void parse_formula_string(const char* p, size_t n, formula_tokens_t& tokens);

/**
 * Convert formula tokens into a human-readable string representation.
 *
 * @param tokens formula tokens.
 *
 * @return string representation of the formula tokens.
 */
std::string print_formula_tokens(const formula_tokens_t& tokens);

}

#endif
