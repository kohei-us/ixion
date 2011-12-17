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
#include "ixion/macros.hpp"

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
    cxt.append_sheet_name(IXION_ASCII("One"));
    cxt.append_sheet_name(IXION_ASCII("Two"));
    cxt.append_sheet_name(IXION_ASCII("Three"));
    cxt.append_sheet_name(IXION_ASCII("A B C")); // name with space
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
        { "Two!B10", true },
        { "Three!CFD234", true },
        { "'A B C'!Z12", true },
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
        { "One!C1:Z400", 0, 0, 2, 0, 399, 25 },
        { "Two!C1:Z400", 1, 0, 2, 1, 399, 25 },
        { "Three!C1:Z400", 2, 0, 2, 2, 399, 25 },
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

bool check_formula_expression(model_context& cxt, const char* p)
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

/**
 * Make sure the public API works as advertized.
 */
void test_parse_and_print_expressions()
{
    cout << "test public formula api" << endl;
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

/**
 * Function name must be resolved case-insensitively.
 */
void test_function_name_resolution()
{
    cout << "test function name resolution" << endl;

    const char* valid_names[] = {
        "SUM", "sum", "Sum", "Average", "max", "min"
    };

    const char* invalid_names[] = {
        "suma", "foo", "", "su", "maxx", "minmin"
    };

    model_context cxt;
    const formula_name_resolver& resolver = cxt.get_name_resolver();
    size_t n = IXION_N_ELEMENTS(valid_names);
    for (size_t i = 0; i < n; ++i)
    {
        const char* name = valid_names[i];
        cout << "valid name: " << name << endl;
        formula_name_type t = resolver.resolve(name, strlen(name), abs_address_t());
        assert(t.type == formula_name_type::function);
    }

    n = IXION_N_ELEMENTS(invalid_names);
    for (size_t i = 0; i < n; ++i)
    {
        const char* name = invalid_names[i];
        cout << "invalid name: " << name << endl;
        formula_name_type t = resolver.resolve(name, strlen(name), abs_address_t());
        assert(t.type != formula_name_type::function);
    }
}

formula_cell* insert_formula(model_context& cxt, const abs_address_t& pos, const char* exp)
{
    unique_ptr<formula_tokens_t> tokens(new formula_tokens_t);
    parse_formula_string(cxt, pos, exp, strlen(exp), *tokens);
    unique_ptr<formula_cell> fcell(new formula_cell);
    size_t tkid = cxt.add_formula_tokens(0, tokens.release());
    fcell->set_identifier(tkid);
    formula_cell* p = fcell.get();
    cxt.set_cell(pos, fcell.release());
    register_formula_cell(cxt, pos, p);
    return p;
}

void test_volatile_function()
{
    cout << "test volatile function" << endl;

    model_context cxt;
    cxt.set_session_handler(NULL);

    dirty_cells_t dirty_cells;
    dirty_cell_addrs_t dirty_addrs;

    // Set values into A1:A3.
    cxt.set_cell(abs_address_t(0,0,0), new numeric_cell(1.0));
    cxt.set_cell(abs_address_t(0,1,0), new numeric_cell(2.0));
    cxt.set_cell(abs_address_t(0,2,0), new numeric_cell(3.0));

    // Set formula in A4 that references A1:A3.
    formula_cell* p = insert_formula(cxt, abs_address_t(0,3,0), "SUM(A1:A3)");
    assert(p);
    dirty_cells.insert(p);

    // Initial full calculation.
    calculate_cells(cxt, dirty_cells, 0);

    const base_cell* bcell = cxt.get_cell(abs_address_t(0,3,0));
    assert(bcell != NULL);
    assert(bcell->get_value() == 6);

    // Modify the value of A2.  This should flag A4 dirty.
    cxt.set_cell(abs_address_t(0,1,0), new numeric_cell(10.0));
    dirty_cells.clear();
    dirty_addrs.push_back(abs_address_t(0,1,0));
    get_all_dirty_cells(cxt, dirty_addrs, dirty_cells);
    assert(dirty_cells.size() == 1);

    // Partial recalculation.
    calculate_cells(cxt, dirty_cells, 0);
    bcell = cxt.get_cell(abs_address_t(0, 3, 0));
    assert(bcell != NULL);
    assert(bcell->get_value() == 14);

    // Insert a volatile cell into B1.  At this point B1 should be the only dirty cell.
    dirty_cells.clear();
    dirty_addrs.clear();
    p = insert_formula(cxt, abs_address_t(0,0,1), "NOW()");
    assert(p);
    dirty_cells.insert(p);
    dirty_addrs.push_back(abs_address_t(0,0,1));
    get_all_dirty_cells(cxt, dirty_addrs, dirty_cells);
    assert(dirty_cells.size() == 1);

    // Partial recalc again.
    calculate_cells(cxt, dirty_cells, 0);
    bcell = cxt.get_cell(abs_address_t(0,0,1));
    assert(bcell);
    double t1 = bcell->get_value();

    // Pause for 0.2 second.
    global::sleep(200);

    // No modification, but B1 should still be flagged dirty.
    dirty_cells.clear();
    dirty_addrs.clear();
    get_all_dirty_cells(cxt, dirty_addrs, dirty_cells);
    assert(dirty_cells.size() == 1);
    calculate_cells(cxt, dirty_cells, 0);
    bcell = cxt.get_cell(abs_address_t(0,0,1));
    assert(bcell);
    double t2 = bcell->get_value();
    double delta = (t2-t1)*24*60*60;
    cout << "delta = " << delta << endl;

    // The delta should be close to 0.2.  It may be a little larger depending
    // on the CPU speed.
    assert(0.2 <= delta && delta <= 0.3);
}

}

int main()
{
    test_size();
    test_string_to_double();
    test_name_resolver();
    test_address();
    test_parse_and_print_expressions();
    test_function_name_resolution();
    test_volatile_function();
    return EXIT_SUCCESS;
}
