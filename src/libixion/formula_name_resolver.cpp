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

#include "ixion/formula_name_resolver.hpp"
#include "ixion/formula_functions.hpp"

#include <iostream>
#include <sstream>

#define DEBUG_NAME_RESOLVER 0

using namespace std;

namespace ixion {

namespace {

bool resolve_function(const string& name, formula_name_type& ret)
{
    formula_function_t func_oc = formula_functions::get_function_opcode(name);
    if (func_oc != func_unknown)
    {
        // This is a built-in function.
        ret.type = formula_name_type::function;
        ret.func_oc = func_oc;
        return true;
    }
    return false;
}

/**
 * Check if the name is a built-in function, or else it's considered a named 
 * expression. 
 * 
 * @param name name to be resolved
 * @param ret resolved name type
 */
void resolve_function_or_name(const string& name, formula_name_type& ret)
{
    if (resolve_function(name, ret))
        return;

    // Everything else is assumed to be a named expression.
    ret.type = formula_name_type::named_expression;
}

enum resolver_parse_mode {
    resolver_parse_column,
    resolver_parse_row
};

void append_column_name_a1(ostringstream& os, col_t col)
{
    const col_t div = 26;
    string col_name;
    while (true)
    {
        col_t rem = col % div;
        char c = 'A' + rem;
        col_name.push_back(c);
        if (col < div)
            break;

        col -= rem;
        col /= div;
        col -= 1;
    }

    reverse(col_name.begin(), col_name.end());
    os << col_name;
}

enum parse_address_result
{
    invalid,
    valid_address,
    range_expected
};

parse_address_result parse_address(
    const char*& p, size_t n, sheet_t& sheet, row_t& row, col_t& col, bool& abs_sheet, bool& abs_row, bool& abs_col)
{
    resolver_parse_mode mode = resolver_parse_column;

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
                return invalid;

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
                    // Leading zeros not allowed.
                    return invalid;

                mode = resolver_parse_row;
            }

            if (row)
                row *= 10;

            row += static_cast<row_t>(c - '0');
        }
        else
            return invalid;
    }
    return valid_address;
}

}

formula_name_type::formula_name_type() : type(invalid) {}

formula_name_resolver_base::formula_name_resolver_base() {}
formula_name_resolver_base::~formula_name_resolver_base() {}

formula_name_resolver_simple::formula_name_resolver_simple() :
    formula_name_resolver_base() {}

formula_name_resolver_simple::~formula_name_resolver_simple() {}

formula_name_type formula_name_resolver_simple::resolve(const string& name, const address_t& pos) const
{
    formula_name_type ret;
    resolve_function_or_name(name, ret);
    return ret;
}

string formula_name_resolver_simple::get_name(const address_t& addr) const
{
    return addr.get_name();
}

formula_name_resolver_a1::~formula_name_resolver_a1() {}

formula_name_type formula_name_resolver_a1::resolve(const string& name, const address_t& pos) const
{
#if DEBUG_NAME_RESOLVER
    cout << "resolve: name=" << name << "; origin=" << pos.get_name() << endl;
#endif
    formula_name_type ret;
    if (resolve_function(name, ret))
        return ret;

    size_t n = name.size();
    if (!n)
        return ret;

    const char* p = &name[0];
    col_t col = 0;
    row_t row = 0;
    sheet_t sheet = 0;
    bool abs_col = false;
    bool abs_row = false;
    bool abs_sheet = false;

    parse_address_result parse_res = 
        parse_address(p, n, sheet, row, col, abs_sheet, abs_row, abs_col);

    if (parse_res == valid_address)
    {
        // Convert column and row from 1-based to 0-based.
        col -= 1;
        row -= 1;

        if (!abs_sheet)
            sheet -= pos.sheet;
        if (!abs_row)
            row -= pos.row;
        if (!abs_col)
            col -= pos.column;

#if DEBUG_NAME_RESOLVER
        string abs_row_s = abs_row ? "abs" : "rel";
        string abs_col_s = abs_col ? "abs" : "rel";
        cout << "resolve: " << name << "=(row=" << row << " [" << abs_row_s << "]; column=" << col << " [" << abs_col_s << "])" << endl;
#endif
        ret.type = formula_name_type::cell_reference;
        ret.address.sheet = sheet;
        ret.address.row = row;
        ret.address.col = col;
        ret.address.abs_sheet = abs_sheet;
        ret.address.abs_row = abs_row;
        ret.address.abs_col = abs_col;
        return ret;
    }

    resolve_function_or_name(name, ret);
    return ret;
}

string formula_name_resolver_a1::get_name(const address_t& addr) const
{
    // For now, sheet index is ignored.
    ostringstream os;
    append_column_name_a1(os, addr.column);
    os << (addr.row + 1);
    return os.str();
}

}
