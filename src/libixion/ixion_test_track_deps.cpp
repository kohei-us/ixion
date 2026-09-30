/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "test_global.hpp" // This must be the first header to be included.

#include <ixion/model_context.hpp>
#include <ixion/formula_name_resolver.hpp>
#include <ixion/formula.hpp>

#include <cassert>
#include <iostream>

using namespace ixion;
using namespace std;

void test_single_cell_dependency()
{
    IXION_TEST_FUNC_SCOPE;

    model_context cxt{{400, 200}};
    cxt.append_sheet("One");

    auto resolver = formula_name_resolver::get(formula_name_resolver_t::excel_a1, &cxt);

    cxt.set_numeric_cell(abs_address_t(0,0,0), 1.0);  // A1

    // A2
    abs_address_t pos(0,1,0);
    formula_tokens_t tokens = parse_formula_string(cxt, pos, *resolver, "A1*2");
    formula_tokens_store_ptr_t store = formula_tokens_store::create(std::move(tokens));
    cxt.set_formula_cell(pos, store);

    // A3
    pos.row = 2;
    tokens = parse_formula_string(cxt, pos, *resolver, "A2*2");
    store = formula_tokens_store::create(std::move(tokens));
    cxt.set_formula_cell(pos, store);

    // If A1 is modified, then both A2 and A3 should get updated.
    abs_address_set_t mod_cells = {
        { 0, 0, 0 }
    };

    abs_address_set_t cells = query_dirty_cells(cxt, mod_cells);

    assert(cells.size() == 2);
    assert(cells.count(abs_address_t(0,1,0)) == 1);
    assert(cells.count(abs_address_t(0,2,0)) == 1);
}

void test_range_dependency()
{
    IXION_TEST_FUNC_SCOPE;

    model_context cxt{{400, 200}};
    cxt.append_sheet("One");

    cxt.set_numeric_cell(abs_address_t(0,0,0), 1.0);  // A1
    cxt.set_numeric_cell(abs_address_t(0,0,0), 2.0);  // A2
    cxt.set_numeric_cell(abs_address_t(0,0,0), 3.0);  // A3

    cxt.set_numeric_cell(abs_address_t(0,0,2), 4.0);  // C1
    cxt.set_numeric_cell(abs_address_t(0,0,2), 5.0);  // D1
    cxt.set_numeric_cell(abs_address_t(0,0,2), 6.0);  // E1

    auto resolver = formula_name_resolver::get(formula_name_resolver_t::excel_a1, &cxt);

    // C5
    abs_address_t pos(0,4,2);
    formula_tokens_t tokens = parse_formula_string(cxt, pos, *resolver, "SUM(A1:A3,C1:E1)");
    auto ts = formula_tokens_store::create(std::move(tokens));
    cxt.set_formula_cell(pos, ts);

    // A10
    pos.row = 9;
    pos.column = 0;
    tokens = parse_formula_string(cxt, pos, *resolver, "C5*2");
    ts = formula_tokens_store::create(std::move(tokens));
    cxt.set_formula_cell(pos, ts);

    // If A1 is modified, both C5 and A10 should get updated.
    abs_address_set_t addrs = { abs_address_t(0,0,0) };
    abs_address_set_t cells = query_dirty_cells(cxt, addrs);

    assert(cells.count(abs_address_t(0,4,2)) == 1);
    assert(cells.count(abs_address_t(0,9,0)) == 1);
}

void test_matrix_dependency()
{
    IXION_TEST_FUNC_SCOPE;

    model_context cxt{{400, 200}};
    cxt.append_sheet("One");

    cxt.set_numeric_cell(abs_address_t(0,0,0), 1.0);  // A1
    cxt.set_numeric_cell(abs_address_t(0,0,0), 2.0);  // A2
    cxt.set_numeric_cell(abs_address_t(0,0,0), 3.0);  // A3

    cxt.set_numeric_cell(abs_address_t(0,0,2), 4.0);  // C1
    cxt.set_numeric_cell(abs_address_t(0,0,2), 5.0);  // D1
    cxt.set_numeric_cell(abs_address_t(0,0,2), 6.0);  // E1

    abs_range_t range;
    range.first = abs_address_t(0,4,2); // C5
    range.last  = abs_address_t(0,6,4); // E7

    auto resolver = formula_name_resolver::get(formula_name_resolver_t::excel_a1, &cxt);

    // C5:E7
    formula_tokens_t tokens = parse_formula_string(
        cxt, range.first, *resolver, "MMULT(A1:A3,C1:E1)");

    cxt.set_grouped_formula_cells(range, std::move(tokens));

    // A10
    abs_address_t pos(0,9,0);
    tokens = parse_formula_string(cxt, pos, *resolver, "C5*2");
    auto ts = formula_tokens_store::create(std::move(tokens));
    cxt.set_formula_cell(pos, ts);

    // If A1 is modified, both C5 and A10 should get updated.
    abs_address_set_t addrs = { abs_address_t(0,0,0) };
    abs_address_set_t cells = query_dirty_cells(cxt, addrs);

    assert(cells.count(abs_address_t(0,4,2)) == 1);
    assert(cells.count(abs_address_t(0,9,0)) == 1);
}

int main()
{
    test_single_cell_dependency();
    test_range_dependency();
    test_matrix_dependency();

    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
