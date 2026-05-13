/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "test_global.hpp" // This must be the first header to be included.
#include "deprecated.hpp"

#include <ixion/formula_name_resolver.hpp>
#include <ixion/address.hpp>
#include <ixion/formula.hpp>
#include <ixion/model_cell_range.hpp>
#include <ixion/model_context.hpp>
#include <ixion/model_iterator.hpp>
#include <ixion/named_expressions_iterator.hpp>
#include <ixion/global.hpp>
#include <ixion/interface/table_handler.hpp>
#include <ixion/config.hpp>
#include <ixion/matrix.hpp>
#include <ixion/cell.hpp>
#include <ixion/cell_access.hpp>
#include <ixion/formula_result.hpp>
#include <ixion/exceptions.hpp>
#include <ixion/table.hpp>

#include <string>
#include <cstring>
#include <ranges>
#include <sstream>
#include <thread>

namespace {

void test_size()
{
    IXION_TEST_FUNC_SCOPE;

    std::cout << "test size" << std::endl;
    std::cout << "* int: " << sizeof(int) << std::endl;
    std::cout << "* long: " << sizeof(long) << std::endl;
    std::cout << "* double: " << sizeof(double) << std::endl;
    std::cout << "* size_t: " << sizeof(size_t) << std::endl;
    std::cout << "* string_id_t: " << sizeof(ixion::string_id_t)
        << " (min:" << ixion::string_id_t{}.value
        << "; max:" << ixion::empty_string_id.value << ")" << std::endl;
    std::cout << "* cell_t: " << sizeof(ixion::cell_t) << std::endl;
    std::cout << "* ixion::formula_cell: " << sizeof(ixion::formula_cell) << std::endl;
    std::cout << "* ixion::formula_tokens_t: " << sizeof(ixion::formula_tokens_t) << std::endl;
}

void test_formula_opcode_name()
{
    IXION_TEST_FUNC_SCOPE;

    constexpr std::tuple<ixion::fopcode_t, std::string_view> checks[] =
    {
        { ixion::fop_unknown, "unknown" },
        { ixion::fop_single_ref, "single-ref" },
        { ixion::fop_range_ref, "range-ref" },
        { ixion::fop_table_ref, "table-ref" },
        { ixion::fop_named_expression, "named-expression" },
        { ixion::fop_string, "string" },
        { ixion::fop_value, "value" },
        { ixion::fop_function, "function" },
        { ixion::fop_error, "error" },
        { ixion::fop_plus, "plus" },
        { ixion::fop_minus, "minus" },
        { ixion::fop_divide, "divide" },
        { ixion::fop_multiply, "multiply" },
        { ixion::fop_exponent, "exponent" },
        { ixion::fop_concat, "concat" },
        { ixion::fop_equal, "equal" },
        { ixion::fop_not_equal, "not-equal" },
        { ixion::fop_less, "less" },
        { ixion::fop_greater, "greater" },
        { ixion::fop_less_equal, "less-equal" },
        { ixion::fop_greater_equal, "greater-equal" },
        { ixion::fop_open, "open" },
        { ixion::fop_close, "close" },
        { ixion::fop_sep, "sep" },
        { ixion::fop_array_row_sep, "array-row-sep" },
        { ixion::fop_array_open, "array-open" },
        { ixion::fop_array_close, "array-close" },
        { ixion::fop_invalid_formula, "invalid-formula" },
    };

    for (const auto& [oc, expected] : checks)
    {
        if (auto actual = ixion::get_formula_opcode_name(oc); actual != expected)
        {
            std::cout << "expected opcode name was '" << expected
                << "' but the actual value was '" << actual
                << "' (opcode=" << int(oc) << ")"
                << std::endl;

            assert(false);
        }
    }
}

void test_formula_opcode_string()
{
    IXION_TEST_FUNC_SCOPE;

    constexpr std::tuple<ixion::fopcode_t, std::string_view> checks[] =
    {
        { ixion::fop_plus, "+", },
        { ixion::fop_minus, "-", },
        { ixion::fop_divide, "/", },
        { ixion::fop_multiply, "*", },
        { ixion::fop_exponent, "^", },
        { ixion::fop_concat, "&", },
        { ixion::fop_equal, "=", },
        { ixion::fop_not_equal, "<>", },
        { ixion::fop_less, "<", },
        { ixion::fop_greater, ">", },
        { ixion::fop_less_equal, "<=", },
        { ixion::fop_greater_equal, ">=", },
        { ixion::fop_open, "(", },
        { ixion::fop_close, ")", },
        { ixion::fop_array_open, "{", },
        { ixion::fop_array_close, "}", },
    };

    for (const auto& [oc, expected] : checks)
    {
        if (auto actual = ixion::get_formula_opcode_string(oc); actual != expected)
        {
            std::cout << "expected opcode string was '" << expected
                << "' but the actual value was '" << actual
                << "' (opcode=" << int(oc) << ")"
                << std::endl;

            assert(false);
        }
    }
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
        double v = ixion::to_double(tests[i].s);
        assert(v == tests[i].v);
    }
}

void test_string_pool()
{
    IXION_TEST_FUNC_SCOPE;

    ixion::model_context cxt;

    ixion::string_id_t s_table1 = cxt.append_string("Table1");
    ixion::string_id_t s_table2 = cxt.append_string("Table2");
    ixion::string_id_t s_cat = cxt.append_string("Category");
    ixion::string_id_t s_val = cxt.append_string("Value");
    cxt.dump_strings();

    // Verify each id round-trips to the original string value.
    const std::string* p = cxt.get_string(s_table1);
    assert(p && *p == "Table1");
    p = cxt.get_string(s_table2);
    assert(p && *p == "Table2");
    p = cxt.get_string(s_cat);
    assert(p && *p == "Category");
    p = cxt.get_string(s_val);
    assert(p && *p == "Value");
}

void test_string_pool_duplicate_strings()
{
    IXION_TEST_FUNC_SCOPE;

    ixion::model_context cxt;

    ixion::string_id_t s_value1 = cxt.append_string("value");
    ixion::string_id_t s_value2 = cxt.append_string("value");
    assert(s_value1 != s_value2);
    ixion::string_id_t s_empty1 = cxt.append_string("");
    ixion::string_id_t s_empty2 = cxt.append_string("");
    assert(s_empty1 != s_empty2);

    {
        const auto* s = cxt.get_string(s_value1);
        assert(s);
        assert(*s == "value");
    }

    {
        const auto* s = cxt.get_string(s_value2);
        assert(s);
        assert(*s == "value");
    }

    {
        const auto* s = cxt.get_string(s_empty1);
        assert(s);
        assert(s->empty());
    }

    {
        const auto* s = cxt.get_string(s_empty2);
        assert(s);
        assert(s->empty());
    }
}

void test_formula_tokens_store()
{
    IXION_TEST_FUNC_SCOPE;

    ixion::formula_tokens_store_ptr_t p = ixion::formula_tokens_store::create();
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

    ixion::numeric_matrix num_mtx(2, 2);

    for (const check& c : checks)
        num_mtx(c.row, c.col) = c.val;

    for (const check& c : checks)
        assert(num_mtx(c.row, c.col) == c.val);

    ixion::matrix mtx(num_mtx);

    for (const check& c : checks)
    {
        ixion::matrix::element e = mtx.get(c.row, c.col);
        assert(e.type == ixion::matrix::element_type::numeric);
        assert(std::get<double>(e.value) == c.val);
    }
}

