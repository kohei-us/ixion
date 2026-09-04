/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "test_global.hpp" // This must be the first header to be included.

#include <ixion/address.hpp>
#include <ixion/cell.hpp>
#include <ixion/exceptions.hpp>
#include <ixion/formula.hpp>
#include <ixion/formula_name_resolver.hpp>
#include <ixion/formula_result.hpp>
#include <ixion/formula_tokens.hpp>
#include <ixion/matrix.hpp>
#include <ixion/model_context.hpp>
#include <ixion/sheet_view.hpp>
#include <ixion/table.hpp>

#include <cassert>
#include <cstdlib>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace {

constexpr auto asc = ixion::sort_order_t::ascending;
constexpr auto desc = ixion::sort_order_t::descending;

/**
 * Set a formula cell with the given expression and a pre-computed result.
 *
 * @param expr Formula expression in A1 notation.
 */
void set_formula(
    ixion::model_context& cxt, const ixion::abs_address_t& pos, std::string_view expr,
    double result)
{
    auto resolver = ixion::formula_name_resolver::get(
        ixion::formula_name_resolver_t::excel_a1, &cxt);
    assert(resolver);

    auto tokens = ixion::parse_formula_string(cxt, pos, *resolver, expr);
    auto ts = ixion::formula_tokens_store::create(std::move(tokens));
    cxt.set_formula_cell(pos, ts, ixion::formula_result(result));
}

void test_sheet_view_create_get_remove()
{
    IXION_TEST_FUNC_SCOPE;

    ixion::model_context cxt{{100, 10}};
    cxt.append_sheet("sheet1");
    cxt.append_sheet("sheet2");

    ixion::sheet_view& view1 = cxt.create_sheet_view(0, "view1");
    assert(view1.get_sheet() == 0);
    assert(view1.get_name() == "view1");

    // get returns the same instance
    assert(cxt.get_sheet_view(0, "view1") == &view1);
    assert(std::as_const(cxt).get_sheet_view(0, "view1") == &view1);

    // unknown names, and a known name on the wrong sheet
    assert(!cxt.get_sheet_view(0, "view2"));
    assert(!cxt.get_sheet_view(1, "view1"));

    // the same name on another sheet is a different view
    ixion::sheet_view& view1_sheet2 = cxt.create_sheet_view(1, "view1");
    assert(&view1_sheet2 != &view1);
    assert(view1_sheet2.get_sheet() == 1);

    cxt.remove_sheet_view(0, "view1");
    assert(!cxt.get_sheet_view(0, "view1"));
    assert(cxt.get_sheet_view(1, "view1") == &view1_sheet2);

    // removing a view that does not exist is a no-op
    cxt.remove_sheet_view(0, "view1");
    cxt.remove_sheet_view(5, "view1");
}

void test_sheet_view_invalid_args()
{
    IXION_TEST_FUNC_SCOPE;

    ixion::model_context cxt{{100, 10}};
    cxt.append_sheet("sheet1");

    ixion::sheet_view& view = cxt.create_sheet_view(0, "view1");

    try
    {
        cxt.create_sheet_view(0, "view1");
        assert(!"model_context_error was expected");
    }
    catch (const ixion::model_context_error& e)
    {
        assert(e.get_error_type() == ixion::model_context_error::sheet_view_name_conflict);
    }

    try
    {
        cxt.create_sheet_view(1, "view1");
        assert(!"std::invalid_argument was expected");
    }
    catch (const std::invalid_argument&)
    {
        // expected
    }

    // the row mapping methods reject rows outside the sheet (100 rows)
    auto expect_out_of_range = [&view](ixion::row_t row)
    {
        try
        {
            view.to_base_row(row);
            assert(!"std::out_of_range was expected");
        }
        catch (const std::out_of_range&)
        {
            // expected
        }

        try
        {
            view.to_view_row(row);
            assert(!"std::out_of_range was expected");
        }
        catch (const std::out_of_range&)
        {
            // expected
        }
    };

    expect_out_of_range(-1);
    expect_out_of_range(100);
}

