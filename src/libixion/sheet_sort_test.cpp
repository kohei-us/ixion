/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "test_global.hpp" // This must be the first header to be included.
#include "sheet_sort.hpp"
#include "sheet_store.hpp"
#include "calc_status.hpp"
#include "column_store_type.hpp"

#include <ixion/address.hpp>
#include <ixion/cell.hpp>
#include <ixion/formula.hpp>
#include <ixion/formula_name_resolver.hpp>
#include <ixion/formula_result.hpp>
#include <ixion/formula_tokens.hpp>
#include <ixion/model_context.hpp>

#include <cassert>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {

constexpr auto wait_policy = ixion::formula_result_wait_policy_t::throw_exception;

ixion::abs_rc_range_t to_range(
    ixion::row_t row1, ixion::col_t col1, ixion::row_t row2, ixion::col_t col2)
{
    ixion::abs_rc_range_t range;
    range.first.row = row1;
    range.first.column = col1;
    range.last.row = row2;
    range.last.column = col2;
    return range;
}

ixion::formula_tokens_store_ptr_t make_tokens(
    ixion::model_context& cxt, const ixion::abs_address_t& pos, std::string_view expr)
{
    auto resolver = ixion::formula_name_resolver::get(
        ixion::formula_name_resolver_t::excel_r1c1, &cxt);
    assert(resolver);

    auto tokens = ixion::parse_formula_string(cxt, pos, *resolver, expr);
    return ixion::formula_tokens_store::create(tokens);
}

/**
 * Set a standalone formula cell in column B with the given expression and
 * cached result.
 *
 * @param expr Formula expression in R1C1 notation.
 */
void set_formula(
    ixion::model_context& cxt, ixion::column_store_t& col, ixion::row_t row,
    std::string_view expr, double result)
{
    auto tokens = make_tokens(cxt, {0, row, 1}, expr);
    auto fc = std::make_unique<ixion::formula_cell>(tokens);
    fc->set_result_cache(result);
    col.set(row, fc.release());
}

/**
 * Set a group of formula cells in column B sharing one calc status and one
 * token store, with cached results of 10, 20, ... in member order.
 *
 * @param expr Formula expression in R1C1 notation shared by all members.
 */
void set_group(
    ixion::model_context& cxt, ixion::column_store_t& col, ixion::row_t row, ixion::row_t size,
    std::string_view expr)
{
    ixion::calc_status_ptr_t cs(new ixion::calc_status({size, 1}));
    auto ts = make_tokens(cxt, {0, row, 1}, expr);

    for (ixion::row_t k = 0; k < size; ++k)
    {
        auto fc = std::make_unique<ixion::formula_cell>(k, 0, cs, ts);
        fc->set_result_cache(ixion::formula_result((k + 1) * 10.0));
        col.set(row + k, fc.release());
    }
}

double formula_value(const ixion::column_store_t& col, ixion::row_t row)
{
    return col.get<ixion::formula_cell*>(row)->get_result_cache(wait_policy).get_value();
}

} // anonymous namespace

void test_sheet_sort_cross_type_order()
{
    IXION_TEST_FUNC_SCOPE;

    ixion::model_context cxt;
    ixion::detail::sheet_store store(10, 1);

    ixion::string_id_t sid = cxt.add_string("foo");

    store[0].set(0, sid.value);
    store[0].set(1, 2.5);
    store[0].set(2, true);
    store[0].set(3, 1.0);
    store[0].set(4, false);
    // The cell at row index 5 stays empty.

    // ascending in this order: numeric, string, false, true, with the empty cell last
    const auto sorted_range = to_range(0, 0, 5, 0);
    ixion::detail::sort_keys_t keys = {{0, true}};
    std::vector<ixion::row_t> sorted_rows =
        ixion::detail::sort_range(cxt, store, sorted_range, keys);

    std::vector<ixion::row_t> expected = {
        3, // 1.0
        1, // 2.5
        0, // "foo"
        4, // false
        2, // true
        5  // (empty)
    };
    assert(sorted_rows == expected);

    assert(store[0].get<double>(0) == 1.0);
    assert(store[0].get<double>(1) == 2.5);
    assert(store[0].get<std::uint32_t>(2) == sid.value);
    assert(store[0].get<bool>(3) == false);
    assert(store[0].get<bool>(4) == true);
    assert(store[0].is_empty(5));

    // Descending: everything reverses except the empty cell stays last.
    keys = {{0, false}};

    // layout before sorting
    // 0: 1.0
    // 1: 2.5
    // 2: "foo"
    // 3: false
    // 4: true
    // 5: (empty)
    sorted_rows = ixion::detail::sort_range(cxt, store, sorted_range, keys);

    expected = {
        4, // true
        3, // false
        2, // "foo"
        1, // 2.5
        0, // 1.0
        5  // (empty)
    };
    assert(sorted_rows == expected);

    assert(store[0].get<bool>(0) == true);
    assert(store[0].get<bool>(1) == false);
    assert(store[0].get<std::uint32_t>(2) == sid.value);
    assert(store[0].get<double>(3) == 2.5);
    assert(store[0].get<double>(4) == 1.0);
    assert(store[0].is_empty(5));
}

