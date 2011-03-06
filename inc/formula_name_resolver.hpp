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

#ifndef __IXION_FORMULA_NAME_RESOLVER_HPP__
#define __IXION_FORMULA_NAME_RESOLVER_HPP__

#include <string>

namespace ixion {

enum formula_name_type {
    cell_reference,
    range_reference,
    named_expression,
    function,
    invalid
};

class formula_name_resolver_base
{
public:
    virtual ~formula_name_resolver_base() = 0;
    virtual formula_name_type resolve(const ::std::string& name) const = 0;
};

class formula_name_resolver_a1 : public formula_name_resolver_base
{
public:
    virtual ~formula_name_resolver_a1();
    virtual formula_name_type resolve(const::std::string &name) const;
};

}

#endif