void test_sheet_view_reads_mirror_base()
{
    IXION_TEST_FUNC_SCOPE;

    ixion::model_context cxt{{100, 10}};
    cxt.append_sheet("sheet1");

    const ixion::abs_address_t A1{0, 0, 0};
    const ixion::abs_address_t A2{0, 1, 0};
    const ixion::abs_address_t A3{0, 2, 0};
    const ixion::abs_address_t A4{0, 3, 0};
    const ixion::abs_address_t A5{0, 4, 0}; // stays empty

    cxt.set_numeric_cell(A1, 1.5);
    cxt.set_boolean_cell(A2, true);
    cxt.set_string_cell(A3, "foo");

    set_formula(cxt, A4, "A1*2", 3.0);

    const ixion::sheet_view& view = cxt.create_sheet_view(0, "view1");

    assert(view.get_celltype(A1) == ixion::cell_t::numeric);
    assert(view.get_numeric_value(A1) == 1.5);

    assert(view.get_celltype(A2) == ixion::cell_t::boolean);
    assert(view.get_boolean_value(A2) == true);
    assert(view.get_numeric_value(A2) == 1.0);

    assert(view.get_celltype(A3) == ixion::cell_t::string);
    assert(view.get_string_value(A3) == "foo");

    assert(view.get_celltype(A4) == ixion::cell_t::formula);
    assert(view.get_numeric_value(A4) == 3.0);
    const ixion::formula_cell* fc = view.get_formula_cell(A4);
    assert(fc);
    assert(fc->get_value(ixion::formula_result_wait_policy_t::throw_exception) == 3.0);

    assert(view.get_celltype(A5) == ixion::cell_t::empty);
    assert(view.get_string_value(A5).empty());
    assert(!view.get_formula_cell(A5));
}

void test_sheet_view_snapshot()
{
    IXION_TEST_FUNC_SCOPE;

    ixion::model_context cxt{{100, 10}};
    cxt.append_sheet("sheet1");

    const ixion::abs_address_t A1{0, 0, 0};
    const ixion::abs_address_t A2{0, 1, 0};
    const ixion::abs_address_t A3{0, 2, 0}; // empty at the creation of the view

    cxt.set_numeric_cell(A1, 1.0);
    cxt.set_numeric_cell(A2, 2.0);

    const ixion::sheet_view& view = cxt.create_sheet_view(0, "view1");

    // Edits to the base sheet after the creation do not show up in the view.
    cxt.set_numeric_cell(A1, 10.0);
    cxt.set_string_cell(A2, "changed");
    cxt.set_numeric_cell(A3, 30.0);

    assert(cxt.get_numeric_value(A1) == 10.0);
    assert(view.get_numeric_value(A1) == 1.0);

    assert(cxt.get_celltype(A2) == ixion::cell_t::string);
    assert(view.get_celltype(A2) == ixion::cell_t::numeric);
    assert(view.get_numeric_value(A2) == 2.0);

    assert(cxt.get_celltype(A3) == ixion::cell_t::numeric);
    assert(view.get_celltype(A3) == ixion::cell_t::empty);
}

void test_sheet_view_sort()
{
    IXION_TEST_FUNC_SCOPE;

    ixion::model_context cxt{{100, 10}};
    cxt.append_sheet("sheet1");

    const ixion::abs_address_t A1{0, 0, 0};
    const ixion::abs_address_t A2{0, 1, 0};
    const ixion::abs_address_t A3{0, 2, 0};
    const ixion::abs_address_t A4{0, 3, 0};
    const ixion::abs_address_t B1{0, 0, 1};
    const ixion::abs_address_t B2{0, 1, 1};
    const ixion::abs_address_t B3{0, 2, 1};
    const ixion::abs_address_t B4{0, 3, 1};
    const ixion::abs_rc_range_t A1_B3{0, 0, 3, 2};

    // layout of the base sheet
    // 0: 3.0 | "c"
    // 1: 1.0 | "a"
    // 2: 2.0 | "b"
    // 3: 9.0 | "z"   <- outside the sorted range
    cxt.set_numeric_cell(A1, 3.0);
    cxt.set_numeric_cell(A2, 1.0);
    cxt.set_numeric_cell(A3, 2.0);
    cxt.set_numeric_cell(A4, 9.0);
    cxt.set_string_cell(B1, "c");
    cxt.set_string_cell(B2, "a");
    cxt.set_string_cell(B3, "b");
    cxt.set_string_cell(B4, "z");

    ixion::sheet_view& view = cxt.create_sheet_view(0, "view1");

    // before any sort the mapping is identity
    assert(view.to_base_row(2) == 2);
    assert(view.to_view_row(2) == 2);

    // sort row indices 0-2 across both columns by column A
    view.sort(A1_B3, {{0, asc}});

    // the view shows the sorted rows
    assert(view.get_numeric_value(A1) == 1.0);
    assert(view.get_numeric_value(A2) == 2.0);
    assert(view.get_numeric_value(A3) == 3.0);
    assert(view.get_string_value(B1) == "a");
    assert(view.get_string_value(B2) == "b");
    assert(view.get_string_value(B3) == "c");

    // the row outside the range stays put
    assert(view.get_numeric_value(A4) == 9.0);
    assert(view.get_string_value(B4) == "z");

    // the base sheet stays untouched
    assert(cxt.get_numeric_value(A1) == 3.0);
    assert(cxt.get_numeric_value(A2) == 1.0);
    assert(cxt.get_numeric_value(A3) == 2.0);
    assert(cxt.get_string_value(B1) == "c");

    // row mapping: view row -> base row, and back
    assert(view.to_base_row(0) == 1);
    assert(view.to_base_row(1) == 2);
    assert(view.to_base_row(2) == 0);
    assert(view.to_base_row(3) == 3);

    assert(view.to_view_row(1) == 0);
    assert(view.to_view_row(2) == 1);
    assert(view.to_view_row(0) == 2);
    assert(view.to_view_row(3) == 3);

    // rows outside the sheet get rejected also after a sort
    try
    {
        view.to_base_row(100);
        assert(!"std::out_of_range was expected");
    }
    catch (const std::out_of_range&)
    {
        // expected
    }

    try
    {
        view.to_view_row(100);
        assert(!"std::out_of_range was expected");
    }
    catch (const std::out_of_range&)
    {
        // expected
    }
}

