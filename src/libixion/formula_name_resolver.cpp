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

#include "formula_name_resolver.hpp"
#include "formula_functions.hpp"

#include <iostream>

using namespace std;

namespace ixion {

namespace {

/**
 * Check if the name is a built-in function, or else it's considered a named 
 * expression. 
 * 
 * @param name name to be resolved
 * @param ret resolved name type
 */
void resolve_function_or_name(const string& name, formula_name_type& ret)
{
    formula_function_t func_oc = formula_functions::get_function_opcode(name);
    if (func_oc != func_unknown)
    {
        // This is a built-in function.
        ret.type = formula_name_type::function;
        ret.func_oc = func_oc;
        return;
    }

    // Everything else is assumed to be a named expression.
    ret.type = formula_name_type::named_expression;
}

enum resolver_parse_mode {
    resolver_parse_column,
    resolver_parse_row
};

}

formula_name_type::formula_name_type() : type(invalid) {}

formula_name_resolver_base::formula_name_resolver_base() {}
formula_name_resolver_base::~formula_name_resolver_base() {}

formula_name_resolver_simple::formula_name_resolver_simple() :
    formula_name_resolver_base() {}

formula_name_resolver_simple::~formula_name_resolver_simple() {}

formula_name_type formula_name_resolver_simple::resolve(const string& name) const
{
    formula_name_type ret;
    resolve_function_or_name(name, ret);
    return ret;
}

formula_name_resolver_a1::~formula_name_resolver_a1() {}

formula_name_type formula_name_resolver_a1::resolve(const string& name) const
{
    formula_name_type ret;
    resolver_parse_mode mode = resolver_parse_column;

    size_t n = name.size();
    if (!n)
        return ret;

    const char* p = &name[0];
    col_t col = 0;
    row_t row = 0;
    sheet_t sheet = 0;
    bool valid_ref = true;
    for (size_t i = 0; i < n; ++i, ++p)
    {
        char c = *p;
        if ('a' <= c && c <= 'z')
        {
            // Convert to upper case.
            c -= 'a' - 'A';
        }

        if ('A' <= c && c <= 'Z')
        {
            // Column digit
            if (mode != resolver_parse_column)
            {
                valid_ref = false;
                break;
            }

            if (col)
                col *= 26;
            col += static_cast<col_t>(c - 'A' + 1);
        }
        else if ('0' <= c && c <= '9')
        {
            if (mode == resolver_parse_column)
            {
                // First digit of a row.
                if (c == '0')
                {
                    // Leading zeros not allowed.
                    valid_ref = false;
                    break;
                }
                mode = resolver_parse_row;
            }

            if (row)
                row *= 10;

            row += static_cast<row_t>(c - '0');
        }
        else
        {
            valid_ref = false;
            break;
        }
    }

    if (valid_ref)
    {
        // This is a valid A1 reference.
        col -= 1;
        row -= 1;
        cout << name << ": (col=" << col << ",row=" << row << ")" << endl;
        ret.type = formula_name_type::cell_reference;
        ret.address.col = col;
        ret.address.row = row;
        ret.address.sheet = sheet;
        return ret;
    }

    resolve_function_or_name(name, ret);
    return ret;
}

}
