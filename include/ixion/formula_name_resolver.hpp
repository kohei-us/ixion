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

#include "ixion/formula_functions.hpp"
#include "ixion/address.hpp"

namespace ixion {

struct formula_name_type
{
    enum name_type {
        cell_reference,
        range_reference,
        named_expression,
        function,
        invalid
    };

    struct address_type {
        sheet_t sheet;
        row_t row;
        col_t col;
        bool abs_sheet:1;
        bool abs_row:1;
        bool abs_col:1;
    };

    struct range_type {
        address_type first;
        address_type last;
    };

    name_type type;
    union {
        address_type address;
        range_type range;
        formula_function_t func_oc; // function opcode
    };

    formula_name_type();

    /**
     * Return a string that represents the data stored internally.  Useful for
     * debugging.
     */
    ::std::string to_string() const;
};

class formula_name_resolver_base
{
public:
    formula_name_resolver_base();
    virtual ~formula_name_resolver_base() = 0;
    virtual formula_name_type resolve(const ::std::string& name, const abs_address_t& pos) const = 0;
    virtual ::std::string get_name(const address_t& addr, const abs_address_t& pos) const = 0;
    virtual ::std::string get_name(const range_t& range, const abs_address_t& pos) const = 0;
    virtual ::std::string get_name(const abs_address_t& addr) const = 0;
    virtual ::std::string get_name(const abs_range_t& range) const = 0;
};

/**
 * Resolve formula expression names by name only.  In other words, all 
 * expressions are named expressions, i.e. no expressions are addressable by
 * cell address syntax. 
 */
class formula_name_resolver_simple : public formula_name_resolver_base
{
public:
    formula_name_resolver_simple();
    virtual ~formula_name_resolver_simple();
    virtual formula_name_type resolve(const::std::string &name, const abs_address_t& pos) const;
    virtual ::std::string get_name(const address_t& addr, const abs_address_t& pos) const;
    virtual ::std::string get_name(const range_t& range, const abs_address_t& pos) const;
    virtual ::std::string get_name(const abs_address_t& addr) const;
    virtual ::std::string get_name(const abs_range_t& range) const;
};

class formula_name_resolver_a1 : public formula_name_resolver_base
{
public:
    virtual ~formula_name_resolver_a1();
    virtual formula_name_type resolve(const::std::string &name, const abs_address_t& pos) const;
    virtual ::std::string get_name(const address_t& addr, const abs_address_t& pos) const;
    virtual ::std::string get_name(const range_t& range, const abs_address_t& pos) const;
    virtual ::std::string get_name(const abs_address_t& addr) const;
    virtual ::std::string get_name(const abs_range_t& range) const;
};

}

#endif
