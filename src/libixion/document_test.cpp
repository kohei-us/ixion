/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/document.hpp"
#include "ixion/address.hpp"
#include "ixion/macros.hpp"

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
}

int main()
{
    test_basic_calc();
    test_string_io();

    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