void test_sheet_view_sort_twice()
{
    IXION_TEST_FUNC_SCOPE;

    ixion::model_context cxt{{100, 10}};
    cxt.append_sheet("sheet1");

    const ixion::abs_address_t A1{0, 0, 0};
    const ixion::abs_address_t A2{0, 1, 0};
    const ixion::abs_address_t A3{0, 2, 0};
    const ixion::abs_address_t B1{0, 0, 1};
    const ixion::abs_address_t B2{0, 1, 1};
    const ixion::abs_address_t B3{0, 2, 1};
    const ixion::abs_rc_range_t A1_B3{0, 0, 3, 2};

    // layout of the base sheet
    // 0: 2.0 | "b"
    // 1: 1.0 | "c"
    // 2: 3.0 | "a"
    cxt.set_numeric_cell(A1, 2.0);
    cxt.set_numeric_cell(A2, 1.0);
    cxt.set_numeric_cell(A3, 3.0);
    cxt.set_string_cell(B1, "b");
    cxt.set_string_cell(B2, "c");
    cxt.set_string_cell(B3, "a");

    ixion::sheet_view& view = cxt.create_sheet_view(0, "view1");

    // first sort by column A ascending: rows become
    // 1: 1.0 | "c"
    // 0: 2.0 | "b"
    // 2: 3.0 | "a"
    view.sort(A1_B3, {{0, asc}});
    assert(view.to_base_row(0) == 1);
    assert(view.to_base_row(1) == 0);
    assert(view.to_base_row(2) == 2);

    // second sort by column B descending re-sorts the current content:
    // rows become
    // 1: 1.0 | "c"
    // 0: 2.0 | "b"
    // 2: 3.0 | "a"
    // i.e. unchanged this time
    view.sort(A1_B3, {{1, desc}});
    assert(view.get_string_value(B1) == "c");
    assert(view.get_string_value(B2) == "b");
    assert(view.get_string_value(B3) == "a");
    assert(view.to_base_row(0) == 1);
    assert(view.to_base_row(1) == 0);
    assert(view.to_base_row(2) == 2);

    // third sort by column B ascending reverses the rows; the mapping still
    // refers to the base rows, not to the rows of the previous sort
    // 2: 3.0 | "a"
    // 0: 2.0 | "b"
    // 1: 1.0 | "c"
    view.sort(A1_B3, {{1, asc}});
    assert(view.get_string_value(B1) == "a");
    assert(view.get_string_value(B2) == "b");
    assert(view.get_string_value(B3) == "c");
    assert(view.get_numeric_value(A1) == 3.0);
    assert(view.get_numeric_value(A2) == 2.0);
    assert(view.get_numeric_value(A3) == 1.0);

    assert(view.to_base_row(0) == 2);
    assert(view.to_base_row(1) == 0);
    assert(view.to_base_row(2) == 1);
    assert(view.to_view_row(2) == 0);
    assert(view.to_view_row(0) == 1);
    assert(view.to_view_row(1) == 2);
}

