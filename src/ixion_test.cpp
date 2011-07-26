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
#include "ixion/address.hpp"

#include <iostream>
#include <cassert>
#include <string>

using namespace std;
using namespace ixion;

void test_name_resolver()
{
    cout << "test name resolver" << endl;

    formula_name_resolver_a1 resolver;

    // Parse single cell addresses.
    const char* names[] = {
        "A1", "Z1", "AA23", "AB23", "BA1", "AAA2", "ABA1", "BAA1", 0
    };

    for (size_t i = 0; names[i]; ++i)
    {
        const char* p = names[i];
        string name_a1(p);
        formula_name_type res = resolver.resolve(name_a1, abs_address_t());
        assert(res.type == formula_name_type::cell_reference);
        address_t addr;
        addr.sheet = res.address.sheet;
        addr.row = res.address.row;
        addr.column = res.address.col;
        string test_name = resolver.get_name(addr);
        assert(name_a1 == test_name);
    }

    // Parse range addresses.
    struct {
        const char* name; sheet_t sheet1; row_t row1; col_t col1; sheet_t sheet2; row_t row2; col_t col2;
    } range_tests[] = {
        { "A1:B2", 0, 0, 0, 0, 1, 1 },
        { "D10:G24", 0, 9, 3, 0, 23, 6 },
        { 0, 0, 0, 0, 0, 0, 0 }
    };

    for (size_t i = 0; range_tests[i].name; ++i)
    {
        string name_a1(range_tests[i].name);
        formula_name_type res = resolver.resolve(name_a1, abs_address_t());
        assert(res.type == formula_name_type::range_reference);
        assert(res.range.first.sheet == range_tests[i].sheet1);
        assert(res.range.first.row == range_tests[i].row1);
        assert(res.range.first.col == range_tests[i].col1);
        assert(res.range.last.sheet == range_tests[i].sheet2);
        assert(res.range.last.row == range_tests[i].row2);
        assert(res.range.last.col == range_tests[i].col2);
    }

    formula_name_type res = resolver.resolve("B1", abs_address_t(0,1,1));
    assert(res.type == formula_name_type::cell_reference);
    assert(res.address.sheet == 0);
    assert(res.address.row == -1);
    assert(res.address.col == 0);

    res = resolver.resolve("B2:B4", abs_address_t(0,0,3));
    assert(res.type == formula_name_type::range_reference);
    assert(res.range.first.sheet == 0);
    assert(res.range.first.row == 1);
    assert(res.range.first.col == -2);
    assert(res.range.last.sheet == 0);
    assert(res.range.last.row == 3);
    assert(res.range.last.col == -2);
}

void test_address()
{
    cout << "test address" << endl;
    address_t addr(-1, 0, 0, false, false, false);
    abs_address_t pos(1, 0, 0);
    abs_address_t abs_addr = addr.to_abs(pos);
    assert(abs_addr.sheet == 0 && abs_addr.row == 0 && abs_addr.column == 0);
}

int main()
{
    test_name_resolver();
    test_address();
    return EXIT_SUCCESS;
}