void test_matrix_non_numeric_values()
{
    IXION_TEST_FUNC_SCOPE;

    ixion::matrix mtx(2, 2);
    mtx.set(0, 0, 1.1);
    mtx.set(1, 0, ixion::formula_error_t::division_by_zero);
    mtx.set(0, 1, std::string("foo"));
    mtx.set(1, 1, true);

    assert(mtx.get_numeric(0, 0) == 1.1);

    ixion::matrix::element elem = mtx.get(1, 0);
    assert(elem.type == ixion::matrix::element_type::error);
    assert(std::get<ixion::formula_error_t>(elem.value) == ixion::formula_error_t::division_by_zero);

    elem = mtx.get(0, 1);
    assert(elem.type == ixion::matrix::element_type::string);
    assert(std::get<std::string_view>(elem.value) == "foo");

    elem = mtx.get(1, 1);
    assert(elem.type == ixion::matrix::element_type::boolean);
    assert(std::get<bool>(elem.value) == true);
}

void test_address()
{
    IXION_TEST_FUNC_SCOPE;

    {
        ixion::address_t addr(-1, 0, 0, false, false, false);
        ixion::abs_address_t pos(1, 0, 0);
        ixion::abs_address_t abs_addr = addr.to_abs(pos);
        assert(abs_addr.sheet == 0 && abs_addr.row == 0 && abs_addr.column == 0);

        ixion::abs_address_t pos_invalid_sheet(ixion::invalid_sheet, 2, 3);
        auto test = addr.to_abs(pos_invalid_sheet);
        assert(test.sheet == ixion::invalid_sheet);
        assert(test.row == 2);
        assert(test.column == 3);
    }


    // Default constructor makes valid address.
    assert(ixion::abs_address_t().valid());
    assert(ixion::abs_range_t().valid());

    // These are invalid addresses.
    assert(!ixion::abs_address_t(ixion::abs_address_t::invalid).valid());
    assert(!ixion::abs_range_t(ixion::abs_range_t::invalid).valid());

    {
        ixion::abs_range_t range(1, 1, 2, 3, 3);
        assert(range.first.sheet == 1);
        assert(range.first.row == 1);
        assert(range.first.column == 2);
        assert(range.last.sheet == 1);
        assert(range.last.row == 3);
        assert(range.last.column == 4);

        ixion::abs_range_t range2(range);
        assert(range2 == range);

        ixion::abs_rc_range_t rc_range(range);
        assert(rc_range.first.row == 1);
        assert(rc_range.first.column == 2);
        assert(rc_range.last.row == 3);
        assert(rc_range.last.column == 4);
    }
}

void test_table_t_equality()
{
    IXION_TEST_FUNC_SCOPE;

    ixion::table_t a;
    a.name = "Table1";
    a.column_first = "Col1";
    a.column_last = "Col2";
    a.areas = ixion::table_area_data;

    ixion::table_t same = a;
    assert(a == same);
    assert(!(a != same));

    ixion::table_t diff_name = a;
    diff_name.name = "other";
    assert(!(a == diff_name));
    assert(a != diff_name);

    ixion::table_t diff_column_first = a;
    diff_column_first.column_first = "other";
    assert(!(a == diff_column_first));
    assert(a != diff_column_first);

    ixion::table_t diff_column_last = a;
    diff_column_last.column_last = "other";
    assert(!(a == diff_column_last));
    assert(a != diff_column_last);

    ixion::table_t diff_areas = a;
    diff_areas.areas = ixion::table_area_headers;
    assert(!(a == diff_areas));
    assert(a != diff_areas);
}