void test_sheet_view_sort_disjoint_ranges()
{
    IXION_TEST_FUNC_SCOPE;

    ixion::model_context cxt{{100, 10}};
    cxt.append_sheet("sheet1");

    const ixion::abs_address_t A3{0, 2, 0};
    const ixion::abs_address_t A4{0, 3, 0};
    const ixion::abs_address_t A6{0, 5, 0};
    const ixion::abs_address_t A7{0, 6, 0};
    const ixion::abs_address_t A8{0, 7, 0};
    const ixion::abs_address_t A11{0, 10, 0};
    const ixion::abs_address_t A12{0, 11, 0};
    const ixion::abs_address_t A13{0, 12, 0};

    const ixion::abs_rc_range_t A3_A4{2, 0, 2, 1};
    const ixion::abs_rc_range_t A6_A8{5, 0, 3, 1};
    const ixion::abs_rc_range_t A11_A13{10, 0, 3, 1};

    // layout of column A (row indices 2-12)
    //  2: 5.0
    //  3: 4.0
    //  5: 3.0
    //  6: 1.0
    //  7: 2.0
    // 10: 30.0
    // 11: 10.0
    // 12: 20.0
    cxt.set_numeric_cell(A3, 5.0);
    cxt.set_numeric_cell(A4, 4.0);
    cxt.set_numeric_cell(A6, 3.0);
    cxt.set_numeric_cell(A7, 1.0);
    cxt.set_numeric_cell(A8, 2.0);
    cxt.set_numeric_cell(A11, 30.0);
    cxt.set_numeric_cell(A12, 10.0);
    cxt.set_numeric_cell(A13, 20.0);

    ixion::sheet_view& view = cxt.create_sheet_view(0, "view1");

    // sort row indices 5-7, away from the top of the sheet
    view.sort(A6_A8, {{0, asc}});

    assert(view.get_numeric_value(A6) == 1.0);
    assert(view.get_numeric_value(A7) == 2.0);
    assert(view.get_numeric_value(A8) == 3.0);

    assert(view.to_base_row(5) == 6);
    assert(view.to_base_row(6) == 7);
    assert(view.to_base_row(7) == 5);
    assert(view.to_view_row(6) == 5);
    assert(view.to_view_row(7) == 6);
    assert(view.to_view_row(5) == 7);

    // the rows outside the sorted rows map to themselves
    assert(view.to_base_row(0) == 0);
    assert(view.to_base_row(4) == 4);
    assert(view.to_base_row(8) == 8);
    assert(view.to_base_row(99) == 99);
    assert(view.to_view_row(0) == 0);
    assert(view.to_view_row(99) == 99);

    // sort a disjoint range below the sorted rows
    view.sort(A11_A13, {{0, asc}});

    assert(view.to_base_row(10) == 11);
    assert(view.to_base_row(11) == 12);
    assert(view.to_base_row(12) == 10);

    // the rows between the two sorted ranges map to themselves
    assert(view.to_base_row(8) == 8);
    assert(view.to_base_row(9) == 9);

    // the mapping of the first sort stays intact
    assert(view.to_base_row(5) == 6);
    assert(view.to_view_row(5) == 7);

    // sort another disjoint range above the sorted rows
    view.sort(A3_A4, {{0, asc}});

    assert(view.to_base_row(2) == 3);
    assert(view.to_base_row(3) == 2);
    assert(view.to_view_row(2) == 3);
    assert(view.to_view_row(3) == 2);
    assert(view.to_base_row(1) == 1);
    assert(view.to_base_row(4) == 4);
}

void test_sheet_view_sort_formula_group()
{
    IXION_TEST_FUNC_SCOPE;

    ixion::model_context cxt{{100, 10}};
    cxt.append_sheet("sheet1");

    const ixion::abs_address_t A1{0, 0, 0};
    const ixion::abs_address_t A2{0, 1, 0};
    const ixion::abs_address_t A3{0, 2, 0};
    const ixion::abs_address_t A4{0, 3, 0};
    const ixion::abs_address_t B1{0, 0, 1};
    const ixion::abs_address_t B2{0, 1, 1};
    const ixion::abs_address_t B3{0, 2, 1};
    const ixion::abs_address_t B4{0, 3, 1};
    const ixion::abs_rc_range_t A1_B4{0, 0, 4, 2};
    const ixion::abs_range_t B1_B3{0, 0, 1, 3, 1};

    cxt.set_numeric_cell(A1, 3.0);
    cxt.set_numeric_cell(A2, 1.0);
    cxt.set_numeric_cell(A3, 4.0);
    cxt.set_numeric_cell(A4, 2.0);
    cxt.set_string_cell(B4, "x");

    {
        // grouped formula cells B1:B3 = A*10 with cached results
        auto resolver = ixion::formula_name_resolver::get(
            ixion::formula_name_resolver_t::excel_a1, &cxt);
        assert(resolver);

        auto tokens = ixion::parse_formula_string(cxt, B1, *resolver, "A1*10");

        ixion::matrix results(3, 1);
        results.set(0, 0, 30.0);
        results.set(1, 0, 10.0);
        results.set(2, 0, 40.0);

        cxt.set_grouped_formula_cells(B1_B3, std::move(tokens), ixion::formula_result(results));
    }

    ixion::sheet_view& view = cxt.create_sheet_view(0, "view1");

    // layout of the base sheet
    // 0: 3.0 | 30.0 (grouped formula)
    // 1: 1.0 | 10.0 (grouped formula)
    // 2: 4.0 | 40.0 (grouped formula)
    // 3: 2.0 | "x"
    view.sort(A1_B4, {{0, asc}});

    // layout of the view after the sort
    // 1: 1.0 | 10.0 (formula)
    // 3: 2.0 | "x"
    // 0: 3.0 | 30.0 (grouped formula)
    // 2: 4.0 | 40.0 (grouped formula)
    assert(view.to_base_row(0) == 1);
    assert(view.to_base_row(1) == 3);
    assert(view.to_base_row(2) == 0);
    assert(view.to_base_row(3) == 2);

    // the cached results travel with the rows
    assert(view.get_numeric_value(B1) == 10.0);
    assert(view.get_string_value(B2) == "x");
    assert(view.get_numeric_value(B3) == 30.0);
    assert(view.get_numeric_value(B4) == 40.0);

    // the isolated member is no longer grouped
    const ixion::formula_cell* fc = view.get_formula_cell(B1);
    assert(fc);
    assert(!fc->get_group_properties().grouped);

    // Read the base sheet through a const reference; the non-const
    // get_formula_cell() would detach the base column.
    const ixion::model_context& ccxt = cxt;

    // the group on the base sheet stays intact
    const ixion::formula_cell* base_fc = ccxt.get_formula_cell(B1);
    assert(base_fc);
    auto base_identity = base_fc->get_group_properties().identity;

    for (ixion::row_t r = 0; r <= 2; ++r)
    {
        base_fc = ccxt.get_formula_cell({0, r, 1});
        assert(base_fc);
        ixion::formula_group_t group = base_fc->get_group_properties();
        assert(group.grouped);
        assert(group.size.row == 3);
        assert(group.identity == base_identity);
    }

    // the two adjacent members on the view regroup into a new group
    for (ixion::row_t r = 2; r <= 3; ++r)
    {
        fc = view.get_formula_cell({r, 1});
        assert(fc);
        ixion::formula_group_t group = fc->get_group_properties();
        assert(group.grouped);
        assert(group.size.row == 2);
        assert(group.identity != base_identity);
        assert(fc->get_parent_position({0, r, 1}).row == 2);
    }

    // the base sheet keeps its original values
    assert(ccxt.get_numeric_value(B1) == 30.0);
    assert(ccxt.get_numeric_value(B2) == 10.0);
    assert(ccxt.get_numeric_value(B3) == 40.0);
    assert(ccxt.get_string_value(B4) == "x");
}

