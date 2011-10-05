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
#include "ixion/formula.hpp"
#include "ixion/model_context.hpp"
#include "ixion/cell.hpp"
#include "ixion/global.hpp"

#include <iostream>
#include <cassert>
#include <string>
#include <cstring>
#include <sstream>

using namespace std;
using namespace ixion;

namespace {

void test_size()
{
    cout << "test size" << endl;
    cout << "int: " << sizeof(int) << endl;
    cout << "long: " << sizeof(long) << endl;
    cout << "double: " << sizeof(double) << endl;
    cout << "size_t: " << sizeof(size_t) << endl;
    cout << "celltype_t: " << sizeof(celltype_t) << endl;
    cout << "base_cell: " << sizeof(base_cell) << endl;
    cout << "string_cell: " << sizeof(string_cell) << endl;
    cout << "numeric_cell: " << sizeof(numeric_cell) << endl;
    cout << "formula_cell: " << sizeof(formula_cell) << endl;
    cout << "formula_tokens_t: " << sizeof(formula_tokens_t) << endl;
}

void test_string_to_double()
{
    cout << "test string to double" << endl;
    struct { const char* s; double v; } tests[] = {
        { "12", 12.0 },
        { "0", 0.0 },
        { "1.3", 1.3 },
        { "1234.00983", 1234.00983 },
        { "-123.3", -123.3 }
    };

    size_t n = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < n; ++i)
    {
        double v = global::to_double(tests[i].s, strlen(tests[i].s));
        assert(v == tests[i].v);
    }
}

void test_name_resolver()
{
    cout << "test name resolver" << endl;

    model_context cxt;
    cxt.append_sheet_name("One", 3);
    cxt.append_sheet_name("Two", 3);
    cxt.append_sheet_name("Three", 5);
    formula_name_resolver_a1 resolver(&cxt);

    // Parse single cell addresses.
    struct {
        const char* name; bool sheet_name;
    } names[] = {
        { "A1", false },
        { "Z1", false },
        { "AA23", false },
        { "AB23", false },
        { "BA1", false },
        { "AAA2", false },
        { "ABA1", false },
        { "BAA1", false },
        { "XFD1048576", false },
        { "One!A1", true },
        { "One!XFD1048576", true },
        { 0, false }
    };

    for (size_t i = 0; names[i].name; ++i)
    {
        const char* p = names[i].name;
        string name_a1(p);
        formula_name_type res = resolver.resolve(&name_a1[0], name_a1.size(), abs_address_t());
        if (res.type != formula_name_type::cell_reference)
        {
            cerr << "failed to resolve cell address: " << name_a1 << endl;
            assert(false);
        }
        address_t addr;
        addr.sheet = res.address.sheet;
        addr.row = res.address.row;
        addr.column = res.address.col;
        string test_name = resolver.get_name(addr, abs_address_t(), names[i].sheet_name);
        if (name_a1 != test_name)
        {
            cerr << "failed to compile name from address: " << name_a1 << endl;
            assert(false);
        }
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
        formula_name_type res = resolver.resolve(&name_a1[0], name_a1.size(), abs_address_t());
        assert(res.type == formula_name_type::range_reference);
        assert(res.range.first.sheet == range_tests[i].sheet1);
        assert(res.range.first.row == range_tests[i].row1);
        assert(res.range.first.col == range_tests[i].col1);
        assert(res.range.last.sheet == range_tests[i].sheet2);
        assert(res.range.last.row == range_tests[i].row2);
        assert(res.range.last.col == range_tests[i].col2);
    }

    formula_name_type res = resolver.resolve("B1", 2, abs_address_t(0,1,1));
    assert(res.type == formula_name_type::cell_reference);
    assert(res.address.sheet == 0);
    assert(res.address.row == -1);
    assert(res.address.col == 0);

    res = resolver.resolve("B2:B4", 5, abs_address_t(0,0,3));
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

bool check_formula_expression(const model_context& cxt, const char* p)
{
    size_t n = strlen(p);
    cout << "testing formula expression '" << p << "'" << endl;
    formula_tokens_t tokens;
    parse_formula_string(cxt, abs_address_t(), p, n, tokens);
    std::string str;
    print_formula_tokens(cxt, abs_address_t(), tokens, str);
    int res = strcmp(p, str.c_str());
    if (res)
        cout << "formula expressions differ: '" << p << "' (before) -> '" << str << "' (after)" << endl;

    return res == 0;
}

void test_external_formula_functions()
{
    cout << "test external formula functions" << endl;
    const char* exps[] = {
        "1/3*1.4",
        "2.3*(1+2)/(34*(3-2))",
        "SUM(1,2,3)",
        "A1",
        "B10",
        "XFD1048576",
        "C10:D20",
        "A1:XFD1048576"
    };
    size_t num_exps = sizeof(exps) / sizeof(exps[0]);
    model_context cxt;
    for (size_t i = 0; i < num_exps; ++i)
    {
        bool result = check_formula_expression(cxt, exps[i]);
        assert(result);
    }
}

}

int main()
{
    test_size();
    test_string_to_double();
    test_name_resolver();
    test_address();
    test_external_formula_functions();
    return EXIT_SUCCESS;
}