void test_sheet_sort_stability()
{
    IXION_TEST_FUNC_SCOPE;

    ixion::model_context cxt;
    ixion::detail::sheet_store store(4, 2);

    ixion::string_id_t sid_a = cxt.add_string("a");
    ixion::string_id_t sid_b = cxt.add_string("b");
    ixion::string_id_t sid_c = cxt.add_string("c");
    ixion::string_id_t sid_d = cxt.add_string("d");

    // column A
    store[0].set(0, 2.0);
    store[0].set(1, 1.0);
    store[0].set(2, 2.0);
    store[0].set(3, 1.0);

    // column B
    store[1].set(0, sid_a.value);
    store[1].set(1, sid_b.value);
    store[1].set(2, sid_c.value);
    store[1].set(3, sid_d.value);

    // Rows with equal keys keep their original order.
    ixion::detail::sort_keys_t keys = {{0, true}};

    // layout before sorting
    // 0: 2.0 | "a"
    // 1: 1.0 | "b"
    // 2: 2.0 | "c"
    // 3: 1.0 | "d"
    std::vector<ixion::row_t> sorted_rows =
        ixion::detail::sort_range(cxt, store, to_range(0, 0, 3, 1), keys);

    std::vector<ixion::row_t> expected = {
        1, // 1.0 | "b"
        3, // 1.0 | "d"
        0, // 2.0 | "a"
        2  // 2.0 | "c"
    };
    assert(sorted_rows == expected);

    assert(store[0].get<double>(0) == 1.0);
    assert(store[0].get<double>(1) == 1.0);
    assert(store[0].get<double>(2) == 2.0);
    assert(store[0].get<double>(3) == 2.0);

    assert(store[1].get<std::uint32_t>(0) == sid_b.value);
    assert(store[1].get<std::uint32_t>(1) == sid_d.value);
    assert(store[1].get<std::uint32_t>(2) == sid_a.value);
    assert(store[1].get<std::uint32_t>(3) == sid_c.value);
}

void test_sheet_sort_multi_key()
{
    IXION_TEST_FUNC_SCOPE;

    ixion::model_context cxt;
    ixion::detail::sheet_store store(4, 2);

    ixion::string_id_t sid_a = cxt.add_string("a");
    ixion::string_id_t sid_b = cxt.add_string("b");
    ixion::string_id_t sid_c = cxt.add_string("c");
    ixion::string_id_t sid_d = cxt.add_string("d");

    store[0].set(0, 1.0);
    store[0].set(1, 1.0);
    store[0].set(2, 0.0);
    store[0].set(3, 0.0);

    store[1].set(0, sid_b.value);
    store[1].set(1, sid_a.value);
    store[1].set(2, sid_d.value);
    store[1].set(3, sid_c.value);

    // The second key breaks the ties of the first.
    ixion::detail::sort_keys_t keys = {{0, true}, {1, true}};

    // layout before sorting
    // 0: 1.0 | "b"
    // 1: 1.0 | "a"
    // 2: 0.0 | "d"
    // 3: 0.0 | "c"
    std::vector<ixion::row_t> sorted_rows =
        ixion::detail::sort_range(cxt, store, to_range(0, 0, 3, 1), keys);

    std::vector<ixion::row_t> expected = {
        3, // 0.0 | "c"
        2, // 0.0 | "d"
        1, // 1.0 | "a"
        0  // 1.0 | "b"
    };
    assert(sorted_rows == expected);

    assert(store[0].get<double>(0) == 0.0);
    assert(store[0].get<double>(1) == 0.0);
    assert(store[0].get<double>(2) == 1.0);
    assert(store[0].get<double>(3) == 1.0);

    assert(store[1].get<std::uint32_t>(0) == sid_c.value);
    assert(store[1].get<std::uint32_t>(1) == sid_d.value);
    assert(store[1].get<std::uint32_t>(2) == sid_a.value);
    assert(store[1].get<std::uint32_t>(3) == sid_b.value);
}