void test_sheet_view_sort_invalid_args()
{
    IXION_TEST_FUNC_SCOPE;

    ixion::model_context cxt{{100, 10}};
    cxt.append_sheet("sheet1");

    const ixion::abs_rc_range_t A1_B3{0, 0, 3, 2};

    ixion::sheet_view& view = cxt.create_sheet_view(0, "view1");

    try
    {
        view.sort(A1_B3, {}); // no keys
        assert(!"std::invalid_argument was expected");
    }
    catch (const std::invalid_argument&)
    {
        // expected
    }

    try
    {
        view.sort(A1_B3, {{5, asc}}); // key column outside the range
        assert(!"std::invalid_argument was expected");
    }
    catch (const std::invalid_argument&)
    {
        // expected
    }

    try
    {
        view.sort(A1_B3, {{0}}); // key without a direction
        assert(!"std::invalid_argument was expected");
    }
    catch (const std::invalid_argument&)
    {
        // expected
    }

    // a failed sort leaves the mapping identity
    assert(view.to_base_row(1) == 1);
}

void test_sheet_view_sort_table()
{
    IXION_TEST_FUNC_SCOPE;

    ixion::model_context cxt{{100, 10}};
    cxt.append_sheet("sheet1");

    const ixion::abs_address_t A1{0, 0, 0};
    const ixion::abs_address_t A2{0, 1, 0};
    const ixion::abs_address_t A3{0, 2, 0};
    const ixion::abs_address_t A4{0, 3, 0};
    const ixion::abs_address_t A5{0, 4, 0};
    const ixion::abs_address_t B1{0, 0, 1};
    const ixion::abs_address_t B2{0, 1, 1};
    const ixion::abs_address_t B3{0, 2, 1};
    const ixion::abs_address_t B4{0, 3, 1};
    const ixion::abs_address_t B5{0, 4, 1};

    // layout of the table
    // 0: "Name"  | "Score"  <- header row
    // 1: "bob"   | 20.0
    // 2: "amy"   | 30.0
    // 3: "cid"   | 10.0
    // 4: "Total" | 60.0     <- totals row
    cxt.set_string_cell(A1, "Name");
    cxt.set_string_cell(B1, "Score");
    cxt.set_string_cell(A2, "bob");
    cxt.set_numeric_cell(B2, 20.0);
    cxt.set_string_cell(A3, "amy");
    cxt.set_numeric_cell(B3, 30.0);
    cxt.set_string_cell(A4, "cid");
    cxt.set_numeric_cell(B4, 10.0);
    cxt.set_string_cell(A5, "Total");
    cxt.set_numeric_cell(B5, 60.0);

    const ixion::abs_rc_range_t A1_B5{0, 0, 5, 2};

    ixion::table_t tab;
    tab.name = "Scores";
    tab.sheet = 0;
    tab.range = A1_B5;
    tab.columns = {"Name", "Score"};
    tab.totals_row_count = 1;
    cxt.set_table(tab);

    ixion::sheet_view& view = cxt.create_sheet_view(0, "view1");

    view.sort_table("Scores", "Score", desc);

    // layout of the table after the sort
    // 0: "Name"  | "Score"  <- header row
    // 2: "amy"   | 30.0
    // 1: "bob"   | 20.0
    // 3: "cid"   | 10.0
    // 4: "Total" | 60.0     <- totals row

    // the header and totals rows stay in place
    assert(view.get_string_value(A1) == "Name");
    assert(view.get_string_value(B1) == "Score");
    assert(view.get_string_value(A5) == "Total");
    assert(view.get_numeric_value(B5) == 60.0);

    // the data rows are sorted by score in descending order
    assert(view.get_string_value(A2) == "amy");
    assert(view.get_numeric_value(B2) == 30.0);
    assert(view.get_string_value(A3) == "bob");
    assert(view.get_numeric_value(B3) == 20.0);
    assert(view.get_string_value(A4) == "cid");
    assert(view.get_numeric_value(B4) == 10.0);

    assert(view.to_base_row(0) == 0);
    assert(view.to_base_row(1) == 2);
    assert(view.to_base_row(2) == 1);
    assert(view.to_base_row(3) == 3);
    assert(view.to_base_row(4) == 4);

    // the base sheet stays untouched
    assert(cxt.get_string_value(A2) == "bob");
    assert(cxt.get_string_value(A3) == "amy");
    assert(cxt.get_string_value(A4) == "cid");
}

