
#include <ixion/model_context.hpp>
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

    return EXIT_SUCCESS;
}