void test_sheet_sort_formula_results_travel()
{
    IXION_TEST_FUNC_SCOPE;

    ixion::model_context cxt;
    ixion::detail::sheet_store store(3, 2);

    store[0].set(0, 3.0);
    store[0].set(1, 1.0);
    store[0].set(2, 2.0);

    // Non-grouped formula cells with cached results will NOT get regrouped
    // after the sort.
    const double results[] = {30.0, 10.0, 20.0};

    for (ixion::row_t r = 0; r <= 2; ++r)
    {
        // insert non-grouped formula cell
        auto fc = std::make_unique<ixion::formula_cell>(ixion::formula_tokens_store::create());
        fc->set_result_cache(results[r]);
        store[1].set(r, fc.release());
    }

    ixion::detail::sort_keys_t keys = {{0, true}};

    // layout before sorting: n -> numeric, f -> formula
    // 0: 3.0 [n] | 30.0 [f]
    // 1: 1.0 [n] | 10.0 [f]
    // 2: 2.0 [n] | 20.0 [f]
    std::vector<ixion::row_t> sorted_rows =
        ixion::detail::sort_range(cxt, store, to_range(0, 0, 2, 1), keys);

    std::vector<ixion::row_t> expected = {
        1, // 1.0
        2, // 2.0
        0  // 3.0
    };
    assert(sorted_rows == expected);

    // The cached results travel with their cells.
    assert(formula_value(store[1], 0) == 10.0);
    assert(formula_value(store[1], 1) == 20.0);
    assert(formula_value(store[1], 2) == 30.0);

    for (ixion::row_t r = 0; r <= 2; ++r)
    {
        // make sure the cells stay non-grouped
        const auto* fc = store[1].get<ixion::formula_cell*>(r);
        assert(!fc->get_group_properties().grouped);
    }
}

void test_sheet_sort_group_survives_slide()
{
    IXION_TEST_FUNC_SCOPE;

    ixion::model_context cxt;
    ixion::detail::sheet_store store(6, 2);

    store[0].set(0, 9.0);
    store[0].set(1, 1.0);
    store[0].set(2, 2.0);
    store[0].set(3, 3.0);

    set_group(cxt, store[1], 1, 3, "RC[-1]*10");
    auto identity = store[1].get<ixion::formula_cell*>(1)->get_group_properties().identity;

    // The sort moves the group up by one row with its member order intact,
    // so it survives as a unit.
    ixion::detail::sort_keys_t keys = {{0, true}};

    // layout before sorting
    // 0: 9.0 | (empty)
    // 1: 1.0 | 10.0 (grouped formula)
    // 2: 2.0 | 20.0 (grouped formula)
    // 3: 3.0 | 30.0 (grouped formula)
    std::vector<ixion::row_t> sorted_rows =
        ixion::detail::sort_range(cxt, store, to_range(0, 0, 3, 1), keys);

    std::vector<ixion::row_t> expected = {
        1, // 1.0 | 10.0 (grouped formula)
        2, // 2.0 | 20.0 (grouped formula)
        3, // 3.0 | 30.0 (grouped formula)
        0  // 9.0 | (empty)
    };
    assert(sorted_rows == expected);

    for (ixion::row_t r = 0; r <= 2; ++r)
    {
        const auto* fc = store[1].get<ixion::formula_cell*>(r);
        ixion::formula_group_t group = fc->get_group_properties();
        assert(group.grouped);
        assert(group.size.row == 3);
        assert(group.identity == identity);
        assert(fc->get_parent_position({0, r, 1}).row == 0);
        assert(formula_value(store[1], r) == (r + 1) * 10.0);
    }

    assert(store[1].is_empty(3));
}