void test_sheet_view_sort_table_invalid_args()
{
    IXION_TEST_FUNC_SCOPE;

    ixion::model_context cxt{{100, 10}};
    cxt.append_sheet("sheet1");
    cxt.append_sheet("sheet2");

    ixion::table_t tab;
    tab.name = "OnSheet2";
    tab.sheet = 1;
    tab.range = ixion::abs_rc_range_t(0, 0, 4, 2); // A1:B4
    tab.columns = {"A", "B"};
    tab.totals_row_count = 0;
    cxt.set_table(tab);

    // table with a header row and a totals row, but no data rows
    tab.name = "NoData";
    tab.sheet = 0;
    tab.range = ixion::abs_rc_range_t(0, 0, 2, 2); // A1:B2
    tab.columns = {"A", "B"};
    tab.totals_row_count = 1;
    cxt.set_table(tab);

    ixion::sheet_view& view = cxt.create_sheet_view(0, "view1");

    auto expect_invalid = [&view](std::string_view table_name, std::string_view column)
    {
        try
        {
            view.sort_table(table_name, column, asc);
            assert(!"std::invalid_argument was expected");
        }
        catch (const std::invalid_argument&)
        {
            // expected
        }
    };

    expect_invalid("NoSuchTable", "A"); // unknown table
    expect_invalid("OnSheet2", "A");    // table on another sheet
    expect_invalid("NoData", "C");      // unknown column

    // sorting a table without data rows does nothing
    view.sort_table("NoData", "A", asc);
    assert(view.to_base_row(1) == 1);
}

