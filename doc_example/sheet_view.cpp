#include <ixion/model_context.hpp>
#include <ixion/sheet_view.hpp>
#include <ixion/address.hpp>
#include <ixion/table.hpp>

#include <iostream>
#include <cstdlib>

int main()
{
    //!code-start: populate
    ixion::model_context cxt;
    cxt.append_sheet("Scores");

    // header row
    cxt.set_string_cell({0, 0, 0}, "Name");
    cxt.set_string_cell({0, 0, 1}, "Score");

    // data rows
    cxt.set_string_cell({0, 1, 0}, "bob");
    cxt.set_numeric_cell({0, 1, 1}, 20.0);
    cxt.set_string_cell({0, 2, 0}, "amy");
    cxt.set_numeric_cell({0, 2, 1}, 30.0);
    cxt.set_string_cell({0, 3, 0}, "cid");
    cxt.set_numeric_cell({0, 3, 1}, 10.0);

    // totals row
    cxt.set_string_cell({0, 4, 0}, "Total");
    cxt.set_numeric_cell({0, 4, 1}, 60.0);

    cxt.dump_sheet(std::cout, 0, ixion::sheet_dump_mode_t::simple);
    std::cout << std::endl;
    //!code-end: populate

    //!code-start: create-view
    ixion::sheet_view& view = cxt.create_sheet_view(0, "by-score");

    ixion::abs_rc_address_t A2{1, 0};
    std::cout << "A2 in the view: " << view.get_string_value(A2) << std::endl;
    //!code-end: create-view

    //!code-start: sort
    ixion::abs_rc_range_t A2_B4{1, 0, 3, 2}; // the data rows
    view.sort(A2_B4, {{1, ixion::sort_order_t::descending}}); // by column B

    view.dump(std::cout, ixion::sheet_dump_mode_t::simple);
    std::cout << std::endl;
    //!code-end: sort

    //!code-start: base-untouched
    std::cout << "A2 in the base sheet: " << cxt.get_string_value({0, 1, 0}) << std::endl;
    //!code-end: base-untouched

    //!code-start: row-mapping
    std::cout << "view row 1 shows base row " << view.to_base_row(1) << std::endl;
    std::cout << "base row 1 is shown at view row " << view.to_view_row(1) << std::endl;
    //!code-end: row-mapping

    //!code-start: refresh
    // change cid's score at A4 on the base sheet
    cxt.set_numeric_cell({0, 3, 1}, 40.0);

    view.refresh();
    view.dump(std::cout, ixion::sheet_dump_mode_t::simple);
    std::cout << std::endl;
    //!code-end: refresh

    //!code-start: re-sort
    view.sort(A2_B4, {{1, ixion::sort_order_t::descending}});
    view.dump(std::cout, ixion::sheet_dump_mode_t::simple);
    std::cout << std::endl;
    //!code-end: re-sort

    //!code-start: set-table
    ixion::table_t tab;
    tab.name = "ScoreTable";
    tab.sheet = 0;
    tab.range = {0, 0, 5, 2}; // A1:B5
    tab.columns = {"Name", "Score"};
    tab.totals_row_count = 1;
    cxt.set_table(std::move(tab));
    //!code-end: set-table

    //!code-start: sort-table
    view.sort_table("ScoreTable", "Name", ixion::sort_order_t::ascending);
    view.dump(std::cout, ixion::sheet_dump_mode_t::simple);
    std::cout << std::endl;
    //!code-end: sort-table

    //!code-start: remove-view
    cxt.remove_sheet_view(0, "by-score");
    bool found = cxt.get_sheet_view(0, "by-score") != nullptr;
    std::cout << "view found: " << std::boolalpha << found << std::endl;
    //!code-end: remove-view

    return EXIT_SUCCESS;
}