void test_sheet_sort_group_breaks_and_regroups()
{
    IXION_TEST_FUNC_SCOPE;

    ixion::model_context cxt;
    ixion::detail::sheet_store store(4, 2);

    store[0].set(0, 4.0);
    store[0].set(1, 1.0);
    store[0].set(2, 3.0);
    store[0].set(3, 2.0);

    set_group(cxt, store[1], 0, 3, "RC[-1]*10");
    store[1].set(3, 99.0);

    // The sort scatters the group members out of order, which breaks the
    // whole group; the two that land adjacent regroup as a sub-run.
    ixion::detail::sort_keys_t keys = {{0, true}};

    // layout before sorting
    // 0: 4.0 | 10.0 (grouped formula)
    // 1: 1.0 | 20.0 (grouped formula)
    // 2: 3.0 | 30.0 (grouped formula)
    // 3: 2.0 | 99.0
    std::vector<ixion::row_t> sorted_rows =
        ixion::detail::sort_range(cxt, store, to_range(0, 0, 3, 1), keys);

    std::vector<ixion::row_t> expected = {
        1, // 1.0 | 20.0 (formula)
        3, // 2.0 | 99.0
        2, // 3.0 | 30.0 (grouped formula)
        0  // 4.0 | 10.0 (grouped formula)
    };
    assert(sorted_rows == expected);

    // the top formula is isolated and is no longer grouped
    const auto* fc = store[1].get<ixion::formula_cell*>(0);
    assert(!fc->get_group_properties().grouped);
    assert(formula_value(store[1], 0) == 20.0); // carries over the result

    assert(store[1].get<double>(1) == 99.0);

    // the bottom two formula cells get regrouped
    for (ixion::row_t r = 2; r <= 3; ++r)
    {
        fc = store[1].get<ixion::formula_cell*>(r);
        ixion::formula_group_t group = fc->get_group_properties();
        assert(group.grouped);
        assert(group.size.row == 2);
        assert(fc->get_parent_position({0, r, 1}).row == 2);
    }

    assert(formula_value(store[1], 2) == 30.0);
    assert(formula_value(store[1], 3) == 10.0);
}

void test_sheet_sort_group_crosses_range_boundary()
{
    IXION_TEST_FUNC_SCOPE;

    ixion::model_context cxt;
    ixion::detail::sheet_store store(6, 2);

    store[0].set(2, 9.0);
    store[0].set(3, 1.0);
    store[0].set(4, 2.0);
    store[0].set(5, 3.0);

    set_group(cxt, store[1], 0, 4, "RC[-1]*10");

    // Group members at row indices 2-3 fall inside the sorted row indices 2-5
    // and get scattered, which breaks the whole group even though row indices
    // 0-1 lie outside the range and never move.
    ixion::detail::sort_keys_t keys = {{0, true}};

    // layout before sorting
    // 0: (empty) | 10.0 (grouped formula)
    // 1: (empty) | 20.0 (grouped formula)
    // 2: 9.0     | 30.0 (grouped formula)
    // 3: 1.0     | 40.0 (grouped formula)
    // 4: 2.0     | (empty)
    // 5: 3.0     | (empty)

    // only sort row indices of 2-5
    std::vector<ixion::row_t> sorted_rows =
        ixion::detail::sort_range(cxt, store, to_range(2, 0, 5, 1), keys);

    std::vector<ixion::row_t> expected = {
        3, // 1.0 | 40.0
        4, // 2.0 | (empty)
        5, // 3.0 | (empty)
        2  // 9.0 | 30.0
    };
    assert(sorted_rows == expected);

    // The member landing at row index 2 regroups with the unmoved members at
    // row indices 0-1 into a new group of 3.
    for (ixion::row_t r = 0; r <= 2; ++r)
    {
        const auto* fc = store[1].get<ixion::formula_cell*>(r);
        ixion::formula_group_t group = fc->get_group_properties();
        assert(group.grouped);
        assert(group.size.row == 3);
        assert(fc->get_parent_position({0, r, 1}).row == 0);
    }

    assert(formula_value(store[1], 0) == 10.0);
    assert(formula_value(store[1], 1) == 20.0);
    assert(formula_value(store[1], 2) == 40.0);
    assert(store[1].is_empty(3));
    assert(store[1].is_empty(4));
    assert(!store[1].get<ixion::formula_cell*>(5)->get_group_properties().grouped);
    assert(formula_value(store[1], 5) == 30.0);
}