void test_sheet_view_dump()
{
    IXION_TEST_FUNC_SCOPE;

    ixion::model_context cxt{{100, 10}};
    cxt.append_sheet("sheet1");

    const ixion::abs_address_t A1{0, 0, 0};
    const ixion::abs_address_t A2{0, 1, 0};
    const ixion::abs_address_t A3{0, 2, 0};
    const ixion::abs_address_t B1{0, 0, 1};
    const ixion::abs_address_t B2{0, 1, 1};
    const ixion::abs_address_t B3{0, 2, 1};
    const ixion::abs_address_t C1{0, 0, 2};
    const ixion::abs_range_t D1_D3{0, 0, 3, 3, 1};
    const ixion::abs_rc_range_t A1_D3{0, 0, 3, 4};

    // an empty view dumps nothing
    {
        const ixion::sheet_view& empty_view = cxt.create_sheet_view(0, "empty");
        std::ostringstream os;
        empty_view.dump(os, ixion::sheet_dump_mode_t::simple);
        assert(os.str().empty());
    }

    // layout of the base sheet
    // 0: 3.0 | "c"   | A1*2 (6.0) | {SUM(A$1:A$3)}@0/3 (6.0)
    // 1: 1.0 | true  |            | {SUM(A$1:A$3)}@1/3 (6.0)
    // 2: 2.0 | "b"   |            | {SUM(A$1:A$3)}@2/3 (6.0)
    cxt.set_numeric_cell(A1, 3.0);
    cxt.set_numeric_cell(A2, 1.0);
    cxt.set_numeric_cell(A3, 2.0);
    cxt.set_string_cell(B1, "c");
    cxt.set_boolean_cell(B2, true);
    cxt.set_string_cell(B3, "b");

    set_formula(cxt, C1, "A1*2", 6.0);

    {
        // grouped formula cells D1:D3 with pre-computed results
        auto resolver = ixion::formula_name_resolver::get(
            ixion::formula_name_resolver_t::excel_a1, &cxt);
        assert(resolver);

        auto tokens = ixion::parse_formula_string(cxt, D1_D3.first, *resolver, "SUM(A$1:A$3)");
        ixion::matrix results(3, 1, 6.0);
        cxt.set_grouped_formula_cells(D1_D3, std::move(tokens), ixion::formula_result(results));
    }

    ixion::sheet_view& view = cxt.create_sheet_view(0, "view1");

    // Sorting by column A moves the rows to 1.0, 2.0, 3.0; the formula in C1
    // lands on the last row and still prints as A1*2, and the group in
    // column D gets ungrouped and regrouped as a whole in the new row order.
    view.sort(A1_D3, {{0, asc}});

    {
        std::ostringstream os;
        view.dump(os, ixion::sheet_dump_mode_t::simple);
        std::string s = os.str();
        assert(!s.ends_with('\n'));
        assert(ixion::test::check_expected_output(
            s, SRCDIR "/test/expected/sheet-view/dump-simple.txt"));
    }

    {
        std::ostringstream os;
        view.dump(os, ixion::sheet_dump_mode_t::verbose);
        assert(ixion::test::check_expected_output(
            os.str(), SRCDIR "/test/expected/sheet-view/dump-verbose.txt"));
    }
}

void test_sheet_view_refresh_no_sort()
{
    IXION_TEST_FUNC_SCOPE;

    ixion::model_context cxt{{100, 10}};
    cxt.append_sheet("sheet1");

    const ixion::abs_address_t A1{0, 0, 0};
    const ixion::abs_address_t A2{0, 1, 0};

    cxt.set_numeric_cell(A1, 1.0);

    ixion::sheet_view& view = cxt.create_sheet_view(0, "view1");

    cxt.set_numeric_cell(A1, 2.0);
    cxt.set_string_cell(A2, "new");

    // An unsorted view refreshes to a plain new snapshot.
    view.refresh();

    assert(view.get_numeric_value(A1) == 2.0);
    assert(view.get_string_value(A2) == "new");
    assert(view.to_base_row(1) == 1);
}

void test_sheet_view_refresh()
{
    IXION_TEST_FUNC_SCOPE;

    ixion::model_context cxt{{100, 10}};
    cxt.append_sheet("sheet1");

    const ixion::abs_address_t A1{0, 0, 0};
    const ixion::abs_address_t A2{0, 1, 0};
    const ixion::abs_address_t A3{0, 2, 0};
    const ixion::abs_address_t B1{0, 0, 1};
    const ixion::abs_address_t B2{0, 1, 1};
    const ixion::abs_address_t B3{0, 2, 1};
    const ixion::abs_address_t B4{0, 3, 1};
    const ixion::abs_rc_range_t A1_B3{0, 0, 3, 2};

    // layout of the base sheet
    // 0: 3.0 | "c"
    // 1: 1.0 | "a"
    // 2: 2.0 | "b"
    cxt.set_numeric_cell(A1, 3.0);
    cxt.set_numeric_cell(A2, 1.0);
    cxt.set_numeric_cell(A3, 2.0);
    cxt.set_string_cell(B1, "c");
    cxt.set_string_cell(B2, "a");
    cxt.set_string_cell(B3, "b");

    ixion::sheet_view& view = cxt.create_sheet_view(0, "view1");
    view.sort(A1_B3, {{0, asc}});

    // layout of the view after the sort
    // 0: 1.0 | "a"   <- base row 1
    // 1: 2.0 | "b"   <- base row 2
    // 2: 3.0 | "c"   <- base row 0
    assert(view.get_numeric_value(A1) == 1.0);

    // Edit the base sheet: a key cell inside the sorted range, and a cell
    // outside of it.  The view does not see them until the refresh.
    cxt.set_numeric_cell(A2, 7.0);
    cxt.set_string_cell(B4, "outside");

    assert(view.get_numeric_value(A1) == 1.0);
    assert(view.get_string_value(B4).empty());

    view.refresh();

    // Each view row keeps showing its base row with the refreshed content;
    // the new key value 7.0 does NOT trigger a re-sort.
    assert(view.get_numeric_value(A1) == 7.0); // base row 1
    assert(view.get_numeric_value(A2) == 2.0); // base row 2
    assert(view.get_numeric_value(A3) == 3.0); // base row 0
    assert(view.get_string_value(B1) == "a");
    assert(view.get_string_value(B2) == "b");
    assert(view.get_string_value(B3) == "c");
    assert(view.get_string_value(B4) == "outside");

    // the row mapping stays the same
    assert(view.to_base_row(0) == 1);
    assert(view.to_base_row(1) == 2);
    assert(view.to_base_row(2) == 0);

    // the base sheet stays untouched by the refresh
    assert(cxt.get_numeric_value(A1) == 3.0);
    assert(cxt.get_string_value(B1) == "c");
}

