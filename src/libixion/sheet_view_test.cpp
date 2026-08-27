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
#include <ixion/model_context.hpp>
#include <ixion/sheet_view.hpp>

#include <cassert>
#include <cstdlib>
#include <stdexcept>
#include <utility>

namespace {

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

    cxt.create_sheet_view(0, "view1");

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

    {
        // formula cell with a pre-computed result
        auto resolver = ixion::formula_name_resolver::get(
            ixion::formula_name_resolver_t::excel_a1, &cxt);
        assert(resolver);

        auto tokens = ixion::parse_formula_string(cxt, A4, *resolver, "A1*2");
        auto ts = ixion::formula_tokens_store::create(std::move(tokens));
        cxt.set_formula_cell(A4, ts, ixion::formula_result(3.0));
    }

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

} // anonymous namespace

int main()
{
    test_sheet_view_create_get_remove();
    test_sheet_view_invalid_args();
    test_sheet_view_reads_mirror_base();
    test_sheet_view_snapshot();

    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