void test_sheet_sort_regroup_splits_at_different_formula()
{
    IXION_TEST_FUNC_SCOPE;

    ixion::model_context cxt;
    ixion::detail::sheet_store store(5, 2);

    store[0].set(0, 5.0);
    store[0].set(1, 4.0);
    store[0].set(2, 2.0);
    store[0].set(3, 1.0);
    store[0].set(4, 3.0);

    set_group(cxt, store[1], 0, 4, "RC[-1]*10");

    // standalone formula cell with a different formula
    set_formula(cxt, store[1], 4, "RC[-1]+99", 99.0);

    // The sort reverses the group members and drops the different formula
    // cell in the middle of them.
    ixion::detail::sort_keys_t keys = {{0, true}};

    // layout before sorting
    // 0: 5.0 | 10.0 (grouped formula)
    // 1: 4.0 | 20.0 (grouped formula)
    // 2: 2.0 | 30.0 (grouped formula)
    // 3: 1.0 | 40.0 (grouped formula)
    // 4: 3.0 | 99.0 (formula)
    std::vector<ixion::row_t> sorted_rows =
        ixion::detail::sort_range(cxt, store, to_range(0, 0, 4, 1), keys);

    std::vector<ixion::row_t> expected = {
        3, // 1.0 | 40.0 (grouped formula)
        2, // 2.0 | 30.0 (grouped formula)
        4, // 3.0 | 99.0 (formula)
        1, // 4.0 | 20.0 (grouped formula)
        0  // 5.0 | 10.0 (grouped formula)
    };
    assert(sorted_rows == expected);

    // The different formula cannot join a group, so the members on either
    // side of it regroup separately.

    // upper group
    auto upper_identity = store[1].get<ixion::formula_cell*>(0)->get_group_properties().identity;

    for (ixion::row_t r = 0; r <= 1; ++r)
    {
        const auto* fc = store[1].get<ixion::formula_cell*>(r);
        ixion::formula_group_t group = fc->get_group_properties();
        assert(group.grouped);
        assert(group.size.row == 2);
        assert(group.identity == upper_identity);
        assert(fc->get_parent_position({0, r, 1}).row == 0);
    }

    assert(formula_value(store[1], 0) == 40.0);
    assert(formula_value(store[1], 1) == 30.0);

    // the different formula in the middle stays non-grouped
    assert(!store[1].get<ixion::formula_cell*>(2)->get_group_properties().grouped);
    assert(formula_value(store[1], 2) == 99.0);

    // lower group
    auto lower_identity = store[1].get<ixion::formula_cell*>(3)->get_group_properties().identity;
    assert(lower_identity != upper_identity);

    for (ixion::row_t r = 3; r <= 4; ++r)
    {
        const auto* fc = store[1].get<ixion::formula_cell*>(r);
        ixion::formula_group_t group = fc->get_group_properties();
        assert(group.grouped);
        assert(group.size.row == 2);
        assert(group.identity == lower_identity);
        assert(fc->get_parent_position({0, r, 1}).row == 3);
    }

    assert(formula_value(store[1], 3) == 20.0);
    assert(formula_value(store[1], 4) == 10.0);
}

