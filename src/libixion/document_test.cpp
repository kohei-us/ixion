/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/document.hpp"
#include "ixion/address.hpp"
#include "ixion/macros.hpp"
#include "ixion/cell_access.hpp"

#include <iostream>
#include <cassert>

using namespace std;
using namespace ixion;

namespace {

bool equal(double v1, double v2, double multiplier)
{
    long i1 = v1 * multiplier;
    long i2 = v2 * multiplier;
    return i1 == i2;
}

}

void test_basic_calc()
{
    abs_address_t A1(0, 0, 0);
    abs_address_t A2(0, 1, 0);
    abs_address_t A3(0, 2, 0);

    document doc;
    doc.append_sheet("test");
    doc.set_numeric_cell("A1", 1.1);
    doc.set_numeric_cell(A2, 1.2);
    doc.set_numeric_cell({IXION_ASCII("A3")}, 1.3);

    assert(doc.get_numeric_value(A1) == 1.1);
    assert(doc.get_numeric_value("A2") == 1.2);
    assert(doc.get_numeric_value(A3) == 1.3);

    doc.set_formula_cell("B4", "SUM(A1:A3)");
    doc.calculate(0);

    double v = doc.get_numeric_value("B4");
    assert(equal(v, 3.6, 10.0));

    doc.empty_cell(A2);
    doc.calculate(0);
    v = doc.get_numeric_value("B4");
    assert(equal(v, 2.4, 10.0));
}

void test_string_io()
{
    document doc;
    doc.append_sheet("test");

    std::string B3("B3"), C4("C4"), D5("D5"), D5_value("Cell D5 value");
    doc.set_string_cell(B3, IXION_ASCII("Cell B3"));
    doc.set_string_cell(C4, "Cell C4");
    doc.set_string_cell(D5, D5_value);

    const std::string* p = doc.get_string_value(B3);
    assert(p);
    assert(*p == "Cell B3");

    p = doc.get_string_value(C4);
    assert(p);
    assert(*p == "Cell C4");

    p = doc.get_string_value(D5);
    assert(p);
    assert(*p == D5_value);

    doc.set_formula_cell("A10", "CONCATENATE(B3, \" and \", C4)");
    doc.calculate(0);

    p = doc.get_string_value("A10");
    assert(p);
    assert(*p == "Cell B3 and Cell C4");

    doc.empty_cell(C4);
    doc.calculate(0);

    p = doc.get_string_value("A10");
    assert(p);
    assert(*p == "Cell B3 and ");
}

void test_boolean_io()
{
    document doc;
    doc.append_sheet("test1");
    doc.append_sheet("test2");

    doc.set_boolean_cell("test2!B2", true);
    doc.set_boolean_cell("test2!C3", false);

    doc.set_formula_cell("test1!A1", "SUM(test2!A1:D4)");
    doc.calculate(0);
    double v = doc.get_numeric_value("test1!A1");
    assert(v == 1.0);

    // Trigger recalculation.
    doc.set_boolean_cell("test2!C4", true);
    doc.calculate(0);
    v = doc.get_numeric_value("test1!A1");
    assert(v == 2.0);

    cell_access ca = doc.get_cell_access("test2!B2");
    assert(ca.get_type() == celltype_t::boolean);
    assert(ca.get_value_type() == cell_value_t::boolean);

    ca = doc.get_cell_access("test2!C3");
    assert(ca.get_type() == celltype_t::boolean);
    assert(ca.get_value_type() == cell_value_t::boolean);
}

int main()
{
    test_basic_calc();
    test_string_io();
    test_boolean_io();

    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