void test_sheet_view_refresh_formula()
{
    IXION_TEST_FUNC_SCOPE;

    ixion::model_context cxt{{100, 10}};
    cxt.append_sheet("sheet1");

    const ixion::abs_address_t A1{0, 0, 0};
    const ixion::abs_address_t A2{0, 1, 0};
    const ixion::abs_address_t B1{0, 0, 1};
    const ixion::abs_address_t B2{0, 1, 1};
    const ixion::abs_rc_range_t A1_B2{0, 0, 2, 2};

    cxt.set_numeric_cell(A1, 2.0);
    cxt.set_numeric_cell(A2, 1.0);

    set_formula(cxt, B1, "A1*2", 4.0);

    ixion::sheet_view& view = cxt.create_sheet_view(0, "view1");

    // the sort moves the formula cell from B1 to B2
    view.sort(A1_B2, {{0, asc}});
    assert(view.get_numeric_value(B2) == 4.0);

    // Replace the formula cell on the base sheet with an updated result.
    set_formula(cxt, B1, "A1*2", 8.0);

    view.refresh();

    // the updated result shows up at the permuted position
    assert(view.get_numeric_value(B2) == 8.0);
    assert(view.get_celltype(B1) == ixion::cell_t::empty);
}

void test_sheet_view_refresh_two_sorts()
{
    IXION_TEST_FUNC_SCOPE;

    ixion::model_context cxt{{100, 10}};
    cxt.append_sheet("sheet1");

    const ixion::abs_address_t A1{0, 0, 0};
    const ixion::abs_address_t A2{0, 1, 0};
    const ixion::abs_address_t A3{0, 2, 0};
    const ixion::abs_address_t B1{0, 0, 1};
    const ixion::abs_address_t B2{0, 1, 1};
    const ixion::abs_address_t B3{0, 2, 1};
    const ixion::abs_rc_range_t A1_A3{0, 0, 3, 1};
    const ixion::abs_rc_range_t B1_B3{0, 1, 3, 1};

    // layout of the base sheet
    // 0: 3.0 | "b"
    // 1: 1.0 | "c"
    // 2: 2.0 | "a"
    cxt.set_numeric_cell(A1, 3.0);
    cxt.set_numeric_cell(A2, 1.0);
    cxt.set_numeric_cell(A3, 2.0);
    cxt.set_string_cell(B1, "b");
    cxt.set_string_cell(B2, "c");
    cxt.set_string_cell(B3, "a");

    ixion::sheet_view& view = cxt.create_sheet_view(0, "view1");

    // Two sorts scoped to single columns; each permutes only its own column.
    view.sort(A1_A3, {{0, asc}});
    view.sort(B1_B3, {{1, asc}});

    assert(view.get_numeric_value(A1) == 1.0);
    assert(view.get_numeric_value(A2) == 2.0);
    assert(view.get_numeric_value(A3) == 3.0);
    assert(view.get_string_value(B1) == "a");
    assert(view.get_string_value(B2) == "b");
    assert(view.get_string_value(B3) == "c");

    // The refresh replays both sorts in order and reproduces the same
    // content from the unchanged base sheet.
    view.refresh();

    assert(view.get_numeric_value(A1) == 1.0);
    assert(view.get_numeric_value(A2) == 2.0);
    assert(view.get_numeric_value(A3) == 3.0);
    assert(view.get_string_value(B1) == "a");
    assert(view.get_string_value(B2) == "b");
    assert(view.get_string_value(B3) == "c");
}

} // anonymous namespace

int main()
{
    test_sheet_view_create_get_remove();
    test_sheet_view_invalid_args();
    test_sheet_view_reads_mirror_base();
    test_sheet_view_snapshot();
    test_sheet_view_sort();
    test_sheet_view_sort_twice();
    test_sheet_view_sort_disjoint_ranges();
    test_sheet_view_sort_formula_group();
    test_sheet_view_sort_invalid_args();
    test_sheet_view_sort_table();
    test_sheet_view_sort_table_invalid_args();
    test_sheet_view_dump();
    test_sheet_view_refresh_no_sort();
    test_sheet_view_refresh();
    test_sheet_view_refresh_formula();
    test_sheet_view_refresh_two_sorts();

    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