void test_sheet_sorted_column_no_detach()
{
    IXION_TEST_FUNC_SCOPE;

    ixion::model_context cxt;
    ixion::detail::sheet_store store(3, 1);

    store[0].set(0, 1.0);
    store[0].set(1, 2.0);
    store[0].set(2, 3.0);

    auto cloned = store.clone();

    // Sorting already-sorted rows should leave the columns untouched.
    ixion::detail::sort_keys_t keys = {{0, true}};
    std::vector<ixion::row_t> sorted_rows =
        ixion::detail::sort_range(cxt, cloned, to_range(0, 0, 2, 0), keys);

    std::vector<ixion::row_t> expected = {
        0, // 1.0
        1, // 2.0
        2  // 3.0
    };
    assert(sorted_rows == expected);

    if constexpr (ixion::column_store_traits::enable_cow)
    {
        // If COW is enabled, the column should not have detached since the
        // column is supposed to be left untouched.
        assert(cloned[0].is_shared());
    }

    assert(cloned[0].get<double>(0) == 1.0);
    assert(cloned[0].get<double>(1) == 2.0);
    assert(cloned[0].get<double>(2) == 3.0);
}

void test_sheet_sort_cow_detach_scope()
{
    IXION_TEST_FUNC_SCOPE;

    ixion::model_context cxt;
    ixion::detail::sheet_store store(3, 2);

    store[0].set(0, 3.0);
    store[0].set(1, 1.0);
    store[0].set(2, 2.0);

    store[1].set(0, 7.0);
    store[1].set(1, 8.0);
    store[1].set(2, 9.0);

    auto cloned = store.clone();

    // Sorting only the first column detaches it on the clone while the second
    // column is still sharing its storage with the source.
    ixion::detail::sort_keys_t keys = {{0, true}};
    std::vector<ixion::row_t> sorted_rows =
        ixion::detail::sort_range(cxt, cloned, to_range(0, 0, 2, 0), keys);

    std::vector<ixion::row_t> expected = {
        1, // 1.0
        2, // 2.0
        0  // 3.0
    };
    assert(sorted_rows == expected);

    if constexpr (ixion::column_store_traits::enable_cow)
    {
        assert(!cloned[0].is_shared()); // detached
        assert(cloned[1].is_shared()); // still sharing
    }

    assert(cloned[0].get<double>(0) == 1.0);
    assert(cloned[0].get<double>(1) == 2.0);
    assert(cloned[0].get<double>(2) == 3.0);

    // The source column stays untouched.
    assert(store[0].get<double>(0) == 3.0);
    assert(store[0].get<double>(1) == 1.0);
    assert(store[0].get<double>(2) == 2.0);
}

void test_sheet_sort_invalid_args()
{
    IXION_TEST_FUNC_SCOPE;

    ixion::model_context cxt;
    ixion::detail::sheet_store store(5, 2); // sheet is only 5 rows by 2 columns

    auto expect_invalid = [&cxt, &store](const ixion::abs_rc_range_t& range, const ixion::detail::sort_keys_t& keys)
    {
        try
        {
            ixion::detail::sort_range(cxt, store, range, keys);
            assert(!"std::invalid_argument was expected");
        }
        catch (const std::invalid_argument&)
        {
            // expected
        }
    };

    expect_invalid(to_range(0, 0, 4, 1), {});          // no keys
    expect_invalid(to_range(0, 0, 4, 0), {{1, true}}); // key column outside the range
    expect_invalid(to_range(0, 0, 4, 2), {{0, true}}); // column is outside sheet range
    expect_invalid(to_range(0, 0, 5, 1), {{0, true}}); // row is outside sheet range
    expect_invalid(to_range(3, 0, 2, 1), {{0, true}}); // inverted rows
}

int main()
{
    test_sheet_sort_cross_type_order();
    test_sheet_sort_stability();
    test_sheet_sort_multi_key();
    test_sheet_sort_formula_results_travel();
    test_sheet_sort_group_survives_slide();
    test_sheet_sort_group_breaks_and_regroups();
    test_sheet_sort_group_crosses_range_boundary();
    test_sheet_sort_regroup_splits_at_different_formula();
    test_sheet_sorted_column_no_detach();
    test_sheet_sort_cow_detach_scope();
    test_sheet_sort_invalid_args();

    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
