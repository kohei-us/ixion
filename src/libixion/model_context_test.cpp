/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "test_global.hpp" // This must be the first header to be included.
#include "deprecated.hpp"

#include <ixion/address.hpp>
#include <ixion/cell.hpp>
#include <ixion/cell_access.hpp>
#include <ixion/config.hpp>
#include <ixion/exceptions.hpp>
#include <ixion/formula.hpp>
#include <ixion/formula_name_resolver.hpp>
#include <ixion/formula_result.hpp>
#include <ixion/formula_tokens.hpp>
#include <ixion/matrix.hpp>
#include <ixion/model_cell_range.hpp>
#include <ixion/model_context.hpp>
#include <ixion/model_iterator.hpp>
#include <ixion/named_expressions_iterator.hpp>
#include <ixion/table.hpp>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <ranges>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace {

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
        auto ts = ixion::formula_tokens_store::create(std::move(tokens));
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

ixion::formula_cell* insert_formula(
    ixion::model_context& cxt, const ixion::abs_address_t& pos, const char* exp,
    const ixion::formula_name_resolver& resolver)
{
    auto tokens = ixion::parse_formula_string(cxt, pos, resolver, exp);
    auto ts = ixion::formula_tokens_store::create(std::move(tokens));
    auto* p_inserted = cxt.set_formula_cell(pos, ts);
    assert(p_inserted);
    ixion::register_formula_cell(cxt, pos);
    auto* p = cxt.get_formula_cell(pos);
    assert(p);
    assert(p == p_inserted);
    return p;
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

void test_model_context_append_sheet_copy()
{
    IXION_TEST_FUNC_SCOPE;

    ixion::model_context cxt{{100, 10}};
    cxt.append_sheet("src");

    auto resolver = ixion::formula_name_resolver::get(ixion::formula_name_resolver_t::excel_a1, &cxt);
    assert(resolver);

    // Cell positions on the source sheet.
    ixion::abs_address_t A1(0,0,0);
    ixion::abs_address_t A2(0,1,0);
    ixion::abs_address_t A3(0,2,0);
    ixion::abs_address_t A4(0,3,0);
    ixion::abs_address_t A5(0,4,0);
    ixion::abs_address_t A7(0,6,0);
    ixion::abs_address_t B1(0,0,1);
    ixion::abs_address_t C1(0,0,2);
    ixion::abs_address_t D1(0,0,3);
    ixion::abs_range_t B1B2(0, 0, 1, 2, 1);

    // Populate the source sheet with a mix of cell types.
    cxt.set_numeric_cell(A1, 1.5);
    cxt.set_numeric_cell(A2, 2.25);
    cxt.set_boolean_cell(A3, true);
    cxt.set_string_cell(A4, "note");

    // Formula cell in A5 referencing A1:A2; calculate it to cache its result.
    insert_formula(cxt, A5, "SUM(A1:A2)", *resolver);

    // Formula cell in A7 with a sheet-absolute reference, and one in C1
    // whose result depends on the sheet it sits on.
    insert_formula(cxt, A7, "src!A1", *resolver);
    insert_formula(cxt, C1, "SHEET()", *resolver);

    // Formula cell in D1 with a non-zero relative sheet offset, like the
    // one an unanchored sheet-qualified Calc A1 reference produces.
    {
        ixion::formula_tokens_t tokens;
        tokens.emplace_back(ixion::address_t(1, 0, 0, false, true, true));
        cxt.set_formula_cell(D1, std::move(tokens));
    }

    {
        ixion::abs_range_set_t dirty_cells;
        ixion::abs_range_set_t modified_cells;
        dirty_cells.insert(A5);
        auto sorted = ixion::query_and_sort_dirty_cells(cxt, modified_cells, &dirty_cells);
        ixion::calculate_sorted_cells(cxt, sorted, 0);
    }

    assert(cxt.get_numeric_value(A5) == 3.75);

    // Grouped formula cells in B1:B2 with a pre-supplied result.
    {
        ixion::formula_tokens_t tokens =
            ixion::parse_formula_string(cxt, B1B2.first, *resolver, "\"grouped\"");
        ixion::matrix res_value(2, 1, std::string("grouped"));
        cxt.set_grouped_formula_cells(B1B2, std::move(tokens), ixion::formula_result(std::move(res_value)));
    }

    // Sheet-local named expression whose origin is on the source sheet.
    cxt.set_named_expression(
        0, "answer", A1, ixion::parse_formula_string(cxt, A1, *resolver, "A1+A2"));

    // Copy the source sheet.
    auto res = cxt.append_sheet_copy(0, "copy");
    ixion::sheet_t copied = res.sheet;
    assert(copied == 1);
    assert(cxt.get_sheet_count() == 2);
    assert(cxt.get_sheet_name(copied) == "copy");
    assert(cxt.get_sheet_index("copy") == copied);

    // Same cell positions on the copied sheet.
    ixion::abs_address_t cp_A1(copied,0,0);
    ixion::abs_address_t cp_A2(copied,1,0);
    ixion::abs_address_t cp_A3(copied,2,0);
    ixion::abs_address_t cp_A4(copied,3,0);
    ixion::abs_address_t cp_A5(copied,4,0);
    ixion::abs_address_t cp_A7(copied,6,0);
    ixion::abs_address_t cp_B1(copied,0,1);
    ixion::abs_address_t cp_B2(copied,1,1);
    ixion::abs_address_t cp_C1(copied,0,2);
    ixion::abs_address_t cp_D1(copied,0,3);

    // Only C1 (SHEET()) and D1 (non-zero relative sheet offset) get
    // reported; A5's zero-offset references re-anchor to identical cell
    // values, and A7's reference is sheet-absolute.
    assert(res.recalc_cells.size() == 2);
    assert(res.recalc_cells.count(cp_C1) == 1);
    assert(res.recalc_cells.count(cp_D1) == 1);
    assert(res.recalc_cells.count(cp_A5) == 0);
    assert(res.recalc_cells.count(cp_A7) == 0);

    // The values carry over to the copied sheet.
    assert(cxt.get_numeric_value(cp_A1) == 1.5);
    assert(cxt.get_numeric_value(cp_A2) == 2.25);
    assert(cxt.get_boolean_value(cp_A3) == true);
    assert(cxt.get_string_value(cp_A4) == "note");

    // The copied formula cell is a new cell instance sharing the token store
    // of its source cell, and its cached result carries over without a
    // recalculation.
    const ixion::formula_cell* fc_src = cxt.get_formula_cell(A5);
    const ixion::formula_cell* fc_cp = cxt.get_formula_cell(cp_A5);
    assert(fc_src && fc_cp);
    assert(fc_src != fc_cp);
    assert(fc_src->get_tokens().get() == fc_cp->get_tokens().get());
    assert(cxt.get_numeric_value(cp_A5) == 3.75);

    // The grouped formula cells also carry over together with their results.
    assert(cxt.get_string_value(cp_B1) == "grouped");
    assert(cxt.get_string_value(cp_B2) == "grouped");
    assert(cxt.get_formula_cell(cp_B1) != cxt.get_formula_cell(B1));

    // The sheet-local named expression gets copied with its origin
    // re-anchored to the new sheet.
    const ixion::named_expression_t* exp = cxt.get_named_expression(0, "answer");
    assert(exp);
    assert(exp->origin.sheet == 0);
    exp = cxt.get_named_expression(1, "answer");
    assert(exp);
    assert(exp->origin.sheet == 1);

    // The two sheets are independent stores; modifying one leaves the other
    // untouched.
    cxt.set_numeric_cell(A1, 100.0);
    assert(cxt.get_numeric_value(cp_A1) == 1.5);
    cxt.set_numeric_cell(cp_A2, 200.0);
    assert(cxt.get_numeric_value(A2) == 2.25);

    // Copying to a conflicting sheet name must throw.
    try
    {
        cxt.append_sheet_copy(0, "copy");
        assert(!"sheet name conflict was not thrown");
    }
    catch (const ixion::model_context_error& e)
    {
        assert(e.get_error_type() == ixion::model_context_error::sheet_name_conflict);
    }

    // Copying from an invalid source sheet index must throw.
    try
    {
        cxt.append_sheet_copy(99, "another");
        assert(!"invalid sheet index was not thrown");
    }
    catch (const std::invalid_argument&)
    {
        // expected
    }

    // The failed attempts must not have added a sheet.
    assert(cxt.get_sheet_count() == 2);
}

void test_model_context_append_sheet_copy_no_recalc()
{
    IXION_TEST_FUNC_SCOPE;

    ixion::model_context cxt{{100, 10}};
    cxt.append_sheet("base");
    cxt.append_sheet("src");

    auto resolver = ixion::formula_name_resolver::get(ixion::formula_name_resolver_t::excel_a1, &cxt);
    assert(resolver);

    ixion::abs_address_t base_A1(0,0,0);
    ixion::abs_address_t A1(1,0,0);
    ixion::abs_address_t A2(1,1,0);
    ixion::abs_address_t B1(1,0,1);
    ixion::abs_address_t B2(1,1,1);
    ixion::abs_address_t B3(1,2,1);

    cxt.set_numeric_cell(base_A1, 4.5);
    cxt.set_numeric_cell(A1, 1.25);
    cxt.set_numeric_cell(A2, 2.5);

    // Formula cells with only zero-offset relative and sheet-absolute
    // references; none of them depend on the sheet they sit on.
    insert_formula(cxt, B1, "SUM(A1:A2)", *resolver);
    insert_formula(cxt, B2, "base!A1*2", *resolver);
    insert_formula(cxt, B3, "AVERAGE(A1:A2)", *resolver);

    {
        ixion::abs_range_set_t dirty_cells;
        ixion::abs_range_set_t modified_cells;
        dirty_cells.insert(B1);
        dirty_cells.insert(B2);
        dirty_cells.insert(B3);
        auto sorted = ixion::query_and_sort_dirty_cells(cxt, modified_cells, &dirty_cells);
        ixion::calculate_sorted_cells(cxt, sorted, 0);
    }

    assert(cxt.get_numeric_value(B1) == 3.75);
    assert(cxt.get_numeric_value(B2) == 9.0);
    assert(cxt.get_numeric_value(B3) == 1.875);

    auto res = cxt.append_sheet_copy(1, "copy");
    ixion::sheet_t copied = res.sheet;
    assert(copied == 2);

    // All the carried-over results remain valid on the copied sheet, so no
    // formula cells get reported as needing a re-calculation.
    assert(res.recalc_cells.empty());

    ixion::abs_address_t cp_B1(copied,0,1);
    ixion::abs_address_t cp_B2(copied,1,1);
    ixion::abs_address_t cp_B3(copied,2,1);

    assert(cxt.get_numeric_value(cp_B1) == 3.75);
    assert(cxt.get_numeric_value(cp_B2) == 9.0);
    assert(cxt.get_numeric_value(cp_B3) == 1.875);
}

void test_model_context_tables()
{
    IXION_TEST_FUNC_SCOPE;

    ixion::model_context cxt{{100, 10}};
    cxt.append_sheet("one");
    cxt.append_sheet("two");

    assert(!cxt.get_table("Table1"));
    assert(cxt.get_tables(0).empty());

    // Table1 in C3:D9 with a header row and one totals row.
    ixion::table_t tab;
    tab.name = "Table1";
    tab.range = ixion::abs_range_t({0, 2, 2}, {0, 8, 3});
    tab.columns = { "Category", "Value" };
    tab.totals_row_count = 1;
    cxt.set_table(tab);

    const ixion::table_t* p = cxt.get_table("Table1");
    assert(p);
    assert(p->name == "Table1");
    assert(p->range == ixion::abs_range_t({0, 2, 2}, {0, 8, 3}));
    assert(p->columns.size() == 2);
    assert(p->columns[0] == "Category");
    assert(p->columns[1] == "Value");
    assert(p->totals_row_count == 1);

    assert(cxt.get_tables(0).size() == 1);
    assert(cxt.get_tables(0)[0] == p);
    assert(cxt.get_tables(1).empty());

    // Single column, data area only.
    ixion::abs_range_t range = cxt.get_table_range("Table1", "Value", "", ixion::table_area_data);
    assert(range == ixion::abs_range_t({0, 3, 3}, {0, 7, 3}));

    // Area specifiers only, using the whole table width.
    range = cxt.get_table_range("Table1", "", "", ixion::table_area_headers);
    assert(range == ixion::abs_range_t({0, 2, 2}, {0, 2, 3}));

    range = cxt.get_table_range("Table1", "", "", ixion::table_area_totals);
    assert(range == ixion::abs_range_t({0, 8, 2}, {0, 8, 3}));

    // Single column, headers + data areas.
    range = cxt.get_table_range(
        "Table1", "Category", "", ixion::table_area_headers | ixion::table_area_data);
    assert(range == ixion::abs_range_t({0, 2, 2}, {0, 7, 2}));

    // Column range, data area only.
    range = cxt.get_table_range("Table1", "Category", "Value", ixion::table_area_data);
    assert(range == ixion::abs_range_t({0, 3, 2}, {0, 7, 3}));

    // Headers + totals areas do not form a contiguous range.
    range = cxt.get_table_range(
        "Table1", "Value", "", ixion::table_area_headers | ixion::table_area_totals);
    assert(!range.valid());

    // No column by this name.
    range = cxt.get_table_range("Table1", "Amount", "", ixion::table_area_data);
    assert(!range.valid());

    // The second column of a column range must not precede the first one.
    range = cxt.get_table_range("Table1", "Value", "Category", ixion::table_area_data);
    assert(!range.valid());

    // No table by this name.
    range = cxt.get_table_range("Table2", "Value", "", ixion::table_area_data);
    assert(!range.valid());

    // Position-based lookups; D5 is inside Table1 while A1 is not.
    range = cxt.get_table_range(ixion::abs_address_t(0, 4, 3), "Value", "", ixion::table_area_data);
    assert(range == ixion::abs_range_t({0, 3, 3}, {0, 7, 3}));

    range = cxt.get_table_range(ixion::abs_address_t(0, 0, 0), "Value", "", ixion::table_area_data);
    assert(!range.valid());

    // Totals area of a table with no totals rows.
    tab.name = "NoTotals";
    tab.range = ixion::abs_range_t({1, 0, 0}, {1, 3, 1});
    tab.columns = { "A", "B" };
    tab.totals_row_count = 0;
    cxt.set_table(tab);

    range = cxt.get_table_range("NoTotals", "", "", ixion::table_area_totals);
    assert(!range.valid());

    assert(cxt.get_tables(1).size() == 1);

    // Inserting a table with an existing name should fail.
    try
    {
        tab.name = "Table1";
        tab.range = ixion::abs_range_t({1, 10, 0}, {1, 12, 1});
        cxt.set_table(tab);
        assert(!"model_context_error was expected for a duplicate table name");
    }
    catch (const ixion::model_context_error& e)
    {
        assert(e.get_error_type() == ixion::model_context_error::table_name_conflict);
    }

    // Neither should a table with an empty name, ...
    try
    {
        tab.name.clear();
        cxt.set_table(tab);
        assert(!"std::invalid_argument was expected for an empty table name");
    }
    catch (const std::invalid_argument&)
    {
        // expected
    }

    // ... an invalid range, ...
    try
    {
        tab.name = "Table3";
        tab.range = ixion::abs_range_t(ixion::abs_range_t::invalid);
        cxt.set_table(tab);
        assert(!"std::invalid_argument was expected for an invalid table range");
    }
    catch (const std::invalid_argument&)
    {
        // expected
    }

    // ... or a range spanning multiple sheets.
    try
    {
        tab.range = ixion::abs_range_t({0, 2, 2}, {1, 8, 3});
        cxt.set_table(tab);
        assert(!"std::invalid_argument was expected for a multi-sheet table range");
    }
    catch (const std::invalid_argument&)
    {
        // expected
    }
}

void test_model_context_append_sheet_copy_tables()
{
    IXION_TEST_FUNC_SCOPE;

    ixion::model_context cxt{{100, 10}};
    cxt.append_sheet("src");
    cxt.append_sheet("other");

    // Cell positions on the source sheet.
    ixion::abs_address_t A1(0, 0, 0);
    ixion::abs_address_t B1(0, 0, 1);
    ixion::abs_address_t C3(0, 2, 2);
    ixion::abs_address_t D9(0, 8, 3);
    ixion::abs_address_t F3(0, 2, 5);
    ixion::abs_address_t G5(0, 4, 6);

    // Cell positions on the 'other' sheet.
    ixion::abs_address_t other_B2(1, 1, 1);
    ixion::abs_address_t other_C4(1, 3, 2);

    ixion::table_t tab;
    tab.name = "Table1";
    tab.range = ixion::abs_range_t(C3, D9);
    tab.columns = { "Category", "Value" };
    tab.totals_row_count = 1;
    cxt.set_table(tab);

    tab.name = "Table2";
    tab.range = ixion::abs_range_t(F3, G5);
    tab.columns = { "A", "B" };
    tab.totals_row_count = 0;
    cxt.set_table(tab);

    // Unrelated table on another sheet, which should not get copied.
    tab.name = "TableX";
    tab.range = ixion::abs_range_t(other_B2, other_C4);
    tab.columns = { "C", "D" };
    tab.totals_row_count = 0;
    cxt.set_table(tab);

    auto resolver = ixion::formula_name_resolver::get(ixion::formula_name_resolver_t::excel_a1, &cxt);
    assert(resolver);

    // Populate the 'Value' column of Table1, and add formula cells with an
    // unnamed table reference in its totals row and a named one outside it.
    for (ixion::row_t r = 3; r <= 7; ++r)
        cxt.set_numeric_cell({0, r, 3}, r - 2);

    insert_formula(cxt, D9, "SUBTOTAL(109,[Value])", *resolver);
    insert_formula(cxt, A1, "SUM(Table1[Value])", *resolver);

    // Named reference to the table on the 'other' sheet.
    insert_formula(cxt, B1, "SUM(TableX[C])", *resolver);

    // Sheet-local named expression whose expression contains a named table
    // reference.
    cxt.set_named_expression(
        0, "TableTotal", A1, ixion::parse_formula_string(cxt, A1, *resolver, "SUM(Table1[Value])"));

    // Calculate them to cache their results.
    {
        ixion::abs_range_set_t dirty_cells;
        ixion::abs_range_set_t modified_cells;
        dirty_cells.insert(D9);
        dirty_cells.insert(A1);
        auto sorted = ixion::query_and_sort_dirty_cells(cxt, modified_cells, &dirty_cells);
        ixion::calculate_sorted_cells(cxt, sorted, 0);
    }

    assert(cxt.get_numeric_value(D9) == 15.0);
    assert(cxt.get_numeric_value(A1) == 15.0);

    auto res = cxt.append_sheet_copy(0, "copy");
    ixion::sheet_t copied = res.sheet;
    assert(copied == 2);

    // Same cell positions on the copied sheet.
    ixion::abs_address_t copied_A1(copied, 0, 0);
    ixion::abs_address_t copied_B1(copied, 0, 1);
    ixion::abs_address_t copied_C3(copied, 2, 2);
    ixion::abs_address_t copied_D4(copied, 3, 3);
    ixion::abs_address_t copied_D8(copied, 7, 3);
    ixion::abs_address_t copied_D9(copied, 8, 3);
    ixion::abs_address_t copied_F3(copied, 2, 5);
    ixion::abs_address_t copied_G5(copied, 4, 6);

    // The tables of the source sheet get cloned to the new sheet with
    // auto-generated unique names.
    assert(cxt.get_tables(copied).size() == 2);

    const ixion::table_t* p = cxt.get_table("Table3");
    assert(p);
    assert(p->range == ixion::abs_range_t(copied_C3, copied_D9));
    assert(p->columns == std::vector<std::string>({ "Category", "Value" }));
    assert(p->totals_row_count == 1);

    p = cxt.get_table("Table4");
    assert(p);
    assert(p->range == ixion::abs_range_t(copied_F3, copied_G5));
    assert(p->columns == std::vector<std::string>({ "A", "B" }));
    assert(p->totals_row_count == 0);

    // The source tables remain unmodified, and the unrelated table does not
    // get cloned.
    p = cxt.get_table("Table1");
    assert(p);
    assert(p->range == ixion::abs_range_t(C3, D9));

    p = cxt.get_table("Table2");
    assert(p);
    assert(p->range == ixion::abs_range_t(F3, G5));

    assert(cxt.get_tables(0).size() == 2);
    assert(cxt.get_tables(1).size() == 1);

    // Table references should resolve against the cloned table.
    ixion::abs_range_t range = cxt.get_table_range("Table3", "Value", "", ixion::table_area_data);
    assert(range == ixion::abs_range_t(copied_D4, copied_D8));

    // The named table reference of the copied A1 gets rewritten to reference
    // the cloned table, in a new token store of its own.
    const ixion::model_context& ccxt = cxt;
    const ixion::formula_cell* fc_src = ccxt.get_formula_cell(A1);
    const ixion::formula_cell* fc_cp = ccxt.get_formula_cell(copied_A1);
    assert(fc_src->get_tokens().get() != fc_cp->get_tokens().get());
    assert(ixion::print_formula_tokens(cxt, A1, *resolver, fc_src->get_tokens()->get()) == "SUM(Table1[Value])");
    assert(ixion::print_formula_tokens(cxt, copied_A1, *resolver, fc_cp->get_tokens()->get()) == "SUM(Table3[Value])");

    // The unnamed reference needs no rewriting, and neither does the
    // reference to the table on the 'other' sheet; both cells keep sharing
    // their token stores with their source cells.
    fc_src = ccxt.get_formula_cell(D9);
    fc_cp = ccxt.get_formula_cell(copied_D9);
    assert(fc_src->get_tokens().get() == fc_cp->get_tokens().get());

    fc_src = ccxt.get_formula_cell(B1);
    fc_cp = ccxt.get_formula_cell(copied_B1);
    assert(fc_src->get_tokens().get() == fc_cp->get_tokens().get());
    assert(ixion::print_formula_tokens(cxt, copied_B1, *resolver, fc_cp->get_tokens()->get()) == "SUM(TableX[C])");

    // The table reference inside the copied sheet-local named expression
    // gets rewritten as well, while the source's stays put.
    const ixion::named_expression_t* exp = cxt.get_named_expression(copied, "TableTotal");
    assert(exp);
    assert(ixion::print_formula_tokens(cxt, exp->origin, *resolver, exp->tokens) == "SUM(Table3[Value])");

    exp = cxt.get_named_expression(0, "TableTotal");
    assert(exp);
    assert(ixion::print_formula_tokens(cxt, exp->origin, *resolver, exp->tokens) == "SUM(Table1[Value])");

    // No table-referencing formula cell gets flagged for recalc: the cloned
    // tables hold identical values at copy time.
    assert(res.recalc_cells.empty());

    // Their cached results carry over to the copied sheet.
    assert(cxt.get_numeric_value(copied_D9) == 15.0);
    assert(cxt.get_numeric_value(copied_A1) == 15.0);
}

void test_model_context_append_sheet_copy_table_groups()
{
    IXION_TEST_FUNC_SCOPE;

    ixion::model_context cxt{{100, 10}};
    cxt.append_sheet("src");

    ixion::abs_address_t C3(0, 2, 2);
    ixion::abs_address_t D9(0, 8, 3);

    ixion::table_t tab;
    tab.name = "Table1";
    tab.range = ixion::abs_range_t(C3, D9);
    tab.columns = { "Category", "Value" };
    tab.totals_row_count = 1;
    cxt.set_table(tab);

    auto resolver = ixion::formula_name_resolver::get(ixion::formula_name_resolver_t::excel_a1, &cxt);
    assert(resolver);

    // Grouped formula cells in B1:B3 with a named table reference and a
    // pre-supplied result.
    ixion::abs_range_t B1B3({0, 0, 1}, {0, 2, 1});
    {
        ixion::formula_tokens_t tokens =
            ixion::parse_formula_string(cxt, B1B3.first, *resolver, "SUM(Table1[Value])");
        ixion::matrix res_value(3, 1, 0.0);
        cxt.set_grouped_formula_cells(B1B3, std::move(tokens), ixion::formula_result(std::move(res_value)));
    }

    auto res = cxt.append_sheet_copy(0, "copy");
    ixion::sheet_t copied = res.sheet;

    // All the cells of the copied group share one new token store with the
    // table reference rewritten, while the source group is unaffected.
    const ixion::model_context& ccxt = cxt;
    const ixion::formula_cell* g1 = ccxt.get_formula_cell({copied, 0, 1});
    const ixion::formula_cell* g2 = ccxt.get_formula_cell({copied, 1, 1});
    const ixion::formula_cell* g3 = ccxt.get_formula_cell({copied, 2, 1});
    assert(g1->get_tokens().get() == g2->get_tokens().get());
    assert(g2->get_tokens().get() == g3->get_tokens().get());

    const ixion::formula_cell* src_g1 = ccxt.get_formula_cell({0, 0, 1});
    assert(src_g1->get_tokens().get() != g1->get_tokens().get());

    assert(ixion::print_formula_tokens(
        cxt, {copied, 0, 1}, *resolver, g1->get_tokens()->get()) == "SUM(Table2[Value])");
    assert(ixion::print_formula_tokens(
        cxt, {0, 0, 1}, *resolver, src_g1->get_tokens()->get()) == "SUM(Table1[Value])");
}

void test_model_context_append_sheet_copy_table_ref_divergence()
{
    IXION_TEST_FUNC_SCOPE;

    ixion::model_context cxt{{100, 10}};
    cxt.append_sheet("src");

    ixion::abs_address_t A1(0, 0, 0);
    ixion::abs_address_t C3(0, 2, 2);
    ixion::abs_address_t D5(0, 4, 3);
    ixion::abs_address_t D9(0, 8, 3);

    ixion::table_t tab;
    tab.name = "Table1";
    tab.range = ixion::abs_range_t(C3, D9);
    tab.columns = { "Category", "Value" };
    tab.totals_row_count = 1;
    cxt.set_table(tab);

    auto resolver = ixion::formula_name_resolver::get(ixion::formula_name_resolver_t::excel_a1, &cxt);
    assert(resolver);

    // populate cells D4:D8 with {1, 2, 3, 4, 5}
    for (ixion::row_t r = 3; r <= 7; ++r)
        cxt.set_numeric_cell({0, r, 3}, r - 2);

    insert_formula(cxt, A1, "SUM(Table1[Value])", *resolver);

    {
        ixion::abs_range_set_t dirty_cells;
        ixion::abs_range_set_t modified_cells;
        dirty_cells.insert(A1);
        auto sorted = ixion::query_and_sort_dirty_cells(cxt, modified_cells, &dirty_cells);
        ixion::calculate_sorted_cells(cxt, sorted, 0);
    }

    assert(cxt.get_numeric_value(A1) == 15.0);

    auto res = cxt.append_sheet_copy(0, "copy");
    ixion::sheet_t copied = res.sheet;
    assert(res.recalc_cells.empty());

    ixion::abs_address_t copied_A1(copied, 0, 0);
    ixion::abs_address_t copied_D5(copied, 4, 3);

    // Register the copied cell for dependency tracking, like the document
    // layer does after a sheet copy.
    ixion::register_formula_cell(cxt, copied_A1);

    // Modify a 'Value' cell of the source table.  Only the source A1
    // follows; the copied A1 now depends on the cloned table.
    cxt.set_numeric_cell(D5, 100.0);
    {
        ixion::abs_range_set_t dirty_cells;
        ixion::abs_range_set_t modified_cells;
        modified_cells.insert(D5);
        auto sorted = ixion::query_and_sort_dirty_cells(cxt, modified_cells, &dirty_cells);
        ixion::calculate_sorted_cells(cxt, sorted, 0);
    }

    assert(cxt.get_numeric_value(A1) == 113.0);
    assert(cxt.get_numeric_value(copied_A1) == 15.0);

    // Modify the same cell inside the cloned table; now only the copied A1
    // follows.
    cxt.set_numeric_cell(copied_D5, 200.0);
    {
        ixion::abs_range_set_t dirty_cells;
        ixion::abs_range_set_t modified_cells;
        modified_cells.insert(copied_D5);
        auto sorted = ixion::query_and_sort_dirty_cells(cxt, modified_cells, &dirty_cells);
        ixion::calculate_sorted_cells(cxt, sorted, 0);
    }

    assert(cxt.get_numeric_value(A1) == 113.0);
    assert(cxt.get_numeric_value(copied_A1) == 213.0);
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

} // anonymous namespace

int main()
{
    test_string_pool();
    test_string_pool_duplicate_strings();
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
    test_model_context_append_sheet_copy();
    test_model_context_append_sheet_copy_no_recalc();
    test_model_context_tables();
    test_model_context_append_sheet_copy_tables();
    test_model_context_append_sheet_copy_table_groups();
    test_model_context_append_sheet_copy_table_ref_divergence();
    test_grouped_formula_string_results();
    test_volatile_function();
    test_parse_and_print_expressions();
    test_function_name_resolution();
    test_invalid_formula_tokens();

    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
