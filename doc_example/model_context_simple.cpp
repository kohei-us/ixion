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

#include <ixion/model_context.hpp>
#include <ixion/macros.hpp>
#include <ixion/formula_name_resolver.hpp>
#include <ixion/formula.hpp>
#include <iostream>

using namespace std;

int main(int argc, char** argv)
{
    ixion::model_context cxt;

    // First and foremost, insert a sheet.
    cxt.append_sheet("MySheet");

    // Now, populate it with some numeric values in A1:A10.
    for (ixion::abs_address_t pos(0, 0, 0); pos.row <= 9; ++pos.row)
    {
        double value = pos.row + 1.0; // Set the row position + 1 as the cell value.
        cxt.set_numeric_cell(pos, value);
    }

    // Insert a string value into B2.
    ixion::abs_address_t B2(0, 1, 1);
    std::string s = "This cell contains a string value.";
    cxt.set_string_cell(B2, s);

    // Insert a literal string value into B3.
    ixion::abs_address_t B3(0, 2, 1);
    cxt.set_string_cell(B3, "This too contains a string value.");

    // Insert a string value into B4 via string identifier.
    ixion::string_id_t sid = cxt.add_string("Yet another string value.");
    ixion::abs_address_t B4(0, 3, 1);
    cxt.set_string_cell(B4, sid);

    // Now, let's insert a formula into A11 to sum up values in A1:A10.

    // Tokenize formula string first.
    std::unique_ptr<ixion::formula_name_resolver> resolver =
        ixion::formula_name_resolver::get(ixion::formula_name_resolver_t::excel_a1, &cxt);

    ixion::abs_address_t A11(0, 10, 0);
    ixion::formula_tokens_t tokens = ixion::parse_formula_string(cxt, A11, *resolver, "SUM(A1:A10)");

    // Set the tokens into the model.
    const ixion::formula_cell* cell = cxt.set_formula_cell(A11, std::move(tokens));

    // Register this formula cell for automatic dependency tracking.
    ixion::register_formula_cell(cxt, A11, cell);

    // Build a set of modified cells, to determine which formula cells depend
    // on them eithe directly or indirectly.  Since we are performing initial
    // calculation, we can flag the entire sheet to be "modified" to trigger
    // all formula cells to be calculated.

    ixion::rc_size_t sheet_size = cxt.get_sheet_size();
    ixion::abs_range_t entire_sheet(0, 0, 0, sheet_size.row, sheet_size.column); // sheet, row, column, row span, column span
    ixion::abs_range_set_t modified_cells{entire_sheet};

    // Determine formula cells that need re-calculation given the modified cells.
    // There should be only one formula cell in this example.
    std::vector<ixion::abs_range_t> dirty_cells = ixion::query_and_sort_dirty_cells(cxt, modified_cells);
    cout << "number of dirty cells: " << dirty_cells.size() << endl;
    cout << "dirty cell: " << dirty_cells[0] << endl;

    // Now perform calculation.
    ixion::calculate_sorted_cells(cxt, dirty_cells, 0);

    double value = cxt.get_numeric_value(A11);
    cout << "value of A11: " << value << endl;

    // Insert a new formula to A11.
    tokens = ixion::parse_formula_string(cxt, A11, *resolver, "AVERAGE(A1:A10)");

    // Before overwriting, make sure to UN-register the old cell.
    ixion::unregister_formula_cell(cxt, A11);

    // Set and register the new formula cell.
    cell = cxt.set_formula_cell(A11, std::move(tokens));
    ixion::register_formula_cell(cxt, A11, cell);

    // This time, we know that none of the cell values have changed, but the
    // formula A11 is updated & needs recalculation.
    ixion::abs_range_set_t modified_formula_cells{A11};
    dirty_cells = ixion::query_and_sort_dirty_cells(cxt, ixion::abs_range_set_t(), &modified_formula_cells);
    cout << "number of dirty cells: " << dirty_cells.size() << endl;

    // Perform calculation again.
    ixion::calculate_sorted_cells(cxt, dirty_cells, 0);

    value = cxt.get_numeric_value(A11);
    cout << "value of A11: " << value << endl;

    // Overwrite A10 with a formula cell with no references.
    ixion::abs_address_t A10(0, 9, 0);
    tokens = ixion::parse_formula_string(cxt, A10, *resolver, "(100+50)/2");
    cxt.set_formula_cell(A10, std::move(tokens));
    // No need to register this cell since it does not reference any other cells.

    modified_formula_cells = { A10 };
    dirty_cells = ixion::query_and_sort_dirty_cells(cxt, ixion::abs_range_set_t(), &modified_formula_cells);
    cout << "number of dirty cells: " << dirty_cells.size() << endl;

    ixion::calculate_sorted_cells(cxt, dirty_cells, 0);
    value = cxt.get_numeric_value(A10);
    cout << "value of A10: " << value << endl;
    value = cxt.get_numeric_value(A11);
    cout << "value of A11: " << value << endl;

    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
