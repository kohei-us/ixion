/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "test_global.hpp" // This must be the first header to be included.

#include <ixion/model_context.hpp>
#include <ixion/formula_name_resolver.hpp>
#include <ixion/address.hpp>
#include <ixion/table.hpp>

#include <string>

using namespace ixion;

namespace {

struct ref_name_entry
{
    const char* name;
    bool sheet_name;
};

void test_name_resolver_calc_a1()
{
    IXION_TEST_FUNC_SCOPE;

    model_context cxt;
    cxt.append_sheet("One");
    cxt.append_sheet("Two");
    cxt.append_sheet("Three");
    cxt.append_sheet("A B C"); // name with space
    auto resolver = formula_name_resolver::get(formula_name_resolver_t::calc_a1, &cxt);
    assert(resolver);

    {
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
            { "One.A1", true },
            { "One.XFD1048576", true },
            { "Two.B10", true },
            { "Two.$B10", true },
            { "Two.B$10", true },
            { "Two.$B$10", true },
            { "Three.CFD234", true },
            { "$Three.CFD234", true },
            { "'A B C'.Z12", true },
            { "$'A B C'.Z12", true },
            { 0, false }
        };

        for (size_t i = 0; names[i].name; ++i)
        {
            const char* p = names[i].name;
            std::string name_a1(p);
            std::cout << "single cell address: " << name_a1 << std::endl;
            formula_name_t res = resolver->resolve(name_a1, abs_address_t());
            if (res.type != formula_name_t::cell_reference)
            {
                std::cerr << "failed to resolve cell address: " << name_a1 << std::endl;
                assert(false);
            }

            address_t addr = std::get<address_t>(res.value);
            std::string test_name = resolver->get_name(addr, abs_address_t(), names[i].sheet_name);

            if (name_a1 != test_name)
            {
                std::cerr << "failed to compile name from address: (name expected: " << name_a1 << "; actual name created: " << test_name << ")" << std::endl;
                assert(false);
            }
        }
    }

    {
        // Parse range addresses.

        ref_name_entry names[] =
        {
            { "A1:B2", false },
            { "$D10:G$24", false },
            { "One.C$1:Z$400", true },
            { "Two.$C1:$Z400", true },
            { "Three.$C1:Z$400", true },
            { "$Three.$C1:Z$400", true },
            { "'A B C'.$C4:$Z256", true },
            { "$'A B C'.$C4:$Z256", true },
            { "One.C4:Three.Z100", true },
            { "One.C4:$Three.Z100", true },
            { "$One.C4:Three.Z100", true },
            { "$One.C4:$Three.Z100", true },
            { 0, false },
        };

        for (sheet_t sheet = 0; sheet <= 3; ++sheet)
        {
            for (size_t i = 0; names[i].name; ++i)
            {
                abs_address_t pos{sheet, 0, 0};
                std::string name_a1(names[i].name);
                std::cout << "range address: " << name_a1 << std::endl;
                formula_name_t res = resolver->resolve(name_a1, pos);
                if (res.type != formula_name_t::range_reference)
                {
                    std::cerr << "failed to resolve range address: " << name_a1 << std::endl;
                    assert(false);
                }

                std::string test_name = resolver->get_name(
                    std::get<range_t>(res.value), pos, names[i].sheet_name);

                if (name_a1 != test_name)
                {
                    std::cerr << "failed to compile name from range: (pos: " << pos << "; name expected: " << name_a1 << "; actual name created: " << test_name << ")" << std::endl;
                    assert(false);
                }
            }
        }
    }

    struct {
        const char* name; sheet_t sheet1; row_t row1; col_t col1; sheet_t sheet2; row_t row2; col_t col2;
    } range_tests[] = {
        { "A1:B2", 0, 0, 0, 0, 1, 1 },
        { "D10:G24", 0, 9, 3, 0, 23, 6 },
        { "One.C1:Z400", 0, 0, 2, 0, 399, 25 },
        { "Two.C1:Z400", 1, 0, 2, 1, 399, 25 },
        { "Three.C1:Z400", 2, 0, 2, 2, 399, 25 },
        { 0, 0, 0, 0, 0, 0, 0 }
    };

    for (size_t i = 0; range_tests[i].name; ++i)
    {
        std::string name_a1(range_tests[i].name);
        std::cout << "range address: " << name_a1 << std::endl;
        formula_name_t res = resolver->resolve(name_a1, abs_address_t());
        auto range = std::get<range_t>(res.value);
        assert(res.type == formula_name_t::range_reference);
        assert(range.first.sheet == range_tests[i].sheet1);
        assert(range.first.row == range_tests[i].row1);
        assert(range.first.column == range_tests[i].col1);
        assert(range.last.sheet == range_tests[i].sheet2);
        assert(range.last.row == range_tests[i].row2);
        assert(range.last.column == range_tests[i].col2);
    }

    {
        formula_name_t res = resolver->resolve("B1", abs_address_t(0,1,1));
        auto addr = std::get<address_t>(res.value);
        assert(res.type == formula_name_t::cell_reference);
        assert(addr.sheet == 0);
        assert(addr.row == -1);
        assert(addr.column == 0);
    }

    {
        formula_name_t res = resolver->resolve("B2:B4", abs_address_t(0,0,3));
        auto range = std::get<range_t>(res.value);
        assert(res.type == formula_name_t::range_reference);
        assert(range.first.sheet == 0);
        assert(range.first.row == 1);
        assert(range.first.column == -2);
        assert(range.last.sheet == 0);
        assert(range.last.row == 3);
        assert(range.last.column == -2);
    }

    {
        formula_name_t res = resolver->resolve("Three.B2", abs_address_t(2,0,0));
        auto addr = std::get<address_t>(res.value);
        assert(res.type == formula_name_t::cell_reference);
        assert(!addr.abs_sheet);
        assert(!addr.abs_row);
        assert(!addr.abs_column);
        assert(addr.sheet == 0);
        assert(addr.row == 1);
        assert(addr.column == 1);
    }

    {
        abs_address_t pos(2, 0, 0);
        std::string name("One.B2:Three.C4");
        formula_name_t res = resolver->resolve(name, pos);
        auto range = std::get<range_t>(res.value);
        assert(res.type == formula_name_t::range_reference);
        assert(!range.first.abs_sheet);
        assert(!range.first.abs_row);
        assert(!range.first.abs_column);
        assert(range.first.sheet == -2);
        assert(range.first.row == 1);
        assert(range.first.column == 1);
        assert(!range.last.abs_sheet);
        assert(!range.last.abs_row);
        assert(!range.last.abs_column);
        assert(range.last.sheet == 0);
        assert(range.last.row == 3);
        assert(range.last.column == 2);

        std::string s = resolver->get_name(range, pos, true);
        assert(s == name);
    }
}

void test_name_resolver_excel_a1()
{
    IXION_TEST_FUNC_SCOPE;

    model_context cxt;
    cxt.append_sheet("One");
    cxt.append_sheet("Two");
    cxt.append_sheet("Three");
    cxt.append_sheet("A B C"); // name with space
    cxt.append_sheet("'quote'"); // quoted name
    auto resolver = formula_name_resolver::get(formula_name_resolver_t::excel_a1, &cxt);
    assert(resolver);

    {
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
            { "'''quote'''!Z12", true },
            { 0, false }
        };

        for (size_t i = 0; names[i].name; ++i)
        {
            const char* p = names[i].name;
            std::string name_a1(p);
            formula_name_t res = resolver->resolve(name_a1, abs_address_t());
            if (res.type != formula_name_t::cell_reference)
            {
                std::cerr << "failed to resolve cell address: " << name_a1 << std::endl;
                assert(false);
            }

            address_t addr = std::get<address_t>(res.value);
            std::string test_name = resolver->get_name(addr, abs_address_t(), names[i].sheet_name);

            if (name_a1 != test_name)
            {
                std::cerr << "failed to compile name from address: (name expected: " << name_a1 << "; actual name created: " << test_name << ")" << std::endl;
                assert(false);
            }
        }
    }

    {
        // Parse range addresses.

        ref_name_entry names[] =
        {
            { "A1:B2", false },
            { "$D10:G$24", false },
            { "One!C$1:Z$400", true },
            { "Two!$C1:$Z400", true },
            { "Three!$C1:Z$400", true },
            { "'A B C'!$C4:$Z256", true },
            { 0, false },
        };

        for (size_t i = 0; names[i].name; ++i)
        {
            const char* p = names[i].name;
            std::string name_a1(p);
            std::cout << "range address: " << name_a1 << std::endl;
            formula_name_t res = resolver->resolve(name_a1, abs_address_t());
            if (res.type != formula_name_t::range_reference)
            {
                std::cerr << "failed to resolve range address: " << name_a1 << std::endl;
                assert(false);
            }

            range_t range = std::get<range_t>(res.value);
            std::string test_name = resolver->get_name(range, abs_address_t(), names[i].sheet_name);

            if (name_a1 != test_name)
            {
                std::cerr << "failed to compile name from range: (name expected: " << name_a1 << "; actual name created: " << test_name << ")" << std::endl;
                assert(false);
            }
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
        std::string name_a1(range_tests[i].name);
        formula_name_t res = resolver->resolve(name_a1, abs_address_t());
        auto range = std::get<range_t>(res.value);
        assert(res.type == formula_name_t::range_reference);
        assert(range.first.sheet == range_tests[i].sheet1);
        assert(range.first.row == range_tests[i].row1);
        assert(range.first.column == range_tests[i].col1);
        assert(range.last.sheet == range_tests[i].sheet2);
        assert(range.last.row == range_tests[i].row2);
        assert(range.last.column == range_tests[i].col2);
    }

    {
        formula_name_t res = resolver->resolve("B1", abs_address_t(0,1,1));
        auto addr = std::get<address_t>(res.value);
        assert(res.type == formula_name_t::cell_reference);
        assert(addr.sheet == 0);
        assert(addr.row == -1);
        assert(addr.column == 0);
    }

    {
        formula_name_t res = resolver->resolve("B2:B4", abs_address_t(0,0,3));
        auto range = std::get<range_t>(res.value);
        assert(res.type == formula_name_t::range_reference);
        assert(range.first.sheet == 0);
        assert(range.first.row == 1);
        assert(range.first.column == -2);
        assert(range.last.sheet == 0);
        assert(range.last.row == 3);
        assert(range.last.column == -2);
    }

    // Parse name without row index.
    struct {
        const char* name; formula_name_t::name_type type;
    } name_tests[] = {
        { "H:H", formula_name_t::range_reference },
        { "ABC", formula_name_t::named_expression },
        { "H", formula_name_t::named_expression },
        { "MAX", formula_name_t::function },
        { 0, formula_name_t::invalid }
    };

    for (size_t i = 0; name_tests[i].name; ++i)
    {
        std::string name_a1(name_tests[i].name);
        formula_name_t res = resolver->resolve(name_a1, abs_address_t());
        assert(res.type == name_tests[i].type);
    }

    {
        // Parse address with non-existing sheet name.  It should be flagged invalid.
        formula_name_t res = resolver->resolve("NotExists!A1", abs_address_t());
        assert(res.type == formula_name_t::invalid);
    }
}

void test_name_resolver_named_expression()
{
    IXION_TEST_FUNC_SCOPE;

    model_context cxt;
    cxt.append_sheet("Sheet");

    const std::vector<formula_name_resolver_t> resolver_types = {
        formula_name_resolver_t::excel_a1,
        formula_name_resolver_t::excel_r1c1,
        // TODO : add more resolver types.
    };

    const std::vector<std::string> names = {
        "MyRange",
        "MyRange2",
    };

    for (formula_name_resolver_t rt : resolver_types)
    {
        std::cout << "formula resolver type: " << int(rt) << std::endl; // TODO : map the enum value to std::string name.

        auto resolver = formula_name_resolver::get(rt, &cxt);
        assert(resolver);

        for (const std::string& name : names)
        {
            std::cout << "parsing '" << name << "'..." << std::endl;
            formula_name_t res = resolver->resolve(name, abs_address_t(0,0,0));
            assert(res.type == formula_name_t::name_type::named_expression);
        }
    }
}

void test_name_resolver_table_excel_a1()
{
    IXION_TEST_FUNC_SCOPE;

    model_context cxt;
    cxt.append_sheet("Sheet");
    string_id_t s_table1 = cxt.append_string("Table1");
    string_id_t s_table2 = cxt.append_string("Table2");
    string_id_t s_cat = cxt.append_string("Category");
    string_id_t s_val = cxt.append_string("Value");

    // Make sure these work correctly before proceeding further with the test.
    assert(s_table1 == cxt.get_identifier_from_string("Table1"));
    assert(s_table2 == cxt.get_identifier_from_string("Table2"));
    assert(s_cat == cxt.get_identifier_from_string("Category"));
    assert(s_val == cxt.get_identifier_from_string("Value"));

    auto resolver = formula_name_resolver::get(formula_name_resolver_t::excel_a1, &cxt);
    assert(resolver);

    struct {
        std::string_view exp;
        sheet_t sheet;
        row_t row;
        col_t col;
        string_id_t table_name;
        string_id_t column_first;
        string_id_t column_last;
        table_areas_t areas;
    } tests[] = {
        { "[Value]", 0, 9, 2, empty_string_id, s_val, empty_string_id, table_area_data },
        { "Table1[Category]", 0, 9, 2, s_table1, s_cat, empty_string_id, table_area_data },
        { "Table1[Value]", 0, 9, 2, s_table1, s_val, empty_string_id, table_area_data },
        { "Table1[[#Headers],[Value]]", 0, 9, 2, s_table1, s_val, empty_string_id, table_area_headers },
        { "Table1[[#Headers],[#Data],[Value]]", 0, 9, 2, s_table1, s_val, empty_string_id, table_area_headers | table_area_data },
        { "Table1[[#All],[Category]]", 0, 9, 2, s_table1, s_cat, empty_string_id, table_area_all },
        { "Table1[[#Totals],[Category]]", 0, 9, 2, s_table1, s_cat, empty_string_id, table_area_totals },
        { "Table1[[#Data],[#Totals],[Value]]", 0, 9, 2, s_table1, s_val, empty_string_id, table_area_data | table_area_totals },
        { "Table1[#All]", 0, 9, 2, s_table1, empty_string_id, empty_string_id, table_area_all },
        { "Table1[#Headers]", 0, 9, 2, s_table1, empty_string_id, empty_string_id, table_area_headers },
        { "Table1[#Data]", 0, 9, 2, s_table1, empty_string_id, empty_string_id, table_area_data },
        { "Table1[#Totals]", 0, 9, 2, s_table1, empty_string_id, empty_string_id, table_area_totals },
        { "Table1[[#Headers],[#Data]]", 0, 9, 2, s_table1, empty_string_id, empty_string_id, table_area_headers | table_area_data },
        { "Table1[[#Totals],[Category]:[Value]]", 0, 9, 2, s_table1, s_cat, s_val, table_area_totals },
        { "Table1[[#Data],[#Totals],[Category]:[Value]]", 0, 9, 2, s_table1, s_cat, s_val, table_area_data | table_area_totals },
    };

    for (size_t i = 0, n = std::size(tests); i < n; ++i)
    {
        std::cout << "* table reference: " << tests[i].exp << std::endl;
        abs_address_t pos(tests[i].sheet, tests[i].row, tests[i].col);
        formula_name_t res = resolver->resolve(tests[i].exp, pos);
        if (res.type != formula_name_t::table_reference)
            assert(!"table reference expected.");

        auto table = std::get<formula_name_t::table_type>(res.value);
        string_id_t table_name = cxt.get_identifier_from_string(table.name);
        string_id_t column_first = cxt.get_identifier_from_string(table.column_first);
        string_id_t column_last = cxt.get_identifier_from_string(table.column_last);
        assert(table_name == tests[i].table_name);
        assert(column_first == tests[i].column_first);
        assert(column_last == tests[i].column_last);
        assert(table.areas == tests[i].areas);

        // Make sure we get the same name back.
        table_t tb;
        tb.name = table_name;
        tb.column_first = column_first;
        tb.column_last = column_last;
        tb.areas = table.areas;
        std::string original(tests[i].exp);
        std::string returned = resolver->get_name(tb);
        std::cout << "  original: " << original << std::endl;
        std::cout << "  returned: " << returned << std::endl;
        assert(original == returned);
    }
}

void test_name_resolver_excel_r1c1()
{
    IXION_TEST_FUNC_SCOPE;

    model_context cxt;
    cxt.append_sheet("One");
    cxt.append_sheet("Two");
    cxt.append_sheet("A B C"); // name with space
    cxt.append_sheet("80's Music");

    auto resolver = formula_name_resolver::get(formula_name_resolver_t::excel_r1c1, &cxt);
    assert(resolver);

    // Parse single cell addresses for round-tripping.
    ref_name_entry single_ref_names[] =
    {
        { "R2", false },
        { "R[3]", false },
        { "R[-10]", false },
        { "C2", false },
        { "C[3]", false },
        { "C[-10]", false },
        { "R1C1", false },
        { "R[1]C2", false },
        { "R2C[-2]", false },
        { "R1C", false },
        { "R[-3]C", false },
        { "RC2", false },
        { "One!R10C", true },
        { "Two!C[-2]", true },
        { "'A B C'!R100", true },
        { 0, false }
    };

    for (size_t i = 0; single_ref_names[i].name; ++i)
    {
        const char* p = single_ref_names[i].name;
        std::string name_r1c1(p);
        std::cout << "Parsing " << name_r1c1 << std::endl;
        formula_name_t res = resolver->resolve(name_r1c1, abs_address_t());
        if (res.type != formula_name_t::cell_reference)
        {
            std::cerr << "failed to resolve cell address: " << name_r1c1 << std::endl;
            assert(false);
        }

        address_t addr = std::get<address_t>(res.value);
        std::string test_name = resolver->get_name(addr, abs_address_t(), single_ref_names[i].sheet_name);

        if (name_r1c1 != test_name)
        {
            std::cerr << "failed to compile name from address: (name expected: "
                << name_r1c1 << "; actual name created: " << test_name << ")" << std::endl;
            assert(false);
        }
    }

    // These are supposed to be all invalid or named expression.
    const char* invalid_address[] = {
        "F",
        "RR",
        "RC",
        "R",
        "C",
        "R0C1",
        "R[-2]C-1",
        "R1C2:",
        "R:",
        "R2:",
        "R[-3]:",
        "C:",
        "C3:",
        "C[-4]:",
        0
    };

    for (size_t i = 0; invalid_address[i]; ++i)
    {
        const char* p = invalid_address[i];
        std::string name_r1c1(p);
        formula_name_t res = resolver->resolve(name_r1c1, abs_address_t());
        if (res.type != formula_name_t::invalid && res.type != formula_name_t::named_expression)
        {
            std::cerr << "address " << name_r1c1 << " is expected to be invalid." << std::endl;
            assert(false);
        }
    }

    // These are supposed to be all valid.
    const char* valid_address[] = {
        "r1c2",
        "r[-2]",
        "c10",
        0
    };

    for (size_t i = 0; valid_address[i]; ++i)
    {
        const char* p = valid_address[i];
        std::string name_r1c1(p);
        formula_name_t res = resolver->resolve(name_r1c1, abs_address_t());
        if (res.type != formula_name_t::cell_reference)
        {
            std::cerr << "address " << name_r1c1 << " is expected to be valid." << std::endl;
            assert(false);
        }
    }

    // Parse range addresses.
    struct {
        const char* name;
        sheet_t sheet1;
        row_t row1;
        col_t col1;
        sheet_t sheet2;
        row_t row2;
        col_t col2;
        bool abs_sheet1;
        bool abs_row1;
        bool abs_col1;
        bool abs_sheet2;
        bool abs_row2;
        bool abs_col2;
    } range_tests[] = {
        { "R1C1:R2C2", 0, 0, 0, 0, 1, 1, true, true, true, true, true, true },
        { "R[-3]C[2]:R[1]C[4]", 0, -3, 2, 0, 1, 4, true, false, false, true, false, false },
        { "R2:R4", 0, 1, column_unset, 0, 3, column_unset, true, true, false, true, true, false },
        { "R[2]:R[4]", 0, 2, column_unset, 0, 4, column_unset, true, false, false, true, false, false },
        { "C3:C6", 0, row_unset, 2, 0, row_unset, 5, true, false, true, true, false, true },
        { "C[3]:C[6]", 0, row_unset, 3, 0, row_unset, 6, true, false, false, true, false, false },
        { "Two!R2C2:R2C[100]", 1, 1, 1, 1, 1, 100, true, true, true, true, true, false },
        { "'A B C'!R[2]:R[4]", 2, 2, column_unset, 2, 4, column_unset, true, false, false, true, false, false },
        { 0, 0, 0, 0, 0, 0, 0, false, false, false, false, false, false }
    };

    for (size_t i = 0; range_tests[i].name; ++i)
    {
        std::string name_r1c1(range_tests[i].name);
        std::cout << "Parsing " << name_r1c1 << std::endl;
        formula_name_t res = resolver->resolve(name_r1c1, abs_address_t());
        auto range = std::get<range_t>(res.value);

        assert(res.type == formula_name_t::range_reference);
        assert(range.first.sheet == range_tests[i].sheet1);
        assert(range.first.row == range_tests[i].row1);
        assert(range.first.column == range_tests[i].col1);
        assert(range.first.abs_sheet == range_tests[i].abs_sheet1);
        if (range.first.row != row_unset)
            // When row is unset, whether it's relative or absolute is not relevant.
            assert(range.first.abs_row == range_tests[i].abs_row1);
        if (range.first.column != column_unset)
            // Same with unset column.
            assert(range.first.abs_column == range_tests[i].abs_col1);

        assert(range.last.sheet == range_tests[i].sheet2);
        assert(range.last.row == range_tests[i].row2);
        assert(range.last.column == range_tests[i].col2);
        assert(range.last.abs_sheet == range_tests[i].abs_sheet2);
        if (range.last.row != row_unset)
            assert(range.last.abs_row == range_tests[i].abs_row2);
        if (range.last.column != column_unset)
            assert(range.last.abs_column == range_tests[i].abs_col2);
    }

    ref_name_entry range_ref_names[] =
    {
        { "R2C2:R3C3", false },
        { "R[-3]C2:R[-1]C3", false },
        { "R[-5]C:R[-1]C", false },
        { "'A B C'!R2:R4", true },
        { "'80''s Music'!C[2]:C[4]", true },
        { 0, false },
    };

    for (size_t i = 0; range_ref_names[i].name; ++i)
    {
        const char* p = range_ref_names[i].name;
        std::string name_r1c1(p);
        std::cout << "Parsing " << name_r1c1 << std::endl;
        formula_name_t res = resolver->resolve(name_r1c1, abs_address_t());
        if (res.type != formula_name_t::range_reference)
        {
            std::cerr << "failed to resolve range address: " << name_r1c1 << std::endl;
            assert(false);
        }

        auto range = std::get<range_t>(res.value);
        std::string test_name = resolver->get_name(range, abs_address_t(), range_ref_names[i].sheet_name);

        if (name_r1c1 != test_name)
        {
            std::cerr << "failed to compile name from range: (name expected: " << name_r1c1 << "; actual name created: " << test_name << ")" << std::endl;
            assert(false);
        }
    }

    struct {
        col_t col;
        std::string name;
    } colnames[] = {
        {   0,   "1" },
        {   1,   "2" },
        {  10,  "11" },
        { 123, "124" },
    };

    for (size_t i = 0, n = std::size(colnames); i < n; ++i)
    {
        std::string colname = resolver->get_column_name(colnames[i].col);
        if (colname != colnames[i].name)
        {
            std::cerr << "column name: expected='" << colnames[i].name << "', actual='" << colname << "'" << std::endl;
            assert(false);
        }
    }
}

void test_name_resolver_odff()
{
    IXION_TEST_FUNC_SCOPE;

    model_context cxt;
    cxt.append_sheet("One");
    cxt.append_sheet("Two");
    cxt.append_sheet("A B C"); // name with space
    cxt.append_sheet("80's Music");

    auto resolver = formula_name_resolver::get(formula_name_resolver_t::odff, &cxt);
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
        std::string name_a1(p);
        formula_name_t res = resolver->resolve(name_a1, abs_address_t());
        if (res.type != formula_name_t::cell_reference)
        {
            std::cerr << "failed to resolve cell address: " << name_a1 << std::endl;
            assert(false);
        }

        address_t addr = std::get<address_t>(res.value);
        std::string test_name = resolver->get_name(addr, abs_address_t(), single_ref_names[i].sheet_name);

        if (name_a1 != test_name)
        {
            std::cerr << "failed to compile name from address: (name expected: " << name_a1 << "; actual name created: " << test_name << ")" << std::endl;
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
        std::string name_a1(p);
        formula_name_t res = resolver->resolve(name_a1, abs_address_t());
        if (res.type != formula_name_t::range_reference)
        {
            std::cerr << "failed to resolve range address: " << name_a1 << std::endl;
            assert(false);
        }

        auto range = std::get<range_t>(res.value);
        std::string test_name = resolver->get_name(range, abs_address_t(), range_ref_names[i].sheet_name);

        if (name_a1 != test_name)
        {
            std::cerr << "failed to compile name from range: (name expected: " << name_a1 << "; actual name created: " << test_name << ")" << std::endl;
            assert(false);
        }
    }

    // single cell addresses with sheet names
    ref_name_entry addr_with_sheet_names[] =
    {
        { "[One.$B$1]", true },
        { "[$One.$B$1]", true },
        { "[Two.$B$2]", true },
        { "[$Two.$B$4]", true },
        { "['A B C'.$B$4]", true },
        { "[$'A B C'.$B$4]", true },
        { 0, false },
    };

    for (size_t i = 0; addr_with_sheet_names[i].name; ++i)
    {
        const char* p = addr_with_sheet_names[i].name;
        std::string name_a1(p);

        formula_name_t res = resolver->resolve(name_a1, abs_address_t());
        if (res.type != formula_name_t::cell_reference)
        {
            std::cerr << "failed to resolve cell address: " << name_a1 << std::endl;
            assert(false);
        }

        address_t addr = std::get<address_t>(res.value);
        std::string test_name = resolver->get_name(addr, abs_address_t(), addr_with_sheet_names[i].sheet_name);

        if (name_a1 != test_name)
        {
            std::cerr << "failed to compile name from cell address: (name expected: " << name_a1 << "; actual name created: " << test_name << ")" << std::endl;
            assert(false);
        }
    }

    // range addresses with sheet names
    ref_name_entry ref_with_sheet_names[] =
    {
        { "[One.$B$1:.$B$30]", true },
        { "[$One.$B$1:.$B$30]", true },
        { "[Two.$B$2:.$D30]", true },
        { "[$Two.$B$4:.F$35]", true },
        { "['A B C'.$B$4:.F$35]", true },
        { "[$'A B C'.$B$4:.F$35]", true },
        { "[$One.B$4:$Two.F35]", true },
        { "[$One.B$4:'A B C'.F35]", true },
        { "[One.B$4:$'A B C'.F35]", true },
        { 0, false },
    };

    for (size_t i = 0; ref_with_sheet_names[i].name; ++i)
    {
        const char* p = ref_with_sheet_names[i].name;
        std::string name_a1(p);

        formula_name_t res = resolver->resolve(name_a1, abs_address_t());
        if (res.type != formula_name_t::range_reference)
        {
            std::cerr << "failed to resolve range address: " << name_a1 << std::endl;
            assert(false);
        }

        auto range = std::get<range_t>(res.value);
        std::string test_name = resolver->get_name(range, abs_address_t(), ref_with_sheet_names[i].sheet_name);

        if (name_a1 != test_name)
        {
            std::cerr << "failed to compile name from range: (name expected: " << name_a1 << "; actual name created: " << test_name << ")" << std::endl;
            assert(false);
        }
    }

    {
        std::string name = "[.H2:.I2]";
        abs_address_t pos(2, 1, 9);
        formula_name_t res = resolver->resolve(name, pos);
        assert(res.type == formula_name_t::range_reference);
        abs_range_t range = std::get<range_t>(res.value).to_abs(pos);
        abs_range_t range_expected(abs_address_t(pos.sheet, 1, 7), 1, 2);
        assert(range == range_expected);
    }

    {
        std::string name = "[Two.$B$2:.$B$10]";
        abs_address_t pos(3, 2, 1);
        formula_name_t res = resolver->resolve(name, pos);
        assert(res.type == formula_name_t::range_reference);
        abs_range_t range = std::get<range_t>(res.value).to_abs(pos);
        abs_range_t range_expected(abs_address_t(1, 1, 1), 9, 1);
        assert(range == range_expected);
    }

    {
        std::string name = "MyRange";
        abs_address_t pos(3, 2, 1);
        formula_name_t res = resolver->resolve(name, pos);
        assert(res.type == formula_name_t::named_expression);
    }
}

void test_name_resolver_odf_cra()
{
    IXION_TEST_FUNC_SCOPE;

    model_context cxt;
    cxt.append_sheet("One");
    cxt.append_sheet("Two");
    cxt.append_sheet("Three");
    cxt.append_sheet("A B C"); // name with space
    auto resolver = formula_name_resolver::get(formula_name_resolver_t::odf_cra, &cxt);
    assert(resolver);

    {
        // Parse single cell addresses.
        ref_name_entry names[] =
        {
            { ".A1", false },
            { ".$A1", false },
            { ".A$1", false },
            { ".$A$1", false },
            { ".Z1", false },
            { ".AA23", false },
            { ".AB23", false },
            { ".$AB23", false },
            { ".AB$23", false },
            { ".$AB$23", false },
            { ".BA1", false },
            { ".AAA2", false },
            { ".ABA1", false },
            { ".BAA1", false },
            { ".XFD1048576", false },
            { "One.A1", true },
            { "One.XFD1048576", true },
            { "Two.B10", true },
            { "Two.$B10", true },
            { "Two.B$10", true },
            { "Two.$B$10", true },
            { "Three.CFD234", true },
            { "$Three.CFD234", true },
            { "'A B C'.Z12", true },
            { "$'A B C'.Z12", true },
            { 0, false }
        };

        for (size_t i = 0; names[i].name; ++i)
        {
            const char* p = names[i].name;
            std::string name_a1(p);
            std::cout << "single cell address: " << name_a1 << std::endl;
            formula_name_t res = resolver->resolve(name_a1, abs_address_t());
            if (res.type != formula_name_t::cell_reference)
            {
                std::cerr << "failed to resolve cell address: " << name_a1 << std::endl;
                assert(false);
            }

            address_t addr = std::get<address_t>(res.value);
            std::string test_name = resolver->get_name(addr, abs_address_t(), names[i].sheet_name);

            if (name_a1 != test_name)
            {
                std::cerr << "failed to compile name from address: (name expected: " << name_a1 << "; actual name created: " << test_name << ")" << std::endl;
                assert(false);
            }
        }
    }

    {
        // Parse range addresses.

        ref_name_entry names[] =
        {
            { ".A1:.B2", false },
            { ".$D10:.G$24", false },
            { "One.C$1:.Z$400", true },
            { "Two.$C1:.$Z400", true },
            { "Three.$C1:.Z$400", true },
            { "$Three.$C1:.Z$400", true },
            { "'A B C'.$C4:.$Z256", true },
            { "$'A B C'.$C4:.$Z256", true },
            { "One.C4:Three.Z100", true },
            { "One.C4:$Three.Z100", true },
            { "$One.C4:Three.Z100", true },
            { "$One.C4:$Three.Z100", true },
            { 0, false },
        };

        for (sheet_t sheet = 0; sheet <= 3; ++sheet)
        {
            for (size_t i = 0; names[i].name; ++i)
            {
                abs_address_t pos{sheet, 0, 0};
                std::string name_a1(names[i].name);
                std::cout << "range address: " << name_a1 << std::endl;
                formula_name_t res = resolver->resolve(name_a1, pos);
                if (res.type != formula_name_t::range_reference)
                {
                    std::cerr << "failed to resolve range address: " << name_a1 << std::endl;
                    assert(false);
                }

                auto range = std::get<range_t>(res.value);
                std::string test_name = resolver->get_name(range, pos, names[i].sheet_name);

                if (name_a1 != test_name)
                {
                    std::cerr << "failed to compile name from range: (pos: " << pos << "; name expected: " << name_a1 << "; actual name created: " << test_name << ")" << std::endl;
                    assert(false);
                }
            }
        }
    }
}

} // anonymous namespace

int main()
{
    test_name_resolver_calc_a1();
    test_name_resolver_excel_a1();
    test_name_resolver_named_expression();
    test_name_resolver_table_excel_a1();
    test_name_resolver_excel_r1c1();
    test_name_resolver_odff();
    test_name_resolver_odf_cra();

    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
