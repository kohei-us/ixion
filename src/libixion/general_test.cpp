/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "test_global.hpp" // This must be the first header to be included.

#include <ixion/address_range.hpp>
#include <ixion/address.hpp>
#include <ixion/formula_tokens.hpp>
#include <ixion/global.hpp>
#include <ixion/interface/table_handler.hpp>
#include <ixion/matrix.hpp>
#include <ixion/cell.hpp>
#include <ixion/exceptions.hpp>
#include <ixion/table.hpp>

#include <string>

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

    ixion::table_ref_t a;
    a.name = "Table1";
    a.column_first = "Col1";
    a.column_last = "Col2";
    a.areas = ixion::table_area_data;

    ixion::table_ref_t same = a;
    assert(a == same);
    assert(!(a != same));

    ixion::table_ref_t diff_name = a;
    diff_name.name = "other";
    assert(!(a == diff_name));
    assert(a != diff_name);

    ixion::table_ref_t diff_column_first = a;
    diff_column_first.column_first = "other";
    assert(!(a == diff_column_first));
    assert(a != diff_column_first);

    ixion::table_ref_t diff_column_last = a;
    diff_column_last.column_last = "other";
    assert(!(a == diff_column_last));
    assert(a != diff_column_last);

    ixion::table_ref_t diff_areas = a;
    diff_areas.areas = ixion::table_area_headers;
    assert(!(a == diff_areas));
    assert(a != diff_areas);
}

void test_abs_address_range()
{
    IXION_TEST_FUNC_SCOPE;

    // 2 rows x 3 cols on a single sheet.
    ixion::abs_range_t range;
    range.first = ixion::abs_address_t(0, 0, 0);
    range.last  = ixion::abs_address_t(0, 1, 2);

    {
        // Horizontal: row-major order.
        std::vector<ixion::abs_address_t> observed;
        ixion::abs_address_range rng(range, ixion::rc_direction_t::horizontal);
        for (const auto& addr : rng)
            observed.push_back(addr);

        std::vector<ixion::abs_address_t> expected =
        {
            {0, 0, 0}, {0, 0, 1}, {0, 0, 2},
            {0, 1, 0}, {0, 1, 1}, {0, 1, 2},
        };

        assert(observed == expected);
    }

    {
        // Vertical: column-major order.
        std::vector<ixion::abs_address_t> observed;
        ixion::abs_address_range rng(range, ixion::rc_direction_t::vertical);
        for (const auto& addr : rng)
            observed.push_back(addr);

        std::vector<ixion::abs_address_t> expected =
        {
            {0, 0, 0}, {0, 1, 0},
            {0, 0, 1}, {0, 1, 1},
            {0, 0, 2}, {0, 1, 2},
        };

        assert(observed == expected);
    }

    {
        // Walking forward to end, then operator-- back to begin, visits cells
        // in reverse order.
        ixion::abs_address_range rng(range, ixion::rc_direction_t::horizontal);
        auto it = rng.end();
        std::vector<ixion::abs_address_t> observed;
        while (it != rng.begin())
        {
            --it;
            observed.push_back(*it);
        }

        std::vector<ixion::abs_address_t> expected =
        {
            {0, 1, 2}, {0, 1, 1}, {0, 1, 0},
            {0, 0, 2}, {0, 0, 1}, {0, 0, 0},
        };

        assert(observed == expected);
    }

    {
        // Multi-sheet range: iteration spans sheet boundaries.
        ixion::abs_range_t multi;
        multi.first = ixion::abs_address_t(0, 0, 0);
        multi.last  = ixion::abs_address_t(1, 0, 1);

        std::vector<ixion::abs_address_t> observed;
        ixion::abs_address_range rng(multi, ixion::rc_direction_t::horizontal);
        for (const auto& addr : rng)
            observed.push_back(addr);

        std::vector<ixion::abs_address_t> expected =
        {
            {0, 0, 0}, {0, 0, 1},
            {1, 0, 0}, {1, 0, 1},
        };

        assert(observed == expected);
    }

    {
        // operator-> matches operator*.
        ixion::abs_address_range rng(range, ixion::rc_direction_t::horizontal);
        auto it = rng.begin();
        assert(it->sheet  == (*it).sheet);
        assert(it->row    == (*it).row);
        assert(it->column == (*it).column);
    }

    {
        // Post-increment returns the previous position; iterator is copyable.
        ixion::abs_address_range rng(range, ixion::rc_direction_t::horizontal);
        auto it = rng.begin();
        auto prev = it++;
        assert(*prev == ixion::abs_address_t(0, 0, 0));
        assert(*it   == ixion::abs_address_t(0, 0, 1));
    }
}

} // anonymous namespace

int main()
{
    test_size();
    test_formula_opcode_name();
    test_formula_opcode_string();
    test_string_to_double();
    test_formula_tokens_store();
    test_matrix();
    test_matrix_non_numeric_values();

    test_address();
    test_table_t_equality();
    test_abs_address_range();

    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
