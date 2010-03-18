/*************************************************************************
 *
 * Copyright (c) 2010 Kohei Yoshida
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

#ifndef __FORMULA_TOKENS_HPP__
#define __FORMULA_TOKENS_HPP__

#include <boost/ptr_container/ptr_vector.hpp>

namespace ixion {

class formula_token_base;

typedef ::boost::ptr_vector<formula_token_base> formula_tokens_t;

const char* print_tokens(const formula_tokens_t& tokens, bool verbose);

class formula_token_base
{
};

// ============================================================================

// We need the following inline functions for boost::ptr_container.

inline formula_token_base* new_clone(const formula_token_base& r)
{
    return NULL;
}

inline void delete_clone(const formula_token_base* p)
{
    delete p;
}

// ============================================================================

}

#endif
