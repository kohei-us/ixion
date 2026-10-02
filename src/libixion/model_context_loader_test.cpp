/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "test_global.hpp" // This must be the first header to be included.
#include <ixion/model_context.hpp>
#include <ixion/model_context_loader.hpp>
#include <ixion/cell.hpp>
#include <ixion/dirty_cell_tracker.hpp>
#include <ixion/exceptions.hpp>
#include <ixion/formula.hpp>
#include <ixion/formula_name_resolver.hpp>
#include <ixion/formula_result.hpp>
#include <ixion/formula_tokens.hpp>
#include <ixion/matrix.hpp>

#include <cassert>
#include <cstdlib>
#include <stdexcept>

using namespace ixion;

namespace {

/** Get the cells listening to changes in a cell. */
abs_range_set_t listeners_of(const model_context& cxt, const abs_address_t& addr)
{
    abs_range_set_t modified;
    modified.emplace(addr);
    return cxt.get_cell_tracker().query_dirty_cells(modified);
}

/** Check that a cell or a formula group is the only listener of a target cell. */
bool is_sole_listener(
    const model_context& cxt, const abs_range_t& listener, const abs_address_t& target)
{
    abs_range_set_t listeners = listeners_of(cxt, target);
    return listeners.size() == 1 && listeners.count(listener) == 1;
}

/**
 * Build a reference to the sheet before the first one.  The parser rejects
 * such a reference, so it has to be built by hand.
 */
formula_tokens_t create_invalid_sheet_ref()
{
    address_t ref(-1, 0, 0, false, false, false);
    formula_tokens_t tokens;
    tokens.emplace_back(ref);
    return tokens;
}

void test_value_setters()
{
    IXION_TEST_FUNC_SCOPE;

    model_context cxt;
    model_context_loader loader(cxt);
    sheet_t sheet = loader.append_sheet("test");
    assert(sheet == 0);

    abs_address_t A3(0, 2, 0);
    abs_address_t B1(0, 0, 1);
    abs_address_t B3(0, 2, 1);
    abs_address_t C1(0, 0, 2);
    abs_address_t C2(0, 1, 2);
    abs_address_t D1(0, 0, 3);
    abs_address_t D2(0, 1, 3);

    // A1:B2
    loader.set_cell_values(sheet, {
        {1.0, 2.0},
        {3.0, 4.0},
    });
    loader.set_numeric_cell(C1, 5.0);
    loader.set_boolean_cell(D1, true);
    loader.set_string_cell(A3, "text");
    loader.set_string_cell(B3, loader.add_string("pooled"));
    loader.set_string_cell(D2, loader.append_string("appended"));
    loader.fill_down_cells(C1, 1);
    loader.finalize();

    assert(cxt.get_numeric_value(B1) == 2.0);
    assert(cxt.get_numeric_value(C1) == 5.0);
    assert(cxt.get_numeric_value(C2) == 5.0);
    assert(cxt.get_boolean_value(D1));
    assert(cxt.get_string_value(A3) == "text");
    assert(cxt.get_string_value(B3) == "pooled");
    assert(cxt.get_string_value(D2) == "appended");
}

void test_overwrite_rejected()
{
    IXION_TEST_FUNC_SCOPE;

    model_context cxt;
    model_context_loader loader(cxt);
    auto resolver = formula_name_resolver::get(formula_name_resolver_t::excel_a1, &cxt);
    loader.append_sheet("test");

    abs_address_t A1(0, 0, 0);
    abs_range_t A1B2(0, 0, 0, 2, 2);
    loader.set_numeric_cell(A1, 1.0);

    // A loader never overwrites a cell.
    try
    {
        loader.set_numeric_cell(A1, 2.0);
        assert(!"overwriting a cell should have thrown");
    }
    catch (const model_context_error& e)
    {
        assert(e.get_error_type() == model_context_error::loader_cell_not_empty);
    }

    // Nor does a group write touch a range that isn't empty.
    try
    {
        loader.set_grouped_formula_cells(A1B2, parse_formula_string(cxt, A1, *resolver, "1"));
        assert(!"writing a group over a cell should have thrown");
    }
    catch (const model_context_error& e)
    {
        assert(e.get_error_type() == model_context_error::loader_cell_not_empty);
    }
}

void test_names_after_formulas()
{
    IXION_TEST_FUNC_SCOPE;

    model_context cxt;

    abs_address_t A1(0, 0, 0);
    abs_address_t B1(0, 0, 1);

    model_context_loader loader(cxt);
    auto resolver = formula_name_resolver::get(formula_name_resolver_t::excel_a1, &cxt);
    loader.append_sheet("test");
    loader.set_numeric_cell(A1, 1.0);

    // The formula uses a name that doesn't exist yet.
    loader.set_formula_cell(B1, parse_formula_string(cxt, B1, *resolver, "MyName*2"));

    // The name arrives after the formula.
    loader.set_named_expression("MyName", parse_formula_string(cxt, A1, *resolver, "$A$1"));

    loader.finalize();
    assert(is_sole_listener(cxt, B1, A1));

    // finalize() ends the load.
    try
    {
        loader.set_numeric_cell(A1, 2.0);
        assert(!"a setter after finalize() should have thrown");
    }
    catch (const model_context_error& e)
    {
        assert(e.get_error_type() == model_context_error::loader_already_finalized);
    }

    try
    {
        loader.finalize();
        assert(!"a second finalize() should have thrown");
    }
    catch (const model_context_error& e)
    {
        assert(e.get_error_type() == model_context_error::loader_already_finalized);
    }
}

void test_cached_results()
{
    IXION_TEST_FUNC_SCOPE;

    model_context cxt;

    abs_address_t A1(0, 0, 0);
    abs_address_t C1(0, 0, 2);
    abs_range_t D1E2({0, 0, 3}, {0, 1, 4});

    model_context_loader loader(cxt);
    auto resolver = formula_name_resolver::get(formula_name_resolver_t::excel_a1, &cxt);
    sheet_t sheet = loader.append_sheet("test");

    // A1:B2
    loader.set_cell_values(sheet, {
        {1.0, 2.0},
        {3.0, 4.0},
    });

    auto ts = formula_tokens_store::create(parse_formula_string(cxt, C1, *resolver, "A1*2"));
    loader.set_formula_cell(C1, ts, formula_result(2.0));

    matrix group_result(2, 2, 5.0);
    loader.set_grouped_formula_cells(
        D1E2, parse_formula_string(cxt, D1E2.first, *resolver, "A1:B2*2"),
        formula_result(std::move(group_result)));

    loader.finalize();

    // The cached results are readable without a calculation.
    assert(cxt.get_numeric_value(C1) == 2.0);
    assert(cxt.get_numeric_value(D1E2.last) == 5.0);

    abs_range_set_t listeners = listeners_of(cxt, A1);
    assert(listeners.size() == 2);
    assert(listeners.count(C1) == 1);
    assert(listeners.count(D1E2) == 1);
}

void test_finalize_rejects_invalid_reference()
{
    IXION_TEST_FUNC_SCOPE;

    model_context cxt;

    abs_address_t A1(0, 0, 0);
    abs_address_t B1(0, 0, 1);
    abs_address_t C1(0, 0, 2);

    model_context_loader loader(cxt);
    auto resolver = formula_name_resolver::get(formula_name_resolver_t::excel_a1, &cxt);
    loader.append_sheet("test");
    loader.set_numeric_cell(A1, 1.0);

    // A reference to an invalid sheet only gets rejected at finalize().
    loader.set_formula_cell(B1, create_invalid_sheet_ref());
    loader.set_formula_cell(C1, parse_formula_string(cxt, C1, *resolver, "A1*2"));

    try
    {
        loader.finalize();
        assert(!"finalize() should have thrown");
    }
    catch (const model_context_error& e)
    {
        assert(e.get_error_type() == model_context_error::invalid_sheet_reference);
    }

    // Nothing got registered, the valid cell included.
    assert(cxt.get_cell_tracker().empty());
}

void test_finalize_rejects_invalid_name_reference()
{
    IXION_TEST_FUNC_SCOPE;

    model_context cxt;

    abs_address_t A1(0, 0, 0);
    abs_address_t B1(0, 0, 1);
    abs_address_t C1(0, 0, 2);

    model_context_loader loader(cxt);
    auto resolver = formula_name_resolver::get(formula_name_resolver_t::excel_a1, &cxt);
    loader.append_sheet("test");
    loader.set_numeric_cell(A1, 1.0);
    loader.set_formula_cell(B1, parse_formula_string(cxt, B1, *resolver, "MyName*2"));
    loader.set_formula_cell(C1, parse_formula_string(cxt, C1, *resolver, "A1*2"));

    // The name resolves to a reference to an invalid sheet, which only
    // shows at finalize().
    loader.set_named_expression("MyName", create_invalid_sheet_ref());

    try
    {
        loader.finalize();
        assert(!"finalize() should have thrown");
    }
    catch (const model_context_error& e)
    {
        assert(e.get_error_type() == model_context_error::invalid_sheet_reference);
    }

    // Nothing got registered, the valid cell included.
    assert(cxt.get_cell_tracker().empty());
}

}

int main()
{
    test_value_setters();
    test_overwrite_rejected();
    test_names_after_formulas();
    test_cached_results();
    test_finalize_rejects_invalid_reference();
    test_finalize_rejects_invalid_name_reference();

    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
