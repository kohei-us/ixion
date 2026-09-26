/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "test_global.hpp" // This must be the first header to be included.
#include "formula_registration.hpp"

#include <ixion/model_context.hpp>
#include <ixion/formula.hpp>
#include <ixion/formula_tokens.hpp>
#include <ixion/formula_name_resolver.hpp>
#include <ixion/dirty_cell_tracker.hpp>
#include <ixion/exceptions.hpp>

#include <cassert>
#include <cstdlib>

using namespace ixion;

namespace {

/**
 * Insert a formula cell without registering it.
 */
void insert_formula(
    model_context& cxt, const formula_name_resolver& resolver, const abs_address_t& pos,
    const char* exp)
{
    formula_tokens_t tokens = parse_formula_string(cxt, pos, resolver, exp);
    auto ts = formula_tokens_store::create(std::move(tokens));
    cxt.set_formula_cell(pos, ts);
}

/**
 * Insert grouped formula cells without registering them.
 */
void insert_grouped_formula(
    model_context& cxt, const formula_name_resolver& resolver, const abs_range_t& range,
    const char* exp)
{
    formula_tokens_t tokens = parse_formula_string(cxt, range.first, resolver, exp);
    cxt.set_grouped_formula_cells(range, std::move(tokens));
}

/**
 * Get the cells listening to changes in A1.
 */
abs_range_set_t query_listeners_of_A1(const model_context& cxt)
{
    abs_range_set_t modified;
    modified.emplace(0, 0, 0);
    return cxt.get_cell_tracker().query_dirty_cells(modified);
}

/**
 * Fill a model with values in A1:B2, a formula in D1 and a 2x2 formula
 * group in E1:F2, both referencing A1.  None of the formula cells get
 * registered.
 */
void populate_model(model_context& cxt, const formula_name_resolver& resolver)
{
    cxt.append_sheet("test");

    cxt.set_cell_values(0, {
        {1.0, 2.0},
        {3.0, 4.0},
    });

    abs_address_t D1(0, 0, 3);
    insert_formula(cxt, resolver, D1, "A1*2");

    abs_range_t E1F2({0, 0, 4}, {0, 1, 5});
    insert_grouped_formula(cxt, resolver, E1F2, "A1:B2*2");
}

void test_register_range()
{
    IXION_TEST_FUNC_SCOPE;

    model_context cxt;
    auto resolver = formula_name_resolver::get(formula_name_resolver_t::excel_a1, &cxt);
    populate_model(cxt, *resolver);
    assert(cxt.get_cell_tracker().empty());

    abs_range_t D1(0, 0, 3);
    abs_range_t E1F2({0, 0, 4}, {0, 1, 5});
    abs_rc_range_t data_range(cxt.get_data_range(0));

    detail::register_formula_cells(cxt, 0, data_range);

    abs_range_set_t listeners = query_listeners_of_A1(cxt);
    assert(listeners.size() == 2);
    assert(listeners.count(D1) == 1);
    assert(listeners.count(E1F2) == 1);

    detail::unregister_formula_cells(cxt, 0, data_range);
    assert(cxt.get_cell_tracker().empty());
}

void test_register_range_invalid_cell()
{
    IXION_TEST_FUNC_SCOPE;

    model_context cxt;
    auto resolver = formula_name_resolver::get(formula_name_resolver_t::excel_a1, &cxt);
    populate_model(cxt, *resolver);

    // The parser rejects an unknown sheet name, so build a reference to
    // the sheet before the first one by hand.  It fails validation.
    abs_address_t D2(0, 1, 3);
    address_t ref(-1, 0, 0, false, false, false);
    formula_tokens_t tokens;
    tokens.emplace_back(ref);
    auto ts = formula_tokens_store::create(std::move(tokens));
    cxt.set_formula_cell(D2, ts);

    abs_rc_range_t data_range(cxt.get_data_range(0));

    try
    {
        detail::register_formula_cells(cxt, 0, data_range);
        assert(!"registration of an invalid cell should have failed");
    }
    catch (const formula_registration_error&)
    {
        // expected.
    }

    // The valid cells didn't get registered either.
    assert(cxt.get_cell_tracker().empty());
}

void test_unregister_partial_group()
{
    IXION_TEST_FUNC_SCOPE;

    model_context cxt;
    auto resolver = formula_name_resolver::get(formula_name_resolver_t::excel_a1, &cxt);
    populate_model(cxt, *resolver);

    abs_range_t D1(0, 0, 3);
    abs_range_t E1F2({0, 0, 4}, {0, 1, 5});
    abs_rc_range_t data_range(cxt.get_data_range(0));

    detail::register_formula_cells(cxt, 0, data_range);
    assert(query_listeners_of_A1(cxt).size() == 2);

    // E1:E2 contains the top-left cell of the group, but not the whole
    // group.
    abs_rc_range_t E1E2(0, 4, 2, 1);

    // F1:F2 contains only cells of the group other than its top-left cell.
    abs_rc_range_t F1F2(0, 5, 2, 1);

    for (const abs_rc_range_t& partial : {E1E2, F1F2})
    {
        try
        {
            detail::unregister_formula_cells(cxt, 0, partial);
            assert(!"unregistering part of a group should have failed");
        }
        catch (const formula_registration_error&)
        {
            // expected.
        }

        // The tracker is unchanged.
        abs_range_set_t listeners = query_listeners_of_A1(cxt);
        assert(listeners.size() == 2);
        assert(listeners.count(D1) == 1);
        assert(listeners.count(E1F2) == 1);
    }

    // Registering part of a group fails the same way.
    try
    {
        detail::register_formula_cells(cxt, 0, E1E2);
        assert(!"registering part of a group should have failed");
    }
    catch (const formula_registration_error&)
    {
        // expected.
    }

    // A range covering the whole group works, and leaves the other cells
    // registered.
    detail::unregister_formula_cells(cxt, 0, abs_rc_range_t(E1F2));
    abs_range_set_t listeners = query_listeners_of_A1(cxt);
    assert(listeners.size() == 1);
    assert(listeners.count(D1) == 1);
}

}

int main()
{
    test_register_range();
    test_register_range_invalid_cell();
    test_unregister_partial_group();

    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
