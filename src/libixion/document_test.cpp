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

void test_document()
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

int main()
{
    test_document();

    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