bool check_formula_expression(
    ixion::model_context& cxt, const ixion::formula_name_resolver& resolver, const char* p)
{
    size_t n = strlen(p);
    std::cout << "testing formula expression '" << p << "'" << std::endl;

    auto tokens = ixion::parse_formula_string(cxt, ixion::abs_address_t(), resolver, {p, n});
    auto expression = ixion::print_formula_tokens(cxt, ixion::abs_address_t(), resolver, tokens);

    int res = std::strcmp(p, expression.data());
    if (res)
    {
        std::cout << "formula expressions differ: '" << p << "' (before) -> '"
            << expression << "' (after)" << std::endl;
        return false;
    }

    std::ostringstream os;
    for (const auto& t : tokens)
        os << ixion::print_formula_token(cxt, ixion::abs_address_t(), resolver, t);
    std::string individual_tokens = os.str();

    if (expression != individual_tokens)
    {
        std::cout << "whole expression differs from individual token strings:" << std::endl
             << "  * expression='" << expression << "'" << std::endl
             << "  * individual-tokens='" << individual_tokens << "'" << std::endl;
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

    ixion::model_context cxt;
    cxt.append_sheet("Test");
    cxt.append_string("Table1");
    cxt.append_string("Category");
    cxt.append_string("Value");
    cxt.append_sheet("Ying & Yang"); // name with '&'

    auto resolver = ixion::formula_name_resolver::get(ixion::formula_name_resolver_t::excel_a1, &cxt);
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

    resolver = ixion::formula_name_resolver::get(ixion::formula_name_resolver_t::excel_r1c1, &cxt);
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

    resolver = ixion::formula_name_resolver::get(ixion::formula_name_resolver_t::odff, &cxt);
    assert(resolver);

    auto cfg = cxt.get_config();
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

    ixion::model_context cxt;
    cxt.append_sheet("Test");
    auto resolver = ixion::formula_name_resolver::get(ixion::formula_name_resolver_t::excel_a1, &cxt);
    size_t n = std::size(valid_names);
    for (size_t i = 0; i < n; ++i)
    {
        const char* name = valid_names[i];
        std::cout << "valid name: " << name << std::endl;
        ixion::formula_name_t t = resolver->resolve(name, ixion::abs_address_t());
        assert(t.type == ixion::formula_name_t::function);
    }

    n = std::size(invalid_names);
    for (size_t i = 0; i < n; ++i)
    {
        const char* name = invalid_names[i];
        std::cout << "invalid name: " << name << std::endl;
        ixion::formula_name_t t = resolver->resolve(name, ixion::abs_address_t());
        assert(t.type != ixion::formula_name_t::function);
    }
}

ixion::formula_cell* insert_formula(
    ixion::model_context& cxt, const ixion::abs_address_t& pos, const char* exp,
    const ixion::formula_name_resolver& resolver)
{
    auto tokens = ixion::parse_formula_string(cxt, pos, resolver, exp);
    auto ts = ixion::formula_tokens_store::create();
    ts->get() = std::move(tokens);
    auto* p_inserted = cxt.set_formula_cell(pos, ts);
    assert(p_inserted);
    ixion::register_formula_cell(cxt, pos);
    auto* p = cxt.get_formula_cell(pos);
    assert(p);
    assert(p == p_inserted);
    return p;
}

void test_model_context_storage()
{
    IXION_TEST_FUNC_SCOPE;

    {
        ixion::model_context cxt;
        auto resolver = ixion::formula_name_resolver::get(ixion::formula_name_resolver_t::excel_a1, &cxt);
        assert(resolver);

        cxt.append_sheet("test");

        // Test empty cell access.
        ixion::cell_access ca = cxt.get_cell_access(ixion::abs_address_t(0, 0, 0));
        assert(ca.get_type() == ixion::cell_t::empty);
        assert(ca.get_value_type() == ixion::cell_value_t::empty);

        // String value on an empty cell should be an empty string.
        std::string_view s = ca.get_string_value();
        assert(s.empty());

        // Likewise...
        s = cxt.get_string_value(ixion::abs_address_t(0, 0, 0));
        assert(s.empty());

        // Test storage of numeric values.
        volatile double val = 0.1;
        for (ixion::col_t col = 0; col < 3; ++col)
        {
            for (ixion::row_t row = 0; row < 3; ++row)
            {
                ixion::abs_address_t pos(0, row, col);
                cxt.set_numeric_cell(pos, val);
                double test = cxt.get_numeric_value(pos);
                assert(test == val);

                ca = cxt.get_cell_access(pos);
                assert(ca.get_type() == ixion::cell_t::numeric);
                assert(ca.get_value_type() == ixion::cell_value_t::numeric);
                test = ca.get_numeric_value();
                assert(test == val);

                val += 0.2;
            }
        }

        // Test formula cells.
        ixion::abs_address_t pos(0,3,0);
        const char* exp = "SUM(1,2,3)";
        auto tokens = ixion::parse_formula_string(cxt, pos, *resolver, exp);
        auto ts = ixion::formula_tokens_store::create();
        ts->get() = std::move(tokens);
        ixion::formula_cell* p_inserted = cxt.set_formula_cell(pos, ts);
        assert(p_inserted);
        ixion::formula_cell* p = cxt.get_formula_cell(pos);
        assert(p);
        assert(p_inserted == p);
        p->interpret(cxt, pos);

        ca = cxt.get_cell_access(pos);
        assert(ca.get_type() == ixion::cell_t::formula);
        assert(ca.get_value_type() == ixion::cell_value_t::numeric);
        assert(ca.get_numeric_value() == 6.0);
    }

    {
        ixion::model_context cxt;
        auto resolver = ixion::formula_name_resolver::get(ixion::formula_name_resolver_t::excel_a1, &cxt);
        assert(resolver);

        cxt.append_sheet("test");
        std::string exp = "1";
        cxt.set_formula_cell(
            ixion::abs_address_t(0,0,0),
            ixion::parse_formula_string(cxt, ixion::abs_address_t(0,0,0), *resolver, exp));
        cxt.set_formula_cell(
            ixion::abs_address_t(0,2,0),
            ixion::parse_formula_string(cxt, ixion::abs_address_t(0,2,0), *resolver, exp));
        cxt.set_formula_cell(
            ixion::abs_address_t(0,1,0),
            ixion::parse_formula_string(cxt, ixion::abs_address_t(0,1,0), *resolver, exp));
    }

    {
        // Test data area.
        ixion::model_context cxt;
        cxt.append_sheet("test");

        ixion::abs_range_t area = cxt.get_data_range(0);
        assert(!area.valid());

        cxt.set_numeric_cell(ixion::abs_address_t(0, 6, 5), 1.1);
        area = cxt.get_data_range(0);
        assert(area.first == area.last);
        assert(area.first.sheet == 0);
        assert(area.first.row == 6);
        assert(area.first.column == 5);

        cxt.set_numeric_cell(ixion::abs_address_t(0, 2, 3), 1.1);
        area = cxt.get_data_range(0);
        assert(area.first.sheet == 0);
        assert(area.first.row == 2);
        assert(area.first.column == 3);
        assert(area.last.sheet == 0);
        assert(area.last.row == 6);
        assert(area.last.column == 5);

        cxt.set_numeric_cell(ixion::abs_address_t(0, 7, 1), 1.1);
        area = cxt.get_data_range(0);
        assert(area.first.sheet == 0);
        assert(area.first.row == 2);
        assert(area.first.column == 1);
        assert(area.last.sheet == 0);
        assert(area.last.row == 7);
        assert(area.last.column == 5);

        // This shouldn't change the data range.
        cxt.set_numeric_cell(ixion::abs_address_t(0, 5, 5), 1.1);
        ixion::abs_range_t test = cxt.get_data_range(0);
        assert(test == area);
    }

    {
        // Fill up the document model and make sure the data range is still
        // correct.
        const ixion::row_t row_size = 5;
        const ixion::col_t col_size = 4;
        ixion::model_context cxt({row_size, col_size});
        cxt.append_sheet("test");
        for (ixion::row_t row = 0; row < row_size; ++row)
            for (ixion::col_t col = 0; col < col_size; ++col)
                cxt.set_numeric_cell(ixion::abs_address_t(0,row,col), 1.0);

        ixion::abs_range_t test = cxt.get_data_range(0);

        assert(test.first.sheet == 0);
        assert(test.first.row == 0);
        assert(test.first.column == 0);
        assert(test.last.sheet == 0);
        assert(test.last.row == row_size-1);
        assert(test.last.column == col_size-1);
    }

    {
        const ixion::row_t row_size = 5;
        const ixion::col_t col_size = 4;
        ixion::model_context cxt({row_size, col_size});
        cxt.append_sheet("test");
        cxt.set_numeric_cell(ixion::abs_address_t(0,0,0), 1.0);
        cxt.set_numeric_cell(ixion::abs_address_t(0,row_size-1,0), 1.0);
        cxt.set_numeric_cell(ixion::abs_address_t(0,row_size/2,col_size/2), 1.0);

        ixion::abs_range_t test = cxt.get_data_range(0);

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

    ixion::model_context cxt{{400, 20}};
    cxt.append_sheet("test");

    // regular string cell
    ixion::abs_address_t B2(0, 1, 1);
    cxt.set_string_cell(B2, "string cell");
    std::string_view s = cxt.get_string_value(B2);
    assert(s == "string cell");

    ixion::cell_access ca = cxt.get_cell_access(B2);
    assert(ca.get_type() == ixion::cell_t::string);
    assert(ca.get_value_type() == ixion::cell_value_t::string);
    s = ca.get_string_value();
    assert(s == "string cell");

    // formula cell containing a string result.
    ixion::abs_address_t C4(0, 3, 2);
    auto resolver = ixion::formula_name_resolver::get(ixion::formula_name_resolver_t::calc_a1, &cxt);
    assert(resolver);

    // Insert a formula containing one literal string token.
    auto tokens = ixion::parse_formula_string(cxt, C4, *resolver, "\"string value in formula\"");
    assert(tokens.size() == 1);
    cxt.set_formula_cell(C4, std::move(tokens));
    // no need to register formula cell since it does not reference other cells.

    ixion::abs_range_set_t formula_cells{C4};
    auto sorted = ixion::query_and_sort_dirty_cells(cxt, ixion::abs_range_set_t(), &formula_cells);
    ixion::calculate_sorted_cells(cxt, sorted, 1);

    s = cxt.get_string_value(C4);
    assert(s == "string value in formula");

    ca = cxt.get_cell_access(C4);
    assert(ca.get_type() == ixion::cell_t::formula);
    assert(ca.get_value_type() == ixion::cell_value_t::string);
    s = ca.get_string_value();
    assert(s == "string value in formula");
}

void test_model_context_inline_string()
{
    IXION_TEST_FUNC_SCOPE;

    using namespace ixion;

    ixion::model_context cxt{{400, 20}};
    cxt.append_sheet("test");

    // Inline writes do not pollute the indexed pool. append_string should keep
    // returning consecutive ids regardless of intervening inline-cell writes.
    auto id_a = cxt.append_string("A");
    assert(cxt.get_string_count() == 1);

    ixion::abs_address_t inline_addr1(0, 0, 1); // B1
    cxt.set_string_cell(inline_addr1, "raw inline 1");
    assert(cxt.get_string_count() == 1); // unchanged

    auto id_b = cxt.append_string("B");
    assert(cxt.get_string_count() == 2);
    assert(id_b.value == id_a.value + 1);

    ixion::abs_address_t sid_addr(0, 1, 1); // B2
    cxt.set_string_cell(sid_addr, id_a); // string ID variant

    // get_string_value() handles both kinds
    assert(cxt.get_string_value(inline_addr1) == "raw inline 1");
    assert(cxt.get_string_value(sid_addr) == "A");

    // get_string_identifier() returns the real ID for the string ID cell and
    // empty_string_id for the inline string cell
    assert(cxt.get_string_identifier(sid_addr) == id_a);
    assert(cxt.get_string_identifier(inline_addr1) == empty_string_id);

    // cell_t::string for both
    assert(cxt.get_celltype(inline_addr1) == cell_t::string);
    assert(cxt.get_celltype(sid_addr) == cell_t::string);

    // cell_access test cases
    {
        cell_access ca = cxt.get_cell_access(inline_addr1);
        assert(ca.get_type() == cell_t::string);
        assert(ca.get_value_type() == cell_value_t::string);
        assert(ca.get_string_value() == "raw inline 1");
        assert(ca.get_string_identifier() == empty_string_id);
    }

    {
        cell_access ca = cxt.get_cell_access(sid_addr);
        assert(ca.get_type() == cell_t::string);
        assert(ca.get_value_type() == cell_value_t::string);
        assert(ca.get_string_value() == "A");
        assert(ca.get_string_identifier() == id_a);
    }

    {
        // walk() exposes both string block types.

        std::vector<column_block_t> block_types;
        auto cb = [&block_types](
            col_t, row_t, row_t, const column_block_shape_t& node)
        {
            block_types.push_back(node.type);
            return true;
        };
        abs_rc_range_t range;
        range.first.column = 1;
        range.last.column = 1;
        range.first.row = 0;
        range.last.row = 1;
        cxt.walk(0, range, cb);

        bool saw_indexed = std::find(block_types.begin(), block_types.end(),
            column_block_t::string) != block_types.end();

        bool saw_inline = std::find(block_types.begin(), block_types.end(),
            column_block_t::inline_string) != block_types.end();

        assert(saw_indexed);
        assert(saw_inline);
    }

    {
        // model_iterator emits std::string_view for both kinds

        abs_rc_range_t range;
        range.set_all_columns();
        range.set_all_rows();
        IXION_DEPRECATED_DECL_PUSH
        model_iterator it = cxt.get_model_iterator(0, rc_direction_t::vertical, range);
        IXION_DEPRECATED_DECL_POP

        bool saw_inline_value = false;
        bool saw_indexed_value = false;

        for (; it.has(); it.next())
        {
            const auto& c = it.get();
            if (c.type != cell_t::string)
                continue;

            auto sv = std::get<std::string_view>(c.value);
            if (sv == "raw inline 1")
                saw_inline_value = true;
            else if (sv == "A")
                saw_indexed_value = true;
        }

        assert(saw_inline_value);
        assert(saw_indexed_value);
    }

    // writing the same inline text into two cells produces views into the same
    // stored string instance
    abs_address_t inline_addr2(0, 2, 1); // B3
    cxt.set_string_cell(inline_addr2, "raw inline 1");
    std::string_view sv1 = cxt.get_string_value(inline_addr1);
    std::string_view sv2 = cxt.get_string_value(inline_addr2);
    assert(sv1 == "raw inline 1");
    assert(sv2 == "raw inline 1");
    assert(sv1.data() == sv2.data());

    {
        // set_cell_values() stores strings as inline values

        model_context cxt2{{10, 5}};
        cxt2.append_sheet("values");
        std::size_t indexed_before = cxt2.get_string_count();
        cxt2.set_cell_values(0, {
            { "alpha", "beta", "gamma" },
            { "delta", "epsilon", "zeta" },
        });
        assert(cxt2.get_string_count() == indexed_before);
        assert(cxt2.get_string_value(abs_address_t(0, 0, 0)) == "alpha");
        assert(cxt2.get_string_value(abs_address_t(0, 1, 2)) == "zeta");
    }
}

void test_model_context_named_expression()
{
    IXION_TEST_FUNC_SCOPE;

    ixion::model_context cxt{{400, 20}};
    cxt.append_sheet("test");
    auto resolver = ixion::formula_name_resolver::get(ixion::formula_name_resolver_t::calc_a1, &cxt);
    assert(resolver);

    ixion::abs_address_t B3(0, 2, 1);

    struct test_case
    {
        std::string name;
        std::string formula;
        ixion::abs_address_t origin;
    };

    std::vector<test_case> tcs = {
        { "LeftAndAbove", "A3+B2", B3 },
        { "SumAboveRow", "SUM(A2:D2)", B3 },
    };

    for (const test_case& tc : tcs)
    {
        auto tokens = ixion::parse_formula_string(cxt, tc.origin, *resolver, tc.formula);
        auto test = ixion::print_formula_tokens(cxt, tc.origin, *resolver, tokens);
        assert(test == tc.formula);

        cxt.set_named_expression(tc.name, tc.origin, std::move(tokens));
    }

    for (const test_case& tc : tcs)
    {
        const ixion::named_expression_t* exp = cxt.get_named_expression(0, tc.name);
        assert(exp);
        assert(exp->origin == tc.origin);
        auto test = ixion::print_formula_tokens(cxt, exp->origin, *resolver, exp->tokens);
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
        ixion::abs_address_t origin;
        std::string formula = "1+2";

        if (tc.valid)
        {
            auto tokens = ixion::parse_formula_string(cxt, origin, *resolver, formula);
            cxt.set_named_expression(tc.name, origin, std::move(tokens));

            tokens = ixion::parse_formula_string(cxt, origin, *resolver, formula);
            cxt.set_named_expression(0, tc.name, origin, std::move(tokens));
        }
        else
        {
            try
            {
                auto tokens = ixion::parse_formula_string(cxt, origin, *resolver, formula);
                cxt.set_named_expression(tc.name, origin, std::move(tokens));
                assert(!"named expression with invalid name should have been rejected!");
            }
            catch (const ixion::model_context_error& e)
            {
                assert(e.get_error_type() == ixion::model_context_error::invalid_named_expression);
            }

            try
            {
                auto tokens = ixion::parse_formula_string(cxt, origin, *resolver, formula);
                cxt.set_named_expression(0, tc.name, origin, std::move(tokens));
                assert(!"named expression with invalid name should have been rejected!");
            }
            catch (const ixion::model_context_error& e)
            {
                assert(e.get_error_type() == ixion::model_context_error::invalid_named_expression);
            }
        }
    }
}

IXION_DEPRECATED_DECL_PUSH

bool check_model_iterator_output(
    ixion::model_iterator& iter, const std::vector<ixion::model_iterator::cell>& checks)
{
    for (const ixion::model_iterator::cell& c : checks)
    {
        if (!iter.has())
        {
            std::cerr << "a cell value was expected, but none found." << std::endl;
            return false;
        }

        if (iter.get() != c)
        {
            std::cerr << "unexpected cell value: expected=" << c << "; observed=" << iter.get() << std::endl;
            return false;
        }

        iter.next();
    }

    if (iter.has())
    {
        std::cerr << "an additional cell value was found, but none was expected." << std::endl;
        return false;
    }

    return true;
}

void test_model_context_iterator_horizontal()
{
    IXION_TEST_FUNC_SCOPE;

    const ixion::row_t row_size = 5;
    const ixion::col_t col_size = 2;
    ixion::model_context cxt{{row_size, col_size}};
    ixion::model_iterator iter;

    ixion::abs_rc_range_t whole_range;
    whole_range.set_all_columns();
    whole_range.set_all_rows();

    // It should not crash or throw an exception on empty model.
    iter = cxt.get_model_iterator(0, ixion::rc_direction_t::horizontal, whole_range);
    assert(!iter.has());

    // Insert an actual sheet and try again.

    cxt.append_sheet("empty sheet");
    iter = cxt.get_model_iterator(0, ixion::rc_direction_t::horizontal, whole_range);

    // Make sure the cell position iterates correctly.
    size_t cell_count = 0;
    for (ixion::row_t row = 0; row < row_size; ++row)
    {
        for (ixion::col_t col = 0; col < col_size; ++cell_count, ++col, iter.next())
        {
            assert(iter.has());
            assert(iter.get().row == row);
            assert(iter.get().col == col);
            assert(iter.get().type == ixion::cell_t::empty);
        }
    }

    assert(!iter.has()); // There should be no more cells on this sheet.
    assert(cell_count == 10);

    cxt.append_sheet("values");
    cxt.set_string_cell(ixion::abs_address_t(1, 0, 0), "F1");
    cxt.set_string_cell(ixion::abs_address_t(1, 0, 1), "F2");
    cxt.set_boolean_cell(ixion::abs_address_t(1, 1, 0), true);
    cxt.set_boolean_cell(ixion::abs_address_t(1, 1, 1), false);
    cxt.set_numeric_cell(ixion::abs_address_t(1, 2, 0), 3.14);
    cxt.set_numeric_cell(ixion::abs_address_t(1, 2, 1), -12.5);

    auto resolver = ixion::formula_name_resolver::get(ixion::formula_name_resolver_t::excel_a1, &cxt);
    ixion::abs_range_set_t modified_cells;
    ixion::abs_address_t pos(1, 3, 0);
    ixion::formula_tokens_t tokens = ixion::parse_formula_string(cxt, pos, *resolver, "SUM(1, 2, 3)");
    ixion::formula_cell* p = cxt.set_formula_cell(pos, std::move(tokens));
    assert(p);
    const ixion::formula_tokens_t& t = p->get_tokens()->get();
    assert(t.size() == 8); // there should be 8 tokens.
    ixion::register_formula_cell(cxt, pos, p);
    modified_cells.insert(pos);

    pos.column = 1;
    tokens = ixion::parse_formula_string(cxt, pos, *resolver, "5 + 6 - 7");
    p = cxt.set_formula_cell(pos, std::move(tokens));
    ixion::register_formula_cell(cxt, pos, p);
    modified_cells.insert(pos);

    // Calculate the formula cells.
    auto sorted = ixion::query_and_sort_dirty_cells(cxt, ixion::abs_range_set_t(), &modified_cells);
    ixion::calculate_sorted_cells(cxt, sorted, 1);

    std::vector<ixion::model_iterator::cell> checks =
    {
        // row, column, value
        { 0, 0, std::string_view("F1") },
        { 0, 1, std::string_view("F2") },
        { 1, 0, true },
        { 1, 1, false },
        { 2, 0, 3.14 },
        { 2, 1, -12.5 },
        { 3, 0, cxt.get_formula_cell(ixion::abs_address_t(1, 3, 0)) },
        { 3, 1, cxt.get_formula_cell(ixion::abs_address_t(1, 3, 1)) },
        { 4, 0 },
        { 4, 1 },
    };

    // Iterator and check the individual cell values.
    iter = cxt.get_model_iterator(1, ixion::rc_direction_t::horizontal, whole_range);
    assert(check_model_iterator_output(iter, checks));
}

void test_model_context_iterator_horizontal_range()
{
    IXION_TEST_FUNC_SCOPE;

    nullptr_t empty = nullptr;
    ixion::model_context cxt{{10, 5}};
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
    ixion::abs_rc_range_t range;
    range.set_all_columns();
    range.first.row = 0;
    range.last.row = 1;

    ixion::model_iterator iter = cxt.get_model_iterator(0, ixion::rc_direction_t::horizontal, range);

    std::vector<ixion::model_iterator::cell> checks =
    {
        // row, column, value
        { 0, 0, std::string_view("F1") },
        { 0, 1, std::string_view("F2") },
        { 0, 2, std::string_view("F3") },
        { 0, 3, std::string_view("F4") },
        { 0, 4, std::string_view("F5") },
        { 1, 0, 1.0 },
        { 1, 1, true },
        { 1, 2, std::string_view("s1") },
        { 1, 3 },
        { 1, 4 },
    };

    assert(check_model_iterator_output(iter, checks));

    // Only iterate over rows 2:4.
    range.first.row = 2;
    range.last.row = 4;
    iter = cxt.get_model_iterator(0, ixion::rc_direction_t::horizontal, range);

    checks =
    {
        // row, column, value
        { 2, 0, 1.1 },
        { 2, 1, false },
        { 2, 2 },
        { 2, 3, std::string_view("s2") },
        { 2, 4 },
        { 3, 0, 1.2 },
        { 3, 1, false },
        { 3, 2 },
        { 3, 3, std::string_view("s3") },
        { 3, 4 },
        { 4, 0, 1.3 },
        { 4, 1, true },
        { 4, 2 },
        { 4, 3, std::string_view("s4") },
        { 4, 4 },
    };

    assert(check_model_iterator_output(iter, checks));

    // Only iterate over columns 1:3 and only down to row 4.
    range.set_all_rows();
    range.first.column = 1;
    range.last.column = 3;
    range.last.row = 4;
    iter = cxt.get_model_iterator(0, ixion::rc_direction_t::horizontal, range);

    checks =
    {
        // row, column, value
        { 0, 1, std::string_view("F2") },
        { 0, 2, std::string_view("F3") },
        { 0, 3, std::string_view("F4") },
        { 1, 1, true },
        { 1, 2, std::string_view("s1") },
        { 1, 3 },
        { 2, 1, false },
        { 2, 2 },
        { 2, 3, std::string_view("s2") },
        { 3, 1, false },
        { 3, 2 },
        { 3, 3, std::string_view("s3") },
        { 4, 1, true },
        { 4, 2 },
        { 4, 3, std::string_view("s4") },
    };

    assert(check_model_iterator_output(iter, checks));
}

void test_model_context_iterator_vertical()
{
    IXION_TEST_FUNC_SCOPE;

    const ixion::row_t row_size = 5;
    const ixion::col_t col_size = 2;
    ixion::model_context cxt{{row_size, col_size}};
    ixion::model_iterator iter;

    ixion::abs_rc_range_t whole_range;
    whole_range.set_all_columns();
    whole_range.set_all_rows();

    // It should not crash or throw an exception on empty model.
    iter = cxt.get_model_iterator(0, ixion::rc_direction_t::vertical, whole_range);
    assert(!iter.has());

    // Insert an actual sheet and try again.

    cxt.append_sheet("empty sheet");
    iter = cxt.get_model_iterator(0, ixion::rc_direction_t::vertical, whole_range);

    // Make sure the cell position iterates correctly.
    size_t cell_count = 0;
    for (ixion::col_t col = 0; col < col_size; ++col)
    {
        for (ixion::row_t row = 0; row < row_size; ++cell_count, ++row, iter.next())
        {
            const ixion::model_iterator::cell& cell = iter.get();
            assert(iter.has());
            assert(cell.row == row);
            assert(cell.col == col);
            assert(cell.type == ixion::cell_t::empty);
        }
    }

    assert(!iter.has()); // There should be no more cells on this sheet.
    assert(cell_count == 10);

    cxt.append_sheet("values");
    cxt.set_string_cell(ixion::abs_address_t(1, 0, 0), "F1");
    cxt.set_string_cell(ixion::abs_address_t(1, 0, 1), "F2");
    cxt.set_boolean_cell(ixion::abs_address_t(1, 1, 0), true);
    cxt.set_boolean_cell(ixion::abs_address_t(1, 1, 1), false);
    cxt.set_numeric_cell(ixion::abs_address_t(1, 2, 0), 3.14);
    cxt.set_numeric_cell(ixion::abs_address_t(1, 2, 1), -12.5);

    auto resolver = ixion::formula_name_resolver::get(ixion::formula_name_resolver_t::excel_a1, &cxt);
    ixion::abs_range_set_t modified_cells;
    ixion::abs_address_t pos(1, 3, 0);
    auto tokens = ixion::parse_formula_string(cxt, pos, *resolver, "SUM(1, 2, 3)");
    cxt.set_formula_cell(pos, std::move(tokens));
    ixion::register_formula_cell(cxt, pos);
    modified_cells.insert(pos);

    pos.column = 1;
    tokens = ixion::parse_formula_string(cxt, pos, *resolver, "5 + 6 - 7");
    cxt.set_formula_cell(pos, std::move(tokens));
    ixion::register_formula_cell(cxt, pos);
    modified_cells.insert(pos);

    // Calculate the formula cells.
    auto sorted = ixion::query_and_sort_dirty_cells(cxt, ixion::abs_range_set_t(), &modified_cells);
    ixion::calculate_sorted_cells(cxt, sorted, 1);

    std::vector<ixion::model_iterator::cell> checks =
    {
        // row, column, value
        { 0, 0, std::string_view("F1") },
        { 1, 0, true },
        { 2, 0, 3.14 },
        { 3, 0, cxt.get_formula_cell(ixion::abs_address_t(1, 3, 0)) },
        { 4, 0 },

        { 0, 1, std::string_view("F2") },
        { 1, 1, false },
        { 2, 1, -12.5 },
        { 3, 1, cxt.get_formula_cell(ixion::abs_address_t(1, 3, 1)) },
        { 4, 1 },
    };

    iter = cxt.get_model_iterator(1, ixion::rc_direction_t::vertical, whole_range);
    assert(check_model_iterator_output(iter, checks));
}

void test_model_context_iterator_vertical_range()
{
    IXION_TEST_FUNC_SCOPE;

    nullptr_t empty = nullptr;
    ixion::model_context cxt{{10, 5}};
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
    ixion::abs_rc_range_t range;
    range.set_all_columns();
    range.set_all_rows();
    range.last.row = 1;

    ixion::model_iterator iter = cxt.get_model_iterator(0, ixion::rc_direction_t::vertical, range);

    std::vector<ixion::model_iterator::cell> checks =
    {
        // row, column, value
        { 0, 0, std::string_view("F1") },
        { 1, 0, 1.0 },
        { 0, 1, std::string_view("F2") },
        { 1, 1, true },
        { 0, 2, std::string_view("F3") },
        { 1, 2, std::string_view("s1") },
        { 0, 3, std::string_view("F4") },
        { 1, 3 },
        { 0, 4, std::string_view("F5") },
        { 1, 4 },
    };

    assert(check_model_iterator_output(iter, checks));

    // Iterate over the bottom 2 rows.

    range.set_all_rows();
    range.first.row = 8;
    iter = cxt.get_model_iterator(0, ixion::rc_direction_t::vertical, range);

    checks =
    {
        // row, column, value
        { 8, 0, 1.7 },
        { 9, 0, 1.8 },
        { 8, 1, 199.9 },
        { 9, 1, 299.9 },
        { 8, 2 },
        { 9, 2 },
        { 8, 3, std::string_view("s8") },
        { 9, 3, std::string_view("s9") },
        { 8, 4 },
        { 9, 4, std::string_view("end") },
    };

    assert(check_model_iterator_output(iter, checks));

    // Iterate over the bottom-left corners.
    range.last.column = 2;
    iter = cxt.get_model_iterator(0, ixion::rc_direction_t::vertical, range);

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
    range.last.column = ixion::column_unset;
    range.first.row = ixion::row_unset;
    range.last.row = 1;
    iter = cxt.get_model_iterator(0, ixion::rc_direction_t::vertical, range);

    checks =
    {
        { 0, 3, std::string_view("F4") },
        { 1, 3 },
        { 0, 4, std::string_view("F5") },
        { 1, 4 },
    };

    assert(check_model_iterator_output(iter, checks));

    // Iterate over only one cell in the middle.
    range.first.row = 5;
    range.last.row = 5;
    range.first.column = 3;
    range.last.column = 3;

    iter = cxt.get_model_iterator(0, ixion::rc_direction_t::vertical, range);
    checks =
    {
        { 5, 3, std::string_view("s5") },
    };

    assert(check_model_iterator_output(iter, checks));
}

IXION_DEPRECATED_DECL_POP

void test_model_context_cell_range_horizontal()
{
    IXION_TEST_FUNC_SCOPE;

    const ixion::row_t row_size = 5;
    const ixion::col_t col_size = 2;
    ixion::model_context cxt{{row_size, col_size}};

    ixion::abs_rc_range_t whole_range;
    whole_range.set_all_columns();
    whole_range.set_all_rows();

    {
        // range-for over an empty model: zero iterations, begin() == end()
        auto cells = cxt.iterate_cells(0, ixion::rc_direction_t::horizontal, whole_range);
        assert(cells.begin() == cells.end());
        std::size_t count = 0;
        for ([[maybe_unused]] const auto& c : cells)
            ++count;
        assert(count == 0);
    }

    cxt.append_sheet("empty sheet");

    {
        // empty sheet: every cell visited, all empty
        std::size_t count = 0;
        for (const auto& cell : cxt.iterate_cells(0, ixion::rc_direction_t::horizontal, whole_range))
        {
            assert(cell.type == ixion::cell_t::empty);
            ++count;
        }
        assert(count == static_cast<std::size_t>(row_size) * col_size);
    }

    cxt.append_sheet("values");
    // Use set_string_cell so the strings go through the string pool (ID-based
    // storage), exercising the iterator's element_type_string branch.
    cxt.set_string_cell(ixion::abs_address_t(1, 0, 0), "F1");
    cxt.set_string_cell(ixion::abs_address_t(1, 0, 1), "F2");
    cxt.set_boolean_cell(ixion::abs_address_t(1, 1, 0), true);
    cxt.set_boolean_cell(ixion::abs_address_t(1, 1, 1), false);
    cxt.set_numeric_cell(ixion::abs_address_t(1, 2, 0), 3.14);
    cxt.set_numeric_cell(ixion::abs_address_t(1, 2, 1), -12.5);

    std::vector<ixion::model_cell_range::cell> observed;
    for (const auto& cell : cxt.iterate_cells(1, ixion::rc_direction_t::horizontal, whole_range))
        observed.push_back(cell);

    std::vector<ixion::model_cell_range::cell> expected =
    {
        // row, column, value
        { 0, 0, "F1" },
        { 0, 1, "F2" },
        { 1, 0, true },
        { 1, 1, false },
        { 2, 0, 3.14 },
        { 2, 1, -12.5 },
        { 3, 0 },
        { 3, 1 },
        { 4, 0 },
        { 4, 1 },
    };

    assert(observed == expected);
}

void test_model_context_cell_range_vertical()
{
    IXION_TEST_FUNC_SCOPE;

    ixion::model_context cxt{{5, 2}};
    cxt.append_sheet("values");
    cxt.set_cell_values(0, {
        { "F1", "F2" },
        { true, false },
        {  3.14, -12.5 },
    });

    ixion::abs_rc_range_t whole_range;
    whole_range.set_all_columns();
    whole_range.set_all_rows();

    std::vector<ixion::model_cell_range::cell> observed;
    for (const auto& cell : cxt.iterate_cells(0, ixion::rc_direction_t::vertical, whole_range))
        observed.push_back(cell);

    std::vector<ixion::model_cell_range::cell> expected =
    {
        // row, column, value
        { 0, 0, "F1" },
        { 1, 0, true },
        { 2, 0, 3.14 },
        { 3, 0 },
        { 4, 0 },

        { 0, 1, "F2" },
        { 1, 1, false },
        { 2, 1, -12.5 },
        { 3, 1 },
        { 4, 1 },
    };

    assert(observed == expected);
}

void test_model_context_cell_range_iterator_semantics()
{
    IXION_TEST_FUNC_SCOPE;

    ixion::model_context cxt{{3, 2}};
    cxt.append_sheet("values");
    cxt.set_cell_values(0, {
        { 1.0, 4.0 },
        { 2.0, 5.0 },
        { 3.0, 6.0 },
    });

    ixion::abs_rc_range_t whole_range;
    whole_range.set_all_columns();
    whole_range.set_all_rows();

    auto cells = cxt.iterate_cells(0, ixion::rc_direction_t::horizontal, whole_range);

    // std::ranges::distance walks begin()..end() and reports cell count.
    assert(std::ranges::distance(cells.begin(), cells.end()) == 6);

    // operator-> exposes the underlying cell.
    auto it = cells.begin();
    assert(it->type == ixion::cell_t::numeric);
    assert(std::get<double>(it->value) == 1.0);

    // Prefix ++ advances to the next cell in row-major order.
    ++it;
    assert(it != cells.end());
    assert(std::get<double>(it->value) == 4.0);

    // ++ past the last cell compares equal to end().
    for (int i = 0; i < 5; ++i)
        ++it;
    assert(it == cells.end());

    // The sentinel value compares equal to a past-the-end iterator.
    assert(it == ixion::model_cell_range::sentinel{});
}

void test_model_context_iterator_named_exps()
{
    IXION_TEST_FUNC_SCOPE;

    struct check
    {
        std::string name;
        const ixion::named_expression_t* exp;
    };

    ixion::model_context cxt{{100, 10}};
    cxt.append_sheet("test1");
    cxt.append_sheet("test2");

    ixion::named_expressions_iterator iter;
    assert(!iter.has());
    assert(iter.size() == 0);

    iter = cxt.get_named_expressions_iterator();
    assert(!iter.has());
    assert(iter.size() == 0);

    auto resolver = ixion::formula_name_resolver::get(ixion::formula_name_resolver_t::calc_a1, &cxt);
    assert(resolver);

    auto tokenize = [&](const char* p) -> ixion::formula_tokens_t
    {
        return ixion::parse_formula_string(cxt, ixion::abs_address_t(), *resolver, p);
    };

    auto validate = [](ixion::named_expressions_iterator _iter, const std::vector<check>& _expected) -> bool
    {
        if (_iter.size() != _expected.size())
        {
            std::cout << "iterator's size() returns wrong value." << std::endl;
            return false;
        }

        for (const check& c : _expected)
        {
            if (!_iter.has())
            {
                std::cout << "iterator has no more element, but it is expected to." << std::endl;
                return false;
            }

            if (c.name != *_iter.get().name)
            {
                std::cout << "names differ: expected='" << c.name << "'; actual='" << *_iter.get().name << std::endl;
                return false;
            }

            if (c.exp != _iter.get().expression)
            {
                std::cout << "expressions differ." << std::endl;
                return false;
            }

            _iter.next();
        }

        if (_iter.has())
        {
            std::cout << "the iterator has more elements, but it is not expected to." << std::endl;
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
    ixion::model_context cxt{{100, 10}};
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

    ixion::abs_address_t pos(0, 1, 0);
    cxt.fill_down_cells(pos, 2);

    assert(cxt.get_numeric_value(ixion::abs_address_t(0, 1, 0)) == 12.3);
    assert(cxt.get_numeric_value(ixion::abs_address_t(0, 2, 0)) == 12.3);
    assert(cxt.get_numeric_value(ixion::abs_address_t(0, 3, 0)) == 12.3);
    assert(cxt.is_empty(ixion::abs_address_t(0, 4, 0)));

    pos.column = 1;
    cxt.fill_down_cells(pos, 1);
    assert(cxt.get_boolean_value(ixion::abs_address_t(0, 1, 1)) == true);
    assert(cxt.get_boolean_value(ixion::abs_address_t(0, 2, 1)) == true);
    assert(cxt.is_empty(ixion::abs_address_t(0, 3, 1)));

    pos.column = 2;
    assert(cxt.get_string_value(pos) == "foo");
    cxt.fill_down_cells(pos, 3);
    assert(cxt.get_string_value(ixion::abs_address_t(0, 2, 2)) == "foo");
    assert(cxt.get_string_value(ixion::abs_address_t(0, 3, 2)) == "foo");
    assert(cxt.get_string_value(ixion::abs_address_t(0, 4, 2)) == "foo");
    assert(cxt.is_empty(ixion::abs_address_t(0, 5, 2)));

    pos.column = 3;
    cxt.fill_down_cells(pos, 2);
    assert(cxt.is_empty(pos));
    assert(cxt.is_empty(ixion::abs_address_t(0, 2, 3)));
    assert(cxt.is_empty(ixion::abs_address_t(0, 3, 3)));
    assert(cxt.get_numeric_value(ixion::abs_address_t(0, 4, 3)) == 1.1);
}

void test_model_context_error_value()
{
    IXION_TEST_FUNC_SCOPE;

    ixion::model_context cxt{{100, 10}};
    cxt.append_sheet("test");

    auto resolver = ixion::formula_name_resolver::get(ixion::formula_name_resolver_t::excel_a1, &cxt);
    assert(resolver);

    ixion::abs_address_t pos(0,3,0);
    const char* exp = "10/0";
    ixion::formula_tokens_t tokens = ixion::parse_formula_string(cxt, pos, *resolver, exp);
    ixion::formula_cell* fc = cxt.set_formula_cell(pos, std::move(tokens));
    fc->interpret(cxt, pos);

    ixion::cell_access ca = cxt.get_cell_access(pos);
    assert(ca.get_type() == ixion::cell_t::formula);
    assert(ca.get_value_type() == ixion::cell_value_t::error);
    assert(ca.get_error_value() == ixion::formula_error_t::division_by_zero);
}

void test_model_context_rename_sheets()
{
    IXION_TEST_FUNC_SCOPE;

    ixion::model_context cxt{{100, 10}};
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

    ixion::model_context cxt{{1048576, 16384}};
    auto resolver = ixion::formula_name_resolver::get(ixion::formula_name_resolver_t::excel_a1, &cxt);
    assert(resolver);

    cxt.append_sheet("test");

    ixion::abs_range_set_t dirty_cells;
    ixion::abs_range_set_t modified_cells;

    // Set values into A1:A3.
    cxt.set_numeric_cell(ixion::abs_address_t(0,0,0), 1.0);
    cxt.set_numeric_cell(ixion::abs_address_t(0,1,0), 2.0);
    cxt.set_numeric_cell(ixion::abs_address_t(0,2,0), 3.0);

    // Set formula in A4 that references A1:A3.
    auto* p = insert_formula(cxt, ixion::abs_address_t(0,3,0), "SUM(A1:A3)", *resolver);
    assert(p);
    dirty_cells.insert(ixion::abs_address_t(0,3,0));

    // Initial full calculation.
    auto sorted = ixion::query_and_sort_dirty_cells(cxt, modified_cells, &dirty_cells);
    ixion::calculate_sorted_cells(cxt, sorted, 0);

    double val = cxt.get_numeric_value(ixion::abs_address_t(0,3,0));
    assert(val == 6);

    modified_cells.clear();
    dirty_cells.clear();

    // Modify the value of A2.  This should flag A4 dirty.
    cxt.set_numeric_cell(ixion::abs_address_t(0,1,0), 10.0);
    modified_cells.insert(ixion::abs_address_t(0,1,0));
    sorted = ixion::query_and_sort_dirty_cells(cxt, modified_cells, &dirty_cells);
    assert(sorted.size() == 1);

    // Partial recalculation.
    ixion::calculate_sorted_cells(cxt, sorted, 0);

    val = cxt.get_numeric_value(ixion::abs_address_t(0, 3, 0));
    assert(val == 14);

    modified_cells.clear();
    dirty_cells.clear();

    // Insert a volatile cell into B1.  At this point B1 should be the only dirty cell.
    p = insert_formula(cxt, ixion::abs_address_t(0,0,1), "NOW()", *resolver);
    assert(p);
    dirty_cells.insert(ixion::abs_address_t(0,0,1));
    sorted = ixion::query_and_sort_dirty_cells(cxt, modified_cells, &dirty_cells);
    assert(sorted.size() == 1);

    // Partial recalc again.
    ixion::calculate_sorted_cells(cxt, sorted, 0);
    double t1 = cxt.get_numeric_value(ixion::abs_address_t(0,0,1));

    // Pause for 0.2 second.
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // No modification, but B1 should still be flagged dirty.
    modified_cells.clear();
    dirty_cells.clear();

    sorted = ixion::query_and_sort_dirty_cells(cxt, modified_cells, &dirty_cells);
    assert(sorted.size() == 1);
    ixion::calculate_sorted_cells(cxt, sorted, 0);
    double t2 = cxt.get_numeric_value(ixion::abs_address_t(0,0,1));
    double delta = (t2-t1)*24*60*60;
    std::cout << "delta = " << delta << std::endl;

    // The delta should be close to 0.2.  It may be a little larger depending
    // on the CPU speed.
    assert(0.2 <= delta && delta <= 0.3);
}

void test_invalid_formula_tokens()
{
    IXION_TEST_FUNC_SCOPE;

    ixion::model_context cxt;
    std::string_view invalid_formula("invalid formula");
    std::string_view error_msg("failed to parse formula");

    ixion::formula_tokens_t tokens =
        ixion::create_formula_error_tokens(cxt, invalid_formula, error_msg);

    assert(tokens[0].opcode == ixion::fop_invalid_formula);
    assert(tokens.size() == (std::get<ixion::string_id_t>(tokens[0].value).value + 1));

    assert(tokens[1].opcode == ixion::fop_string);
    assert(invalid_formula == std::get<std::string_view>(tokens[1].value));

    assert(tokens[2].opcode == ixion::fop_string);
    assert(error_msg == std::get<std::string_view>(tokens[2].value));
}

void test_grouped_formula_string_results()
{
    IXION_TEST_FUNC_SCOPE;

    ixion::model_context cxt;
    cxt.append_sheet("test");

    auto resolver = ixion::formula_name_resolver::get(ixion::formula_name_resolver_t::excel_a1, &cxt);
    assert(resolver);

    ixion::abs_range_t A1B2(0, 0, 0, 2, 2);

    ixion::formula_tokens_t tokens =
        ixion::parse_formula_string(cxt, A1B2.first, *resolver, "\"literal string\"");

    ixion::matrix res_value(2, 2, std::string("literal string"));
    ixion::formula_result res(std::move(res_value));
    cxt.set_grouped_formula_cells(A1B2, std::move(tokens), std::move(res));

    std::string_view s = cxt.get_string_value(A1B2.last);
    assert(s == "literal string");
}

} // anonymous namespace

int main()
{
    test_size();
    test_formula_opcode_name();
    test_formula_opcode_string();
    test_string_to_double();
    test_string_pool();
    test_string_pool_duplicate_strings();
    test_formula_tokens_store();
    test_matrix();
    test_matrix_non_numeric_values();

    test_address();
    test_table_t_equality();
    test_parse_and_print_expressions();
    test_function_name_resolution();
    test_model_context_storage();
    test_model_context_direct_string_access();
    test_model_context_inline_string();
    test_model_context_named_expression();
    test_model_context_iterator_horizontal();
    test_model_context_iterator_horizontal_range();
    test_model_context_iterator_vertical();
    test_model_context_iterator_vertical_range();
    test_model_context_cell_range_horizontal();
    test_model_context_cell_range_vertical();
    test_model_context_cell_range_iterator_semantics();
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
