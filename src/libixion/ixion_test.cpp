/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/formula_name_resolver.hpp"
#include "ixion/address.hpp"
#include "ixion/formula.hpp"
#include "ixion/model_context.hpp"
#include "ixion/global.hpp"
#include "ixion/macros.hpp"
#include "ixion/interface/table_handler.hpp"

#include <iostream>
#include <cassert>
#include <string>
#include <cstring>
#include <sstream>

#include <boost/scoped_ptr.hpp>

using namespace std;
using namespace ixion;

namespace {

void test_size()
{
    cout << "test size" << endl;
    cout << "* int: " << sizeof(int) << endl;
    cout << "* long: " << sizeof(long) << endl;
    cout << "* double: " << sizeof(double) << endl;
    cout << "* size_t: " << sizeof(size_t) << endl;
    cout << "* celltype_t: " << sizeof(celltype_t) << endl;
    cout << "* formula_cell: " << sizeof(formula_cell) << endl;
    cout << "* formula_tokens_t: " << sizeof(formula_tokens_t) << endl;
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

struct ref_name_entry
{
    const char* name;
    bool sheet_name;
};

void test_name_resolver_excel_a1()
{
    cout << "test name resolver excel a1" << endl;

    model_context cxt;
    cxt.append_sheet(IXION_ASCII("One"), 1048576, 1024);
    cxt.append_sheet(IXION_ASCII("Two"), 1048576, 1024);
    cxt.append_sheet(IXION_ASCII("Three"), 1048576, 1024);
    cxt.append_sheet(IXION_ASCII("A B C"), 1048576, 1024); // name with space
    boost::scoped_ptr<formula_name_resolver> resolver(
        formula_name_resolver::get(formula_name_resolver_excel_a1, &cxt));
    assert(resolver);

    // Parse single cell addresses.
    ref_name_entry names[] =
    {
        { "A1", false },
        { "$A1", false },
        { "A$1", false },
        { "$A$1", false },
        { "Z1", false },
        { "AA23", false },
        { "AB23", false },
        { "$AB23", false },
        { "AB$23", false },
        { "$AB$23", false },
        { "BA1", false },
        { "AAA2", false },
        { "ABA1", false },
        { "BAA1", false },
        { "XFD1048576", false },
        { "One!A1", true },
        { "One!XFD1048576", true },
        { "Two!B10", true },
        { "Two!$B10", true },
        { "Two!B$10", true },
        { "Two!$B$10", true },
        { "Three!CFD234", true },
        { "'A B C'!Z12", true },
        { 0, false }
    };

    for (size_t i = 0; names[i].name; ++i)
    {
        const char* p = names[i].name;
        string name_a1(p);
        formula_name_type res = resolver->resolve(&name_a1[0], name_a1.size(), abs_address_t());
        if (res.type != formula_name_type::cell_reference)
        {
            cerr << "failed to resolve cell address: " << name_a1 << endl;
            assert(false);
        }

        address_t addr = to_address(res.address);
        string test_name = resolver->get_name(addr, abs_address_t(), names[i].sheet_name);

        if (name_a1 != test_name)
        {
            cerr << "failed to compile name from address: (name expected: " << name_a1 << "; actual name created: " << test_name << ")" << endl;
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
        formula_name_type res = resolver->resolve(&name_a1[0], name_a1.size(), abs_address_t());
        assert(res.type == formula_name_type::range_reference);
        assert(res.range.first.sheet == range_tests[i].sheet1);
        assert(res.range.first.row == range_tests[i].row1);
        assert(res.range.first.col == range_tests[i].col1);
        assert(res.range.last.sheet == range_tests[i].sheet2);
        assert(res.range.last.row == range_tests[i].row2);
        assert(res.range.last.col == range_tests[i].col2);
    }

    formula_name_type res = resolver->resolve("B1", 2, abs_address_t(0,1,1));
    assert(res.type == formula_name_type::cell_reference);
    assert(res.address.sheet == 0);
    assert(res.address.row == -1);
    assert(res.address.col == 0);

    res = resolver->resolve("B2:B4", 5, abs_address_t(0,0,3));
    assert(res.type == formula_name_type::range_reference);
    assert(res.range.first.sheet == 0);
    assert(res.range.first.row == 1);
    assert(res.range.first.col == -2);
    assert(res.range.last.sheet == 0);
    assert(res.range.last.row == 3);
    assert(res.range.last.col == -2);

    // Parse name without row index.
    struct {
        const char* name; formula_name_type::name_type type;
    } name_tests[] = {
        { "H:H", formula_name_type::range_reference },
        { "ABC", formula_name_type::named_expression },
        { "H", formula_name_type::named_expression },
        { "MAX", formula_name_type::function },
        { 0, formula_name_type::invalid }
    };

    for (size_t i = 0; name_tests[i].name; ++i)
    {
        string name_a1(name_tests[i].name);
        formula_name_type res = resolver->resolve(&name_a1[0], name_a1.size(), abs_address_t());
        assert(res.type == name_tests[i].type);
    }
}

void test_name_resolver_table_excel_a1()
{
    cout << "Testing the Excel A1 name resolver for parsing table references." << endl;
    model_context cxt;
    cxt.append_sheet(IXION_ASCII("Sheet"), 1048576, 1024);
    string_id_t s_table1 = cxt.append_string(IXION_ASCII("Table1"));
    string_id_t s_table2 = cxt.append_string(IXION_ASCII("Table2"));
    string_id_t s_cat = cxt.append_string(IXION_ASCII("Category"));
    string_id_t s_val = cxt.append_string(IXION_ASCII("Value"));

    boost::scoped_ptr<formula_name_resolver> resolver(
        formula_name_resolver::get(formula_name_resolver_excel_a1, &cxt));
    assert(resolver);

    struct {
        const char* exp;
        size_t len;
        sheet_t sheet;
        row_t row;
        col_t col;
        string_id_t table_name;
        string_id_t column_first;
        string_id_t column_last;
        table_areas_t areas;
    } tests[] = {
        { IXION_ASCII("[Value]"), 0, 9, 2, empty_string_id, s_val, empty_string_id, table_area_data },
        { IXION_ASCII("Table1[Category]"), 0, 9, 2, s_table1, s_cat, empty_string_id, table_area_data },
        { IXION_ASCII("Table1[Value]"), 0, 9, 2, s_table1, s_val, empty_string_id, table_area_data },
        { IXION_ASCII("Table1[[#Headers],[Value]]"), 0, 9, 2, s_table1, s_val, empty_string_id, table_area_headers },
        { IXION_ASCII("Table1[[#Headers],[#Data],[Value]]"), 0, 9, 2, s_table1, s_val, empty_string_id, table_area_headers | table_area_data },
        { IXION_ASCII("Table1[[#All],[Category]]"), 0, 9, 2, s_table1, s_cat, empty_string_id, table_area_all },
        { IXION_ASCII("Table1[[#Totals],[Category]]"), 0, 9, 2, s_table1, s_cat, empty_string_id, table_area_totals },
        { IXION_ASCII("Table1[[#Data],[#Totals],[Value]]"), 0, 9, 2, s_table1, s_val, empty_string_id, table_area_data | table_area_totals },
        { IXION_ASCII("Table1[#All]"), 0, 9, 2, s_table1, empty_string_id, empty_string_id, table_area_all },
        { IXION_ASCII("Table1[#Headers]"), 0, 9, 2, s_table1, empty_string_id, empty_string_id, table_area_headers },
        { IXION_ASCII("Table1[#Data]"), 0, 9, 2, s_table1, empty_string_id, empty_string_id, table_area_data },
        { IXION_ASCII("Table1[#Totals]"), 0, 9, 2, s_table1, empty_string_id, empty_string_id, table_area_totals },
        { IXION_ASCII("Table1[[#Headers],[#Data]]"), 0, 9, 2, s_table1, empty_string_id, empty_string_id, table_area_headers | table_area_data },
        { IXION_ASCII("Table1[[#Totals],[Category]:[Value]]"), 0, 9, 2, s_table1, s_cat, s_val, table_area_totals },
        { IXION_ASCII("Table1[[#Data],[#Totals],[Category]:[Value]]"), 0, 9, 2, s_table1, s_cat, s_val, table_area_data | table_area_totals },
    };

    for (size_t i = 0, n = IXION_N_ELEMENTS(tests); i < n; ++i)
    {
        cout << "* table reference: " << tests[i].exp << endl;
        abs_address_t pos(tests[i].sheet, tests[i].row, tests[i].col);
        formula_name_type res = resolver->resolve(tests[i].exp, tests[i].len, pos);
        if (res.type != formula_name_type::table_reference)
            assert(!"table reference expected.");

        formula_name_type::table_type table = res.table;
        string_id_t table_name = cxt.get_string_identifier(table.name, table.name_length);
        string_id_t column_first = cxt.get_string_identifier(table.column_first, table.column_first_length);
        string_id_t column_last = cxt.get_string_identifier(table.column_last, table.column_last_length);
        assert(table_name == tests[i].table_name);
        assert(column_first == tests[i].column_first);
        assert(column_last == tests[i].column_last);
        assert(table.areas == tests[i].areas);
    }
}

void test_name_resolver_odff()
{
    cout << "test name resolver odff" << endl;

    model_context cxt;
    cxt.append_sheet(IXION_ASCII("One"), 1048576, 1024);
    cxt.append_sheet(IXION_ASCII("Two"), 1048576, 1024);
    cxt.append_sheet(IXION_ASCII("A B C"), 1048576, 1024); // name with space
    cxt.append_sheet(IXION_ASCII("80's Music"), 1048576, 1024);

    boost::scoped_ptr<formula_name_resolver> resolver(
        formula_name_resolver::get(formula_name_resolver_odff, &cxt));
    assert(resolver);

    // Parse single cell addresses.
    ref_name_entry single_ref_names[] =
    {
        { "[.A1]",   false },
        { "[.$A1]",  false },
        { "[.A$1]",  false },
        { "[.$A$1]", false },
        { 0, false }
    };

    for (size_t i = 0; single_ref_names[i].name; ++i)
    {
        const char* p = single_ref_names[i].name;
        string name_a1(p);
        formula_name_type res = resolver->resolve(&name_a1[0], name_a1.size(), abs_address_t());
        if (res.type != formula_name_type::cell_reference)
        {
            cerr << "failed to resolve cell address: " << name_a1 << endl;
            assert(false);
        }

        address_t addr = to_address(res.address);
        string test_name = resolver->get_name(addr, abs_address_t(), single_ref_names[i].sheet_name);

        if (name_a1 != test_name)
        {
            cerr << "failed to compile name from address: (name expected: " << name_a1 << "; actual name created: " << test_name << ")" << endl;
            assert(false);
        }
    }

    // Parse cell range addresses.
    ref_name_entry range_ref_names[] =
    {
        { "[.A1:.A3]", false },
        { "[.$B5:.$D10]", false },
        { 0, false }
    };

    for (size_t i = 0; range_ref_names[i].name; ++i)
    {
        const char* p = range_ref_names[i].name;
        string name_a1(p);
        formula_name_type res = resolver->resolve(&name_a1[0], name_a1.size(), abs_address_t());
        if (res.type != formula_name_type::range_reference)
        {
            cerr << "failed to resolve range address: " << name_a1 << endl;
            assert(false);
        }

        range_t range = to_range(res.range);
        string test_name = resolver->get_name(range, abs_address_t(), range_ref_names[i].sheet_name);

        if (name_a1 != test_name)
        {
            cerr << "failed to compile name from range: (name expected: " << name_a1 << "; actual name created: " << test_name << ")" << endl;
            assert(false);
        }
    }
}

void test_address()
{
    cout << "test address" << endl;
    address_t addr(-1, 0, 0, false, false, false);
    abs_address_t pos(1, 0, 0);
    abs_address_t abs_addr = addr.to_abs(pos);
    assert(abs_addr.sheet == 0 && abs_addr.row == 0 && abs_addr.column == 0);

    // Default constructor makes valid address.
    assert(abs_address_t().valid());
    assert(abs_range_t().valid());

    // These are invalid addresses.
    assert(!abs_address_t(abs_address_t::invalid).valid());
    assert(!abs_range_t(abs_range_t::invalid).valid());
}

bool check_formula_expression(
    model_context& cxt, const formula_name_resolver& resolver, const char* p)
{
    size_t n = strlen(p);
    cout << "testing formula expression '" << p << "'" << endl;
    formula_tokens_t tokens;
    parse_formula_string(cxt, abs_address_t(), resolver, p, n, tokens);
    std::string str;
    print_formula_tokens(cxt, abs_address_t(), resolver, tokens, str);
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
        "A1:XFD1048576",
        "H:H",
        "B:D",
        "AB:AD",
        "2:2",
        "3:5",
        "34:36"
    };
    size_t num_exps = sizeof(exps) / sizeof(exps[0]);
    model_context cxt;
    boost::scoped_ptr<formula_name_resolver> resolver(
        formula_name_resolver::get(formula_name_resolver_excel_a1, &cxt));
    assert(resolver);

    for (size_t i = 0; i < num_exps; ++i)
    {
        bool result = check_formula_expression(cxt, *resolver, exps[i]);
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
    boost::scoped_ptr<formula_name_resolver> resolver(formula_name_resolver::get(ixion::formula_name_resolver_excel_a1, &cxt));
    size_t n = IXION_N_ELEMENTS(valid_names);
    for (size_t i = 0; i < n; ++i)
    {
        const char* name = valid_names[i];
        cout << "valid name: " << name << endl;
        formula_name_type t = resolver->resolve(name, strlen(name), abs_address_t());
        assert(t.type == formula_name_type::function);
    }

    n = IXION_N_ELEMENTS(invalid_names);
    for (size_t i = 0; i < n; ++i)
    {
        const char* name = invalid_names[i];
        cout << "invalid name: " << name << endl;
        formula_name_type t = resolver->resolve(name, strlen(name), abs_address_t());
        assert(t.type != formula_name_type::function);
    }
}

formula_cell* insert_formula(
    model_context& cxt, const abs_address_t& pos, const char* exp,
    const formula_name_resolver& resolver)
{
    cxt.set_formula_cell(pos, exp, strlen(exp), resolver);
    register_formula_cell(cxt, pos);
    formula_cell* p = cxt.get_formula_cell(pos);
    assert(p);
    return p;
}

void test_model_context_storage()
{
    cout << "test model context storage" << endl;
    {
        model_context cxt;
        boost::scoped_ptr<formula_name_resolver> resolver(
            formula_name_resolver::get(formula_name_resolver_excel_a1, &cxt));
        assert(resolver);

        cxt.append_sheet(IXION_ASCII("test"), 1048576, 1024);
        cxt.set_session_handler(NULL);

        // Test storage of numeric values.
        double val = 0.1;
        for (col_t col = 0; col < 3; ++col)
        {
            for (row_t row = 0; row < 3; ++row)
            {
                abs_address_t pos(0, row, col);
                cxt.set_numeric_cell(pos, val);
                double test = cxt.get_numeric_value(pos);
                assert(test == val);
                val += 0.2;
            }
        }

        // Test formula cells.
        abs_address_t pos(0,3,0);
        const char* exp = "SUM(1,2,3)";
        cxt.set_formula_cell(pos, exp, strlen(exp), *resolver);
        formula_cell* p = cxt.get_formula_cell(pos);
        assert(p);
    }

    {
        model_context cxt;
        boost::scoped_ptr<formula_name_resolver> resolver(
            formula_name_resolver::get(formula_name_resolver_excel_a1, &cxt));
        assert(resolver);

        cxt.append_sheet(IXION_ASCII("test"), 1048576, 1024);
        cxt.set_session_handler(NULL);
        string exp = "1";
        cxt.set_formula_cell(abs_address_t(0,0,0), &exp[0], exp.size(), *resolver);
        cxt.set_formula_cell(abs_address_t(0,2,0), &exp[0], exp.size(), *resolver);
        cxt.set_formula_cell(abs_address_t(0,1,0), &exp[0], exp.size(), *resolver);
    }

    {
        // Test data area.
        model_context cxt;
        cxt.append_sheet(IXION_ASCII("test"), 1048576, 1024);

        abs_range_t area = cxt.get_data_range(0);
        assert(!area.valid());

        cxt.set_numeric_cell(abs_address_t(0, 6, 5), 1.1);
        area = cxt.get_data_range(0);
        assert(area.first == area.last);
        assert(area.first.sheet == 0);
        assert(area.first.row == 6);
        assert(area.first.column == 5);

        cxt.set_numeric_cell(abs_address_t(0, 2, 3), 1.1);
        area = cxt.get_data_range(0);
        assert(area.first.sheet == 0);
        assert(area.first.row == 2);
        assert(area.first.column == 3);
        assert(area.last.sheet == 0);
        assert(area.last.row == 6);
        assert(area.last.column == 5);

        cxt.set_numeric_cell(abs_address_t(0, 7, 1), 1.1);
        area = cxt.get_data_range(0);
        assert(area.first.sheet == 0);
        assert(area.first.row == 2);
        assert(area.first.column == 1);
        assert(area.last.sheet == 0);
        assert(area.last.row == 7);
        assert(area.last.column == 5);

        // This shouldn't change the data range.
        cxt.set_numeric_cell(abs_address_t(0, 5, 5), 1.1);
        abs_range_t test = cxt.get_data_range(0);
        assert(test == area);
    }
}

void test_volatile_function()
{
    cout << "test volatile function" << endl;

    model_context cxt;
    boost::scoped_ptr<formula_name_resolver> resolver(
        formula_name_resolver::get(formula_name_resolver_excel_a1, &cxt));
    assert(resolver);

    cxt.append_sheet(IXION_ASCII("test"), 1048576, 1024);
    cxt.set_session_handler(NULL);

    dirty_formula_cells_t dirty_cells;
    modified_cells_t dirty_addrs;

    // Set values into A1:A3.
    cxt.set_numeric_cell(abs_address_t(0,0,0), 1.0);
    cxt.set_numeric_cell(abs_address_t(0,1,0), 2.0);
    cxt.set_numeric_cell(abs_address_t(0,2,0), 3.0);

    // Set formula in A4 that references A1:A3.
    formula_cell* p = insert_formula(cxt, abs_address_t(0,3,0), "SUM(A1:A3)", *resolver);
    assert(p);
    dirty_cells.insert(abs_address_t(0,3,0));

    // Initial full calculation.
    calculate_cells(cxt, dirty_cells, 0);

    double val = cxt.get_numeric_value(abs_address_t(0,3,0));
    assert(val == 6);

    // Modify the value of A2.  This should flag A4 dirty.
    cxt.set_numeric_cell(abs_address_t(0,1,0), 10.0);
    dirty_cells.clear();
    dirty_addrs.push_back(abs_address_t(0,1,0));
    get_all_dirty_cells(cxt, dirty_addrs, dirty_cells);
    assert(dirty_cells.size() == 1);

    // Partial recalculation.
    calculate_cells(cxt, dirty_cells, 0);
    val = cxt.get_numeric_value(abs_address_t(0, 3, 0));
    assert(val == 14);

    // Insert a volatile cell into B1.  At this point B1 should be the only dirty cell.
    dirty_cells.clear();
    dirty_addrs.clear();
    p = insert_formula(cxt, abs_address_t(0,0,1), "NOW()", *resolver);
    assert(p);
    dirty_cells.insert(abs_address_t(0,0,1));
    dirty_addrs.push_back(abs_address_t(0,0,1));
    get_all_dirty_cells(cxt, dirty_addrs, dirty_cells);
    assert(dirty_cells.size() == 1);

    // Partial recalc again.
    calculate_cells(cxt, dirty_cells, 0);
    double t1 = cxt.get_numeric_value(abs_address_t(0,0,1));

    // Pause for 0.2 second.
    global::sleep(200);

    // No modification, but B1 should still be flagged dirty.
    dirty_cells.clear();
    dirty_addrs.clear();
    get_all_dirty_cells(cxt, dirty_addrs, dirty_cells);
    assert(dirty_cells.size() == 1);
    calculate_cells(cxt, dirty_cells, 0);
    double t2 = cxt.get_numeric_value(abs_address_t(0,0,1));
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
    test_name_resolver_excel_a1();
    test_name_resolver_table_excel_a1();
    test_name_resolver_odff();
    test_address();
    test_parse_and_print_expressions();
    test_function_name_resolution();
    test_model_context_storage();
    test_volatile_function();
    return EXIT_SUCCESS;
}
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
