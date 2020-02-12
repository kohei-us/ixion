
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
    cxt.set_string_cell(B2, s.data(), s.size());

    // Insert a literal string value into B3.
    ixion::abs_address_t B3(0, 2, 1);
    cxt.set_string_cell(B3, IXION_ASCII("This too contains a string value."));

    // Insert a string value into B4 via string identifier.
    s = "Yet another string value.";
    ixion::string_id_t sid = cxt.add_string(s.data(), s.size());
    ixion::abs_address_t B4(0, 3, 1);
    cxt.set_string_cell(B4, sid);

    // Now, let's insert a formula into A11 to sum up values in A1:A10.
    ixion::abs_address_t A11(0, 10, 0);

    // Tokenize formula string first.
    std::unique_ptr<ixion::formula_name_resolver> resolver =
        ixion::formula_name_resolver::get(ixion::formula_name_resolver_t::excel_a1, &cxt);
    s = "SUM(A1:A10)";
    ixion::formula_tokens_t tokens = ixion::parse_formula_string(cxt, A11, *resolver, s.data(), s.size());

    // Set the tokens into the model.
    cxt.set_formula_cell(A11, std::move(tokens));

    // Register this formula cell for automatic dependency tracking.
    ixion::register_formula_cell(cxt, A11);

    // Build a set of modified cells, to determine which formula cells depend
    // on them eithe directly or indirectly.

    ixion::abs_range_t A1_A10(0, 0, 0, 10, 1); // sheet, row, column, row span, column span
    ixion::abs_range_set_t modified_cells{A1_A10};

    // Determine formula cells that need re-calculation given the modified cells.
    // There should be only one formula cell in this example.
    std::vector<ixion::abs_range_t> dirty_cells = ixion::query_and_sort_dirty_cells(cxt, modified_cells);
    cout << "number of dirty cells: " << dirty_cells.size() << endl;

    // Now perform calculation.
    ixion::calculate_sorted_cells(cxt, dirty_cells, 0);

    double value = cxt.get_numeric_value(A11);
    cout << "value of A11: " << value << endl;

    return EXIT_SUCCESS;
}