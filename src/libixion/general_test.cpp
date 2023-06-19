/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "test_global.hpp" // This must be the first header to be included.

#include <ixion/formula_name_resolver.hpp>
#include <ixion/address.hpp>
#include <ixion/formula.hpp>
#include <ixion/model_context.hpp>
#include <ixion/model_iterator.hpp>
#include <ixion/named_expressions_iterator.hpp>
#include <ixion/global.hpp>
#include <ixion/macros.hpp>
#include <ixion/interface/table_handler.hpp>
#include <ixion/config.hpp>
#include <ixion/matrix.hpp>
#include <ixion/cell.hpp>
#include <ixion/cell_access.hpp>
#include <ixion/formula_result.hpp>
#include <ixion/exceptions.hpp>

#include <string>
#include <cstring>
#include <sstream>
#include <thread>

using namespace std;
using namespace ixion;

namespace {

void test_size()
{
    IXION_TEST_FUNC_SCOPE;

    cout << "test size" << endl;
    cout << "* int: " << sizeof(int) << endl;
    cout << "* long: " << sizeof(long) << endl;
    cout << "* double: " << sizeof(double) << endl;
    cout << "* size_t: " << sizeof(size_t) << endl;
    cout << "* string_id_t: " << sizeof(string_id_t)
        << " (min:" << std::numeric_limits<string_id_t>::min()
        << "; max:" << std::numeric_limits<string_id_t>::max() << ")" << endl;
    cout << "* celltype_t: " << sizeof(celltype_t) << endl;
    cout << "* formula_cell: " << sizeof(formula_cell) << endl;
    cout << "* formula_tokens_t: " << sizeof(formula_tokens_t) << endl;
}

void test_string_to_double()
{
    IXION_TEST_FUNC_SCOPE;

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
        double v = to_double(tests[i].s);
        assert(v == tests[i].v);
    }
}

void test_string_pool()
{
    IXION_TEST_FUNC_SCOPE;

    model_context cxt;

    string_id_t s_table1 = cxt.append_string("Table1");
    string_id_t s_table2 = cxt.append_string("Table2");
    string_id_t s_cat = cxt.append_string("Category");
    string_id_t s_val = cxt.append_string("Value");

    cxt.dump_strings();

    // Make sure these work correctly before proceeding further with the test.
    assert(s_table1 == cxt.get_identifier_from_string("Table1"));
    assert(s_table2 == cxt.get_identifier_from_string("Table2"));
    assert(s_cat == cxt.get_identifier_from_string("Category"));
    assert(s_val == cxt.get_identifier_from_string("Value"));
}

void test_formula_tokens_store()
{
    IXION_TEST_FUNC_SCOPE;

    formula_tokens_store_ptr_t p = formula_tokens_store::create();
    assert(p->get_reference_count() == 1);
    auto p2 = p;

    assert(p->get_reference_count() == 2);
    assert(p2->get_reference_count() == 2);

    auto p3(p);

    assert(p->get_reference_count() == 3);
    assert(p2->get_reference_count() == 3);
    assert(p3->get_reference_count() == 3);

    p3.reset();
    assert(p->get_reference_count() == 2);
    assert(p2->get_reference_count() == 2);

    p2.reset();
    assert(p->get_reference_count() == 1);
    p.reset();
}

void test_matrix()
{
    IXION_TEST_FUNC_SCOPE;

    struct check
    {
        size_t row;
        size_t col;
        double val;
    };

    std::vector<check> checks =
    {
        { 0, 0, 1.0 },
        { 0, 1, 2.0 },
        { 1, 0, 3.0 },
        { 1, 1, 4.0 },
    };

    numeric_matrix num_mtx(2, 2);

    for (const check& c : checks)
        num_mtx(c.row, c.col) = c.val;

    for (const check& c : checks)
        assert(num_mtx(c.row, c.col) == c.val);

    matrix mtx(num_mtx);

    for (const check& c : checks)
    {
        matrix::element e = mtx.get(c.row, c.col);
        assert(e.type == matrix::element_type::numeric);
        assert(std::get<double>(e.value) == c.val);
    }
}

void test_matrix_non_numeric_values()
{
    IXION_TEST_FUNC_SCOPE;

    matrix mtx(2, 2);
    mtx.set(0, 0, 1.1);
    mtx.set(1, 0, formula_error_t::division_by_zero);
    mtx.set(0, 1, std::string("foo"));
    mtx.set(1, 1, true);

    assert(mtx.get_numeric(0, 0) == 1.1);

    matrix::element elem = mtx.get(1, 0);
    assert(elem.type == matrix::element_type::error);
    assert(std::get<formula_error_t>(elem.value) == formula_error_t::division_by_zero);

    elem = mtx.get(0, 1);
    assert(elem.type == matrix::element_type::string);
    assert(std::get<std::string_view>(elem.value) == "foo");

    elem = mtx.get(1, 1);
    assert(elem.type == matrix::element_type::boolean);
    assert(std::get<bool>(elem.value) == true);
}

void test_address()
{
    IXION_TEST_FUNC_SCOPE;

    {
        address_t addr(-1, 0, 0, false, false, false);
        abs_address_t pos(1, 0, 0);
        abs_address_t abs_addr = addr.to_abs(pos);
        assert(abs_addr.sheet == 0 && abs_addr.row == 0 && abs_addr.column == 0);

        abs_address_t pos_invalid_sheet(invalid_sheet, 2, 3);
        auto test = addr.to_abs(pos_invalid_sheet);
        assert(test.sheet == invalid_sheet);
        assert(test.row == 2);
        assert(test.column == 3);
    }


    // Default constructor makes valid address.
    assert(abs_address_t().valid());
    assert(abs_range_t().valid());

    // These are invalid addresses.
    assert(!abs_address_t(abs_address_t::invalid).valid());
    assert(!abs_range_t(abs_range_t::invalid).valid());

    {
        abs_range_t range(1, 1, 2, 3, 3);
        assert(range.first.sheet == 1);
        assert(range.first.row == 1);
        assert(range.first.column == 2);
        assert(range.last.sheet == 1);
        assert(range.last.row == 3);
        assert(range.last.column == 4);

        abs_range_t range2(range);
        assert(range2 == range);

        abs_rc_range_t rc_range(range);
        assert(rc_range.first.row == 1);
        assert(rc_range.first.column == 2);
        assert(rc_range.last.row == 3);
        assert(rc_range.last.column == 4);
    }
}

bool check_formula_expression(
    model_context& cxt, const formula_name_resolver& resolver, const char* p)
{
    size_t n = strlen(p);
    cout << "testing formula expression '" << p << "'" << endl;

    formula_tokens_t tokens = parse_formula_string(cxt, abs_address_t(), resolver, {p, n});
    std::string expression = print_formula_tokens(cxt, abs_address_t(), resolver, tokens);

    int res = strcmp(p, expression.data());
    if (res)
    {
        cout << "formula expressions differ: '" << p << "' (before) -> '" << expression << "' (after)" << endl;
        return false;
    }

    std::ostringstream os;
    for (const auto& t : tokens)
        os << print_formula_token(cxt, abs_address_t(), resolver, t);
    std::string individual_tokens = os.str();

    if (expression != individual_tokens)
    {
        cout << "whole expression differs from individual token strings:" << endl
             << "  * expression='" << expression << "'" << endl
             << "  * individual-tokens='" << individual_tokens << "'" << endl;
        return false;
    }

    return true;
}

/**
 * Make sure the public API works as advertized.
 */
void test_parse_and_print_expressions()
{
    IXION_TEST_FUNC_SCOPE;

    // Excel A1

    std::vector<const char*> exps = {
        "\" \"",
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
        "34:36",
        "1>2",
        "1>=2",
        "1<2",
        "1<=2",
        "1<>2",
        "1=2",
        "Table1[Category]",
        "Table1[Value]",
        "Table1[#Headers]",
        "Table1[[#Headers],[Category]:[Value]]",
        "Table1[[#Headers],[#Data],[Category]:[Value]]",
        "IF(A1=\"\",\"empty\",\"not empty\")",
        "$'Ying & Yang'.$A$1:$H$54",
    };

    model_context cxt;
    cxt.append_sheet("Test");
    cxt.append_string("Table1");
    cxt.append_string("Category");
    cxt.append_string("Value");
    cxt.append_sheet("Ying & Yang"); // name with '&'

    auto resolver = formula_name_resolver::get(formula_name_resolver_t::excel_a1, &cxt);
    assert(resolver);

    for (const char* exp : exps)
    {
        bool result = check_formula_expression(cxt, *resolver, exp);
        assert(result);
    }

    // Excel R1C1

    exps = {
        "SUM(R[-5]C:R[-1]C)",
    };

    resolver = formula_name_resolver::get(formula_name_resolver_t::excel_r1c1, &cxt);
    assert(resolver);

    for (const char* exp : exps)
    {
        bool result = check_formula_expression(cxt, *resolver, exp);
        assert(result);
    }

    // ODFF

    exps = {
        "\" \"",
        "SUM([.A1];[.B1])",
        "CONCATENATE([.A6];\" \";[.B6])",
        "IF(['Ying & Yang'.$A$1:.$O$200];2;0)",
    };

    resolver = formula_name_resolver::get(formula_name_resolver_t::odff, &cxt);
    assert(resolver);

    config cfg = cxt.get_config();
    cfg.sep_function_arg = ';';
    cxt.set_config(cfg);

    for (const char* exp : exps)
    {
        bool result = check_formula_expression(cxt, *resolver, exp);
        assert(result);
    }
}

/**
 * Function name must be resolved case-insensitively.
 */
void test_function_name_resolution()
{
    IXION_TEST_FUNC_SCOPE;

    const char* valid_names[] = {
        "SUM", "sum", "Sum", "Average", "max", "min"
    };

    const char* invalid_names[] = {
        "suma", "foo", "", "su", "maxx", "minmin"
    };

    model_context cxt;
    cxt.append_sheet("Test");
    auto resolver = formula_name_resolver::get(ixion::formula_name_resolver_t::excel_a1, &cxt);
    size_t n = std::size(valid_names);
    for (size_t i = 0; i < n; ++i)
    {
        const char* name = valid_names[i];
        cout << "valid name: " << name << endl;
        formula_name_t t = resolver->resolve(name, abs_address_t());
        assert(t.type == formula_name_t::function);
    }

    n = std::size(invalid_names);
    for (size_t i = 0; i < n; ++i)
    {
        const char* name = invalid_names[i];
        cout << "invalid name: " << name << endl;
        formula_name_t t = resolver->resolve(name, abs_address_t());
        assert(t.type != formula_name_t::function);
    }
}

formula_cell* insert_formula(
    model_context& cxt, const abs_address_t& pos, const char* exp,
    const formula_name_resolver& resolver)
{
    formula_tokens_t tokens = parse_formula_string(cxt, pos, resolver, exp);
    auto ts = formula_tokens_store::create();
    ts->get() = std::move(tokens);
    formula_cell* p_inserted = cxt.set_formula_cell(pos, ts);
    assert(p_inserted);
    register_formula_cell(cxt, pos);
    formula_cell* p = cxt.get_formula_cell(pos);
    assert(p);
    assert(p == p_inserted);
    return p;
}

