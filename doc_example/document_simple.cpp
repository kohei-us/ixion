/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*************************************************************************
 *
 * Copyright (c) 2020 Kohei Yoshida
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 ************************************************************************/

#include <ixion/document.hpp>
#include <ixion/address.hpp>
#include <iostream>

int main(int argc, char** argv)
{
    //!code-start: instantiate
    ixion::document doc;
    doc.append_sheet("MySheet");
    //!code-end: instantiate

    //!code-start: set-values
    // Now, populate it with some numeric values in A1:A10.
    for (ixion::abs_address_t pos(0, 0, 0); pos.row <= 9; ++pos.row)
    {
        double value = pos.row + 1.0; // Set the row position + 1 as the cell value.
        doc.set_numeric_cell(pos, value);
    }
    //!code-end: set-values

    //!code-start: insert-strings-1
    // Insert string values.
    std::string s = "This cell contains a string value.";
    doc.set_string_cell("B2", s);
    doc.set_string_cell("B3", "This too contains a string value.");
    //!code-end: insert-strings-1

    //!code-start: insert-string-2
    doc.set_string_cell("MySheet!B4", "Yet another string value.");
    //!code-end: insert-string-2

    //!code-start: set-formula-and-calc
    // Now, let's insert a formula into A11 to sum up values in A1:A10, and calculate it afterward.
    doc.set_formula_cell("A11", "SUM(A1:A10)");
    doc.calculate(0);
    //!code-end: set-formula-and-calc

    //!code-start: set-value-and-print
    double value = doc.get_numeric_value("A11");
    std::cout << "value of A11: " << value << std::endl;
    //!code-end: set-value-and-print

    //!code-start: update-a11-and-recalc
    // Insert a new formula to A11.
    doc.set_formula_cell("A11", "AVERAGE(A1:A10)");
    doc.calculate(0);

    value = doc.get_numeric_value("A11");
    std::cout << "value of A11: " << value << std::endl;
    //!code-end: update-a11-and-recalc

    //!code-start: update-a10-and-recalc
    // Overwrite A10 with a formula cell with no references.
    doc.set_formula_cell("A10", "(100+50)/2");
    doc.calculate(0);

    value = doc.get_numeric_value("A10");
    std::cout << "value of A10: " << value << std::endl;

    value = doc.get_numeric_value("A11");
    std::cout << "value of A11: " << value << std::endl;
    //!code-end: update-a10-and-recalc

    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