void test_model_context_storage()
{
    IXION_TEST_FUNC_SCOPE;

    {
        model_context cxt;
        auto resolver = formula_name_resolver::get(formula_name_resolver_t::excel_a1, &cxt);
        assert(resolver);

        cxt.append_sheet("test");

        // Test empty cell access.
        cell_access ca = cxt.get_cell_access(abs_address_t(0, 0, 0));
        assert(ca.get_type() == celltype_t::empty);
        assert(ca.get_value_type() == cell_value_t::empty);

        // String value on an empty cell should be an empty string.
        std::string_view s = ca.get_string_value();
        assert(s.empty());

        // Likewise...
        s = cxt.get_string_value(abs_address_t(0, 0, 0));
        assert(s.empty());

        // Test storage of numeric values.
        volatile double val = 0.1;
        for (col_t col = 0; col < 3; ++col)
        {
            for (row_t row = 0; row < 3; ++row)
            {
                abs_address_t pos(0, row, col);
                cxt.set_numeric_cell(pos, val);
                double test = cxt.get_numeric_value(pos);
                assert(test == val);

                ca = cxt.get_cell_access(pos);
                assert(ca.get_type() == celltype_t::numeric);
                assert(ca.get_value_type() == cell_value_t::numeric);
                test = ca.get_numeric_value();
                assert(test == val);

                val += 0.2;
            }
        }

        // Test formula cells.
        abs_address_t pos(0,3,0);
        const char* exp = "SUM(1,2,3)";
        formula_tokens_t tokens = parse_formula_string(cxt, pos, *resolver, exp);
        auto ts = formula_tokens_store::create();
        ts->get() = std::move(tokens);
        formula_cell* p_inserted = cxt.set_formula_cell(pos, ts);
        assert(p_inserted);
        formula_cell* p = cxt.get_formula_cell(pos);
        assert(p);
        assert(p_inserted == p);
        p->interpret(cxt, pos);

        ca = cxt.get_cell_access(pos);
        assert(ca.get_type() == celltype_t::formula);
        assert(ca.get_value_type() == cell_value_t::numeric);
        assert(ca.get_numeric_value() == 6.0);
    }

    {
        model_context cxt;
        auto resolver = formula_name_resolver::get(formula_name_resolver_t::excel_a1, &cxt);
        assert(resolver);

        cxt.append_sheet("test");
        string exp = "1";
        cxt.set_formula_cell(abs_address_t(0,0,0), parse_formula_string(cxt, abs_address_t(0,0,0), *resolver, exp));
        cxt.set_formula_cell(abs_address_t(0,2,0), parse_formula_string(cxt, abs_address_t(0,2,0), *resolver, exp));
        cxt.set_formula_cell(abs_address_t(0,1,0), parse_formula_string(cxt, abs_address_t(0,1,0), *resolver, exp));
    }

    {
        // Test data area.
        model_context cxt;
        cxt.append_sheet("test");

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

    {
        // Fill up the document model and make sure the data range is still
        // correct.
        const row_t row_size = 5;
        const col_t col_size = 4;
        model_context cxt({row_size, col_size});
        cxt.append_sheet("test");
        for (row_t row = 0; row < row_size; ++row)
            for (col_t col = 0; col < col_size; ++col)
                cxt.set_numeric_cell(abs_address_t(0,row,col), 1.0);

        abs_range_t test = cxt.get_data_range(0);

        assert(test.first.sheet == 0);
        assert(test.first.row == 0);
        assert(test.first.column == 0);
        assert(test.last.sheet == 0);
        assert(test.last.row == row_size-1);
        assert(test.last.column == col_size-1);
    }

    {
        const row_t row_size = 5;
        const col_t col_size = 4;
        model_context cxt({row_size, col_size});
        cxt.append_sheet("test");
        cxt.set_numeric_cell(abs_address_t(0,0,0), 1.0);
        cxt.set_numeric_cell(abs_address_t(0,row_size-1,0), 1.0);
        cxt.set_numeric_cell(abs_address_t(0,row_size/2,col_size/2), 1.0);

        abs_range_t test = cxt.get_data_range(0);

        assert(test.first.sheet == 0);
        assert(test.first.row == 0);
        assert(test.first.column == 0);
        assert(test.last.sheet == 0);
        assert(test.last.row == row_size-1);
        assert(test.last.column == col_size/2);
    }
}

void test_model_context_direct_string_access()
{
    IXION_TEST_FUNC_SCOPE;

    model_context cxt{{400, 20}};
    cxt.append_sheet("test");

    // regular string cell
    abs_address_t B2(0, 1, 1);
    cxt.set_string_cell(B2, "string cell");
    std::string_view s = cxt.get_string_value(B2);
    assert(s == "string cell");

    cell_access ca = cxt.get_cell_access(B2);
    assert(ca.get_type() == celltype_t::string);
    assert(ca.get_value_type() == cell_value_t::string);
    s = ca.get_string_value();
    assert(s == "string cell");

    // formula cell containing a string result.
    abs_address_t C4(0, 3, 2);
    auto resolver = formula_name_resolver::get(formula_name_resolver_t::calc_a1, &cxt);
    assert(resolver);

    // Insert a formula containing one literal string token.
    formula_tokens_t tokens = parse_formula_string(cxt, C4, *resolver, "\"string value in formula\"");
    assert(tokens.size() == 1);
    cxt.set_formula_cell(C4, std::move(tokens));
    // no need to register formula cell since it does not reference other cells.

    abs_range_set_t formula_cells{C4};
    auto sorted = query_and_sort_dirty_cells(cxt, abs_range_set_t(), &formula_cells);
    calculate_sorted_cells(cxt, sorted, 1);

    s = cxt.get_string_value(C4);
    assert(s == "string value in formula");

    ca = cxt.get_cell_access(C4);
    assert(ca.get_type() == celltype_t::formula);
    assert(ca.get_value_type() == cell_value_t::string);
    s = ca.get_string_value();
    assert(s == "string value in formula");
}

void test_model_context_named_expression()
{
    IXION_TEST_FUNC_SCOPE;

    model_context cxt{{400, 20}};
    cxt.append_sheet("test");
    auto resolver = formula_name_resolver::get(formula_name_resolver_t::calc_a1, &cxt);
    assert(resolver);

    abs_address_t B3(0, 2, 1);

    struct test_case
    {
        std::string name;
        std::string formula;
        abs_address_t origin;
    };

    std::vector<test_case> tcs = {
        { "LeftAndAbove", "A3+B2", B3 },
        { "SumAboveRow", "SUM(A2:D2)", B3 },
    };

    for (const test_case& tc : tcs)
    {
        formula_tokens_t tokens = parse_formula_string(cxt, tc.origin, *resolver, tc.formula);
        std::string test = print_formula_tokens(cxt, tc.origin, *resolver, tokens);
        assert(test == tc.formula);

        cxt.set_named_expression(tc.name, tc.origin, std::move(tokens));
    }

    for (const test_case& tc : tcs)
    {
        const named_expression_t* exp = cxt.get_named_expression(0, tc.name);
        assert(exp);
        assert(exp->origin == tc.origin);
        std::string test = print_formula_tokens(cxt, exp->origin, *resolver, exp->tokens);
        assert(test == tc.formula);
    }

    // invalid names should be rejected.
    struct name_test_case
    {
        std::string name;
        bool valid;
    };

    std::vector<name_test_case> invalid_names = {
        { "Name 1", false },
        { "Name_1", true },
        { "123Name", false },
        { "Name123", true },
        { "", false },
        { "Name.1", true },
        { ".Name.2", false },
    };

    for (const name_test_case& tc : invalid_names)
    {
        abs_address_t origin;
        std::string formula = "1+2";

        if (tc.valid)
        {
            formula_tokens_t tokens = parse_formula_string(cxt, origin, *resolver, formula);
            cxt.set_named_expression(tc.name, origin, std::move(tokens));

            tokens = parse_formula_string(cxt, origin, *resolver, formula);
            cxt.set_named_expression(0, tc.name, origin, std::move(tokens));
        }
        else
        {
            try
            {
                formula_tokens_t tokens = parse_formula_string(cxt, origin, *resolver, formula);
                cxt.set_named_expression(tc.name, origin, std::move(tokens));
                assert(!"named expression with invalid name should have been rejected!");
            }
            catch (const model_context_error& e)
            {
                assert(e.get_error_type() == model_context_error::invalid_named_expression);
            }

            try
            {
                formula_tokens_t tokens = parse_formula_string(cxt, origin, *resolver, formula);
                cxt.set_named_expression(0, tc.name, origin, std::move(tokens));
                assert(!"named expression with invalid name should have been rejected!");
            }
            catch (const model_context_error& e)
            {
                assert(e.get_error_type() == model_context_error::invalid_named_expression);
            }
        }
    }
}

bool check_model_iterator_output(model_iterator& iter, const std::vector<model_iterator::cell>& checks)
{
    for (const model_iterator::cell& c : checks)
    {
        if (!iter.has())
        {
            cerr << "a cell value was expected, but none found." << endl;
            return false;
        }

        if (iter.get() != c)
        {
            cerr << "unexpected cell value: expected=" << c << "; observed=" << iter.get() << endl;
            return false;
        }

        iter.next();
    }

    if (iter.has())
    {
        cerr << "an additional cell value was found, but none was expected." << endl;
        return false;
    }

    return true;
}

void test_model_context_iterator_horizontal()
{
    IXION_TEST_FUNC_SCOPE;

    const row_t row_size = 5;
    const col_t col_size = 2;
    model_context cxt{{row_size, col_size}};
    model_iterator iter;

    abs_rc_range_t whole_range;
    whole_range.set_all_columns();
    whole_range.set_all_rows();

    // It should not crash or throw an exception on empty model.
    iter = cxt.get_model_iterator(0, rc_direction_t::horizontal, whole_range);
    assert(!iter.has());

    // Insert an actual sheet and try again.

    cxt.append_sheet("empty sheet");
    iter = cxt.get_model_iterator(0, rc_direction_t::horizontal, whole_range);

    // Make sure the cell position iterates correctly.
    size_t cell_count = 0;
    for (row_t row = 0; row < row_size; ++row)
    {
        for (col_t col = 0; col < col_size; ++cell_count, ++col, iter.next())
        {
            assert(iter.has());
            assert(iter.get().row == row);
            assert(iter.get().col == col);
            assert(iter.get().type == celltype_t::empty);
        }
    }

    assert(!iter.has()); // There should be no more cells on this sheet.
    assert(cell_count == 10);

    cxt.append_sheet("values");
    cxt.set_string_cell(abs_address_t(1, 0, 0), "F1");
    cxt.set_string_cell(abs_address_t(1, 0, 1), "F2");
    cxt.set_boolean_cell(abs_address_t(1, 1, 0), true);
    cxt.set_boolean_cell(abs_address_t(1, 1, 1), false);
    cxt.set_numeric_cell(abs_address_t(1, 2, 0), 3.14);
    cxt.set_numeric_cell(abs_address_t(1, 2, 1), -12.5);

    auto resolver = formula_name_resolver::get(formula_name_resolver_t::excel_a1, &cxt);
    abs_range_set_t modified_cells;
    abs_address_t pos(1, 3, 0);
    formula_tokens_t tokens = parse_formula_string(cxt, pos, *resolver, "SUM(1, 2, 3)");
    formula_cell* p = cxt.set_formula_cell(pos, std::move(tokens));
    assert(p);
    const formula_tokens_t& t = p->get_tokens()->get();
    assert(t.size() == 8); // there should be 8 tokens.
    register_formula_cell(cxt, pos, p);
    modified_cells.insert(pos);

    pos.column = 1;
    tokens = parse_formula_string(cxt, pos, *resolver, "5 + 6 - 7");
    p = cxt.set_formula_cell(pos, std::move(tokens));
    register_formula_cell(cxt, pos, p);
    modified_cells.insert(pos);

    // Calculate the formula cells.
    auto sorted = query_and_sort_dirty_cells(cxt, abs_range_set_t(), &modified_cells);
    calculate_sorted_cells(cxt, sorted, 1);

    std::vector<model_iterator::cell> checks =
    {
        // row, column, value
        { 0, 0, cxt.get_identifier_from_string("F1") },
        { 0, 1, cxt.get_identifier_from_string("F2") },
        { 1, 0, true },
        { 1, 1, false },
        { 2, 0, 3.14 },
        { 2, 1, -12.5 },
        { 3, 0, cxt.get_formula_cell(abs_address_t(1, 3, 0)) },
        { 3, 1, cxt.get_formula_cell(abs_address_t(1, 3, 1)) },
        { 4, 0 },
        { 4, 1 },
    };

    // Iterator and check the individual cell values.
    iter = cxt.get_model_iterator(1, rc_direction_t::horizontal, whole_range);
    assert(check_model_iterator_output(iter, checks));
}

void test_model_context_iterator_horizontal_range()
{
    IXION_TEST_FUNC_SCOPE;

    nullptr_t empty = nullptr;
    model_context cxt{{10, 5}};
    cxt.append_sheet("Values");
    cxt.set_cell_values(0, {
        { "F1",  "F2",  "F3",  "F4",  "F5" },
        {  1.0,  true,  "s1", empty, empty },
        {  1.1, false, empty,  "s2", empty },
        {  1.2, false, empty,  "s3", empty },
        {  1.3,  true, empty,  "s4", empty },
        {  1.4, false, empty,  "s5", empty },
        {  1.5,  "NA", empty,  "s6", empty },
        {  1.6,  99.9, empty,  "s7", empty },
        {  1.7, 199.9, empty,  "s8", empty },
        {  1.8, 299.9, empty,  "s9", "end" },
    });

    // Only iterate over the first two rows.
    abs_rc_range_t range;
    range.set_all_columns();
    range.first.row = 0;
    range.last.row = 1;

    model_iterator iter = cxt.get_model_iterator(0, rc_direction_t::horizontal, range);

    std::vector<model_iterator::cell> checks =
    {
        // row, column, value
        { 0, 0, cxt.get_identifier_from_string("F1") },
        { 0, 1, cxt.get_identifier_from_string("F2") },
        { 0, 2, cxt.get_identifier_from_string("F3") },
        { 0, 3, cxt.get_identifier_from_string("F4") },
        { 0, 4, cxt.get_identifier_from_string("F5") },
        { 1, 0, 1.0 },
        { 1, 1, true },
        { 1, 2, cxt.get_identifier_from_string("s1") },
        { 1, 3 },
        { 1, 4 },
    };

    assert(check_model_iterator_output(iter, checks));

    // Only iterate over rows 2:4.
    range.first.row = 2;
    range.last.row = 4;
    iter = cxt.get_model_iterator(0, rc_direction_t::horizontal, range);

    checks =
    {
        // row, column, value
        { 2, 0, 1.1 },
        { 2, 1, false },
        { 2, 2 },
        { 2, 3, cxt.get_identifier_from_string("s2") },
        { 2, 4 },
        { 3, 0, 1.2 },
        { 3, 1, false },
        { 3, 2 },
        { 3, 3, cxt.get_identifier_from_string("s3") },
        { 3, 4 },
        { 4, 0, 1.3 },
        { 4, 1, true },
        { 4, 2 },
        { 4, 3, cxt.get_identifier_from_string("s4") },
        { 4, 4 },
    };

    assert(check_model_iterator_output(iter, checks));

    // Only iterate over columns 1:3 and only down to row 4.
    range.set_all_rows();
    range.first.column = 1;
    range.last.column = 3;
    range.last.row = 4;
    iter = cxt.get_model_iterator(0, rc_direction_t::horizontal, range);

    checks =
    {
        // row, column, value
        { 0, 1, cxt.get_identifier_from_string("F2") },
        { 0, 2, cxt.get_identifier_from_string("F3") },
        { 0, 3, cxt.get_identifier_from_string("F4") },
        { 1, 1, true },
        { 1, 2, cxt.get_identifier_from_string("s1") },
        { 1, 3 },
        { 2, 1, false },
        { 2, 2 },
        { 2, 3, cxt.get_identifier_from_string("s2") },
        { 3, 1, false },
        { 3, 2 },
        { 3, 3, cxt.get_identifier_from_string("s3") },
        { 4, 1, true },
        { 4, 2 },
        { 4, 3, cxt.get_identifier_from_string("s4") },
    };

    assert(check_model_iterator_output(iter, checks));
}

void test_model_context_iterator_vertical()
{
    IXION_TEST_FUNC_SCOPE;

    const row_t row_size = 5;
    const col_t col_size = 2;
    model_context cxt{{row_size, col_size}};
    model_iterator iter;

    abs_rc_range_t whole_range;
    whole_range.set_all_columns();
    whole_range.set_all_rows();

    // It should not crash or throw an exception on empty model.
    iter = cxt.get_model_iterator(0, rc_direction_t::vertical, whole_range);
    assert(!iter.has());

    // Insert an actual sheet and try again.

    cxt.append_sheet("empty sheet");
    iter = cxt.get_model_iterator(0, rc_direction_t::vertical, whole_range);

    // Make sure the cell position iterates correctly.
    size_t cell_count = 0;
    for (col_t col = 0; col < col_size; ++col)
    {
        for (row_t row = 0; row < row_size; ++cell_count, ++row, iter.next())
        {
            const model_iterator::cell& cell = iter.get();
            assert(iter.has());
            assert(cell.row == row);
            assert(cell.col == col);
            assert(cell.type == celltype_t::empty);
        }
    }

    assert(!iter.has()); // There should be no more cells on this sheet.
    assert(cell_count == 10);

    cxt.append_sheet("values");
    cxt.set_string_cell(abs_address_t(1, 0, 0), "F1");
    cxt.set_string_cell(abs_address_t(1, 0, 1), "F2");
    cxt.set_boolean_cell(abs_address_t(1, 1, 0), true);
    cxt.set_boolean_cell(abs_address_t(1, 1, 1), false);
    cxt.set_numeric_cell(abs_address_t(1, 2, 0), 3.14);
    cxt.set_numeric_cell(abs_address_t(1, 2, 1), -12.5);

    auto resolver = formula_name_resolver::get(formula_name_resolver_t::excel_a1, &cxt);
    abs_range_set_t modified_cells;
    abs_address_t pos(1, 3, 0);
    formula_tokens_t tokens = parse_formula_string(cxt, pos, *resolver, "SUM(1, 2, 3)");
    cxt.set_formula_cell(pos, std::move(tokens));
    register_formula_cell(cxt, pos);
    modified_cells.insert(pos);

    pos.column = 1;
    tokens = parse_formula_string(cxt, pos, *resolver, "5 + 6 - 7");
    cxt.set_formula_cell(pos, std::move(tokens));
    register_formula_cell(cxt, pos);
    modified_cells.insert(pos);

    // Calculate the formula cells.
    auto sorted = query_and_sort_dirty_cells(cxt, abs_range_set_t(), &modified_cells);
    calculate_sorted_cells(cxt, sorted, 1);

    std::vector<model_iterator::cell> checks =
    {
        // row, column, value
        { 0, 0, cxt.get_identifier_from_string("F1") },
        { 1, 0, true },
        { 2, 0, 3.14 },
        { 3, 0, cxt.get_formula_cell(abs_address_t(1, 3, 0)) },
        { 4, 0 },

        { 0, 1, cxt.get_identifier_from_string("F2") },
        { 1, 1, false },
        { 2, 1, -12.5 },
        { 3, 1, cxt.get_formula_cell(abs_address_t(1, 3, 1)) },
        { 4, 1 },
    };

    iter = cxt.get_model_iterator(1, rc_direction_t::vertical, whole_range);
    assert(check_model_iterator_output(iter, checks));
}

void test_model_context_iterator_vertical_range()
{
    IXION_TEST_FUNC_SCOPE;

    nullptr_t empty = nullptr;
    model_context cxt{{10, 5}};
    cxt.append_sheet("Values");
    cxt.set_cell_values(0, {
        { "F1",  "F2",  "F3",  "F4",  "F5" },
        {  1.0,  true,  "s1", empty, empty },
        {  1.1, false, empty,  "s2", empty },
        {  1.2, false, empty,  "s3", empty },
        {  1.3,  true, empty,  "s4", empty },
        {  1.4, false, empty,  "s5", empty },
        {  1.5,  "NA", empty,  "s6", empty },
        {  1.6,  99.9, empty,  "s7", empty },
        {  1.7, 199.9, empty,  "s8", empty },
        {  1.8, 299.9, empty,  "s9", "end" },
    });

    // Iterate over the top 2 rows.
    abs_rc_range_t range;
    range.set_all_columns();
    range.set_all_rows();
    range.last.row = 1;

    model_iterator iter = cxt.get_model_iterator(0, rc_direction_t::vertical, range);

    std::vector<model_iterator::cell> checks =
    {
        // row, column, value
        { 0, 0, cxt.get_identifier_from_string("F1") },
        { 1, 0, 1.0 },
        { 0, 1, cxt.get_identifier_from_string("F2") },
        { 1, 1, true },
        { 0, 2, cxt.get_identifier_from_string("F3") },
        { 1, 2, cxt.get_identifier_from_string("s1") },
        { 0, 3, cxt.get_identifier_from_string("F4") },
        { 1, 3 },
        { 0, 4, cxt.get_identifier_from_string("F5") },
        { 1, 4 },
    };

    assert(check_model_iterator_output(iter, checks));

    // Iterate over the bottom 2 rows.

    range.set_all_rows();
    range.first.row = 8;
    iter = cxt.get_model_iterator(0, rc_direction_t::vertical, range);

    checks =
    {
        // row, column, value
        { 8, 0, 1.7 },
        { 9, 0, 1.8 },
        { 8, 1, 199.9 },
        { 9, 1, 299.9 },
        { 8, 2 },
        { 9, 2 },
        { 8, 3, cxt.get_identifier_from_string("s8") },
        { 9, 3, cxt.get_identifier_from_string("s9") },
        { 8, 4 },
        { 9, 4, cxt.get_identifier_from_string("end") },
    };

    assert(check_model_iterator_output(iter, checks));

    // Iterate over the bottom-left corners.
    range.last.column = 2;
    iter = cxt.get_model_iterator(0, rc_direction_t::vertical, range);

    checks =
    {
        // row, column, value
        { 8, 0, 1.7 },
        { 9, 0, 1.8 },
        { 8, 1, 199.9 },
        { 9, 1, 299.9 },
        { 8, 2 },
        { 9, 2 },
    };

    assert(check_model_iterator_output(iter, checks));

    // Iterate over the top-right corners.
    range.first.column = 3;
    range.last.column = column_unset;
    range.first.row = row_unset;
    range.last.row = 1;
    iter = cxt.get_model_iterator(0, rc_direction_t::vertical, range);

    checks =
    {
        { 0, 3, cxt.get_identifier_from_string("F4") },
        { 1, 3 },
        { 0, 4, cxt.get_identifier_from_string("F5") },
        { 1, 4 },
    };

    assert(check_model_iterator_output(iter, checks));

    // Iterate over only one cell in the middle.
    range.first.row = 5;
    range.last.row = 5;
    range.first.column = 3;
    range.last.column = 3;

    iter = cxt.get_model_iterator(0, rc_direction_t::vertical, range);
    checks =
    {
        { 5, 3, cxt.get_identifier_from_string("s5") },
    };

    assert(check_model_iterator_output(iter, checks));
}

void test_model_context_iterator_named_exps()
{
    IXION_TEST_FUNC_SCOPE;

    struct check
    {
        std::string name;
        const named_expression_t* exp;
    };

    model_context cxt{{100, 10}};
    cxt.append_sheet("test1");
    cxt.append_sheet("test2");

    named_expressions_iterator iter;
    assert(!iter.has());
    assert(iter.size() == 0);

    iter = cxt.get_named_expressions_iterator();
    assert(!iter.has());
    assert(iter.size() == 0);

    auto resolver = formula_name_resolver::get(formula_name_resolver_t::calc_a1, &cxt);
    assert(resolver);

    auto tokenize = [&](const char* p) -> formula_tokens_t
    {
        return parse_formula_string(cxt, abs_address_t(), *resolver, p);
    };

    auto validate = [](named_expressions_iterator _iter, const std::vector<check>& _expected) -> bool
    {
        if (_iter.size() != _expected.size())
        {
            cout << "iterator's size() returns wrong value." << endl;
            return false;
        }

        for (const check& c : _expected)
        {
            if (!_iter.has())
            {
                cout << "iterator has no more element, but it is expected to." << endl;
                return false;
            }

            if (c.name != *_iter.get().name)
            {
                cout << "names differ: expected='" << c.name << "'; actual='" << *_iter.get().name << endl;
                return false;
            }

            if (c.exp != _iter.get().expression)
            {
                cout << "expressions differ." << endl;
                return false;
            }

            _iter.next();
        }

        if (_iter.has())
        {
            cout << "the iterator has more elements, but it is not expected to." << endl;
            return false;
        }

        return true;
    };

    cxt.set_named_expression("MyCalc", tokenize("(1+2)/3"));

    std::vector<check> expected =
    {
        { "MyCalc", cxt.get_named_expression(0, "MyCalc") },
    };

    iter = cxt.get_named_expressions_iterator();
    assert(validate(iter, expected));

    cxt.set_named_expression("RefToRight", tokenize("B1"));

    expected =
    {
        { "MyCalc", cxt.get_named_expression(0, "MyCalc") },
        { "RefToRight", cxt.get_named_expression(0, "RefToRight") },
    };

    iter = cxt.get_named_expressions_iterator();
    assert(validate(iter, expected));

    cxt.set_named_expression(1, "MyCalc2", tokenize("(B1+C1)/D1"));
    cxt.set_named_expression(1, "MyCalc3", tokenize("B1/(PI()*2)"));

    iter = cxt.get_named_expressions_iterator(0);
    assert(!iter.has());

    iter = cxt.get_named_expressions_iterator(1);

    expected =
    {
        { "MyCalc2", cxt.get_named_expression(1, "MyCalc2") },
        { "MyCalc3", cxt.get_named_expression(1, "MyCalc3") },
    };

    assert(validate(iter, expected));
}

void test_model_context_fill_down()
{
    IXION_TEST_FUNC_SCOPE;

    nullptr_t empty = nullptr;
    model_context cxt{{100, 10}};
    cxt.append_sheet("test");
    cxt.set_cell_values(0, {
        { "numeric", "bool", "string",  "empty" },
        {      12.3,   true,    "foo",    empty },
        {     empty,  empty,    empty,      1.1 },
        {     empty,  empty,    empty,      1.1 },
        {     empty,  empty,    empty,      1.1 },
        {     empty,  empty,    empty,      1.1 },
        {     empty,  empty,    empty,      1.1 },
    });

    abs_address_t pos(0, 1, 0);
    cxt.fill_down_cells(pos, 2);

    assert(cxt.get_numeric_value(abs_address_t(0, 1, 0)) == 12.3);
    assert(cxt.get_numeric_value(abs_address_t(0, 2, 0)) == 12.3);
    assert(cxt.get_numeric_value(abs_address_t(0, 3, 0)) == 12.3);
    assert(cxt.is_empty(abs_address_t(0, 4, 0)));

    pos.column = 1;
    cxt.fill_down_cells(pos, 1);
    assert(cxt.get_boolean_value(abs_address_t(0, 1, 1)) == true);
    assert(cxt.get_boolean_value(abs_address_t(0, 2, 1)) == true);
    assert(cxt.is_empty(abs_address_t(0, 3, 1)));

    pos.column = 2;
    string_id_t s_foo = cxt.get_string_identifier(pos);
    const std::string* p = cxt.get_string(s_foo);
    assert(p && *p == "foo");
    cxt.fill_down_cells(pos, 3);
    assert(cxt.get_string_identifier(abs_address_t(0, 2, 2)) == s_foo);
    assert(cxt.get_string_identifier(abs_address_t(0, 3, 2)) == s_foo);
    assert(cxt.get_string_identifier(abs_address_t(0, 4, 2)) == s_foo);
    assert(cxt.is_empty(abs_address_t(0, 5, 2)));

    pos.column = 3;
    cxt.fill_down_cells(pos, 2);
    assert(cxt.is_empty(pos));
    assert(cxt.is_empty(abs_address_t(0, 2, 3)));
    assert(cxt.is_empty(abs_address_t(0, 3, 3)));
    assert(cxt.get_numeric_value(abs_address_t(0, 4, 3)) == 1.1);
}

void test_model_context_error_value()
{
    IXION_TEST_FUNC_SCOPE;

    model_context cxt{{100, 10}};
    cxt.append_sheet("test");

    auto resolver = formula_name_resolver::get(formula_name_resolver_t::excel_a1, &cxt);
    assert(resolver);

    abs_address_t pos(0,3,0);
    const char* exp = "10/0";
    formula_tokens_t tokens = parse_formula_string(cxt, pos, *resolver, exp);
    formula_cell* fc = cxt.set_formula_cell(pos, std::move(tokens));
    fc->interpret(cxt, pos);

    cell_access ca = cxt.get_cell_access(pos);
    assert(ca.get_type() == celltype_t::formula);
    assert(ca.get_value_type() == cell_value_t::error);
    assert(ca.get_error_value() == formula_error_t::division_by_zero);
}

void test_model_context_rename_sheets()
{
    IXION_TEST_FUNC_SCOPE;

    model_context cxt{{100, 10}};
    cxt.append_sheet("sheet1");
    cxt.append_sheet("sheet2");
    cxt.append_sheet("sheet3");

    assert(cxt.get_sheet_index("sheet1") == 0);
    assert(cxt.get_sheet_index("sheet2") == 1);
    assert(cxt.get_sheet_index("sheet3") == 2);

    cxt.set_sheet_name(0, "sheet1"); // Setting it to the current name is a no-op.
    try
    {
        cxt.set_sheet_name(0, "sheet3");
        assert(!"exception should have been thrown!");
    }
    catch (const ixion::model_context_error& e)
    {
        assert(e.get_error_type() == ixion::model_context_error::sheet_name_conflict);
    }
    catch (...)
    {
        assert(!"wrong exception caught");
    }

    cxt.set_sheet_name(0, "one");
    cxt.set_sheet_name(1, "two");
    cxt.set_sheet_name(2, "three");

    assert(cxt.get_sheet_index("one") == 0);
    assert(cxt.get_sheet_index("two") == 1);
    assert(cxt.get_sheet_index("three") == 2);
}

void test_volatile_function()
{
    IXION_TEST_FUNC_SCOPE;

    model_context cxt{{1048576, 16384}};
    auto resolver = formula_name_resolver::get(formula_name_resolver_t::excel_a1, &cxt);
    assert(resolver);

    cxt.append_sheet("test");

    abs_range_set_t dirty_cells;
    abs_range_set_t modified_cells;

    // Set values into A1:A3.
    cxt.set_numeric_cell(abs_address_t(0,0,0), 1.0);
    cxt.set_numeric_cell(abs_address_t(0,1,0), 2.0);
    cxt.set_numeric_cell(abs_address_t(0,2,0), 3.0);

    // Set formula in A4 that references A1:A3.
    formula_cell* p = insert_formula(cxt, abs_address_t(0,3,0), "SUM(A1:A3)", *resolver);
    assert(p);
    dirty_cells.insert(abs_address_t(0,3,0));

    // Initial full calculation.
    auto sorted = ixion::query_and_sort_dirty_cells(cxt, modified_cells, &dirty_cells);
    ixion::calculate_sorted_cells(cxt, sorted, 0);

    double val = cxt.get_numeric_value(abs_address_t(0,3,0));
    assert(val == 6);

    modified_cells.clear();
    dirty_cells.clear();

    // Modify the value of A2.  This should flag A4 dirty.
    cxt.set_numeric_cell(abs_address_t(0,1,0), 10.0);
    modified_cells.insert(abs_address_t(0,1,0));
    sorted = ixion::query_and_sort_dirty_cells(cxt, modified_cells, &dirty_cells);
    assert(sorted.size() == 1);

    // Partial recalculation.
    ixion::calculate_sorted_cells(cxt, sorted, 0);

    val = cxt.get_numeric_value(abs_address_t(0, 3, 0));
    assert(val == 14);

    modified_cells.clear();
    dirty_cells.clear();

    // Insert a volatile cell into B1.  At this point B1 should be the only dirty cell.
    p = insert_formula(cxt, abs_address_t(0,0,1), "NOW()", *resolver);
    assert(p);
    dirty_cells.insert(abs_address_t(0,0,1));
    sorted = ixion::query_and_sort_dirty_cells(cxt, modified_cells, &dirty_cells);
    assert(sorted.size() == 1);

    // Partial recalc again.
    ixion::calculate_sorted_cells(cxt, sorted, 0);
    double t1 = cxt.get_numeric_value(abs_address_t(0,0,1));

    // Pause for 0.2 second.
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // No modification, but B1 should still be flagged dirty.
    modified_cells.clear();
    dirty_cells.clear();

    sorted = ixion::query_and_sort_dirty_cells(cxt, modified_cells, &dirty_cells);
    assert(sorted.size() == 1);
    ixion::calculate_sorted_cells(cxt, sorted, 0);
    double t2 = cxt.get_numeric_value(abs_address_t(0,0,1));
    double delta = (t2-t1)*24*60*60;
    cout << "delta = " << delta << endl;

    // The delta should be close to 0.2.  It may be a little larger depending
    // on the CPU speed.
    assert(0.2 <= delta && delta <= 0.3);
}

void test_invalid_formula_tokens()
{
    IXION_TEST_FUNC_SCOPE;

    model_context cxt;
    std::string_view invalid_formula("invalid formula");
    std::string_view error_msg("failed to parse formula");

    formula_tokens_t tokens = create_formula_error_tokens(cxt, invalid_formula, error_msg);

    assert(tokens[0].opcode == fop_error);
    assert(tokens.size() == (std::get<string_id_t>(tokens[0].value) + 1));

    assert(tokens[1].opcode == fop_string);
    string_id_t sid = std::get<string_id_t>(tokens[1].value);
    const std::string* s = cxt.get_string(sid);
    assert(invalid_formula == *s);

    assert(tokens[2].opcode == fop_string);
    sid = std::get<string_id_t>(tokens[2].value);
    s = cxt.get_string(sid);
    assert(error_msg == *s);
}

void test_grouped_formula_string_results()
{
    IXION_TEST_FUNC_SCOPE;

    model_context cxt;
    cxt.append_sheet("test");

    auto resolver = formula_name_resolver::get(formula_name_resolver_t::excel_a1, &cxt);
    assert(resolver);

    abs_range_t A1B2(0, 0, 0, 2, 2);

    formula_tokens_t tokens = parse_formula_string(cxt, A1B2.first, *resolver, "\"literal string\"");

    matrix res_value(2, 2, std::string("literal string"));
    formula_result res(std::move(res_value));
    cxt.set_grouped_formula_cells(A1B2, std::move(tokens), std::move(res));

    std::string_view s = cxt.get_string_value(A1B2.last);
    assert(s == "literal string");
}

} // anonymous namespace

int main()
{
    test_size();
    test_string_to_double();
    test_string_pool();
    test_formula_tokens_store();
    test_matrix();
    test_matrix_non_numeric_values();

    test_address();
    test_parse_and_print_expressions();
    test_function_name_resolution();
    test_model_context_storage();
    test_model_context_direct_string_access();
    test_model_context_named_expression();
    test_model_context_iterator_horizontal();
    test_model_context_iterator_horizontal_range();
    test_model_context_iterator_vertical();
    test_model_context_iterator_vertical_range();
    test_model_context_iterator_named_exps();
    test_model_context_fill_down();
    test_model_context_error_value();
    test_model_context_rename_sheets();
    test_volatile_function();
    test_invalid_formula_tokens();
    test_grouped_formula_string_results();

    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
