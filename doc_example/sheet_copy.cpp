#include <ixion/document.hpp>
#include <ixion/model_context.hpp>
#include <ixion/model_cell_range.hpp>
#include <ixion/formula_name_resolver.hpp>
#include <ixion/formula.hpp>
#include <ixion/cell.hpp>
#include <ixion/address.hpp>

#include <iostream>
#include <cstdlib>

void set_formula(
    ixion::model_context& cxt, const ixion::formula_name_resolver& resolver,
    const ixion::abs_address_t& pos, std::string_view formula)
{
    ixion::formula_tokens_t tokens = ixion::parse_formula_string(cxt, pos, resolver, formula);
    cxt.set_formula_cell(pos, std::move(tokens));
    ixion::register_formula_cell(cxt, pos);
}

void calculate(ixion::model_context& cxt, const ixion::abs_range_set_t& dirty_formula_cells)
{
    std::vector<ixion::abs_range_t> sorted =
        ixion::query_and_sort_dirty_cells(cxt, {}, &dirty_formula_cells);
    ixion::calculate_sorted_cells(cxt, sorted, 0);
}

//!code-start: register-sheet
// Register every formula cell on a sheet with the dependency tracker.
void register_formula_cells(ixion::model_context& cxt, ixion::sheet_t sheet)
{
    ixion::abs_range_t data_range = cxt.get_data_range(sheet);
    if (!data_range.valid())
        return; // empty sheet

    auto cells = cxt.iterate_cells(
        sheet, ixion::rc_direction_t::vertical, ixion::abs_rc_range_t(data_range));

    for (auto it = cells.begin(); it != cells.end(); ++it)
    {
        if (it->type != ixion::cell_t::formula)
            continue;

        const auto* fc = std::get<const ixion::formula_cell*>(it->value);
        ixion::abs_address_t pos{sheet, it->row, it->col};
        ixion::register_formula_cell(cxt, pos, fc);

        // Registering the top cell of a formula group covers the whole
        // group, so skip over the rest of its cells.
        ixion::formula_group_t group = fc->get_group_properties();
        if (group.grouped)
            std::advance(it, group.size.row - 1);
    }
}
//!code-end: register-sheet

void copy_with_document()
{
    //!code-start: doc-setup
    ixion::document doc;
    doc.append_sheet("src");
    doc.set_numeric_cell("src!A1", 1.5);
    doc.set_numeric_cell("src!A2", 2.25);
    doc.set_formula_cell("src!B1", "SUM(A1:A2)"); // references on the same sheet
    doc.set_formula_cell("src!B2", "src!A1*2");   // sheet-qualified reference
    doc.set_formula_cell("src!C1", "SHEET()");    // depends on the sheet position
    doc.calculate(0);

    doc.get_model_context().dump_sheet(std::cout, 0, ixion::sheet_dump_mode_t::verbose);
    std::cout << std::endl;
    //!code-end: doc-setup

    //!code-start: doc-copy
    ixion::sheet_t copied = doc.append_sheet_copy(0, "copy");

    doc.get_model_context().dump_sheet(std::cout, copied, ixion::sheet_dump_mode_t::verbose);
    std::cout << std::endl;

    doc.calculate(0);
    doc.get_model_context().dump_sheet(std::cout, copied, ixion::sheet_dump_mode_t::verbose);
    std::cout << std::endl;
    //!code-end: doc-copy

    //!code-start: doc-edit-copy
    doc.set_numeric_cell("copy!A1", 100.0);
    doc.calculate(0);

    std::cout << "copy!B1 = " << doc.get_numeric_value("copy!B1") << std::endl;
    std::cout << "copy!B2 = " << doc.get_numeric_value("copy!B2") << std::endl;
    std::cout << "src!B1  = " << doc.get_numeric_value("src!B1") << std::endl;
    //!code-end: doc-edit-copy

    //!code-start: doc-edit-src
    doc.set_numeric_cell("src!A1", 10.5);
    doc.calculate(0);

    std::cout << "src!B1  = " << doc.get_numeric_value("src!B1") << std::endl;
    std::cout << "src!B2  = " << doc.get_numeric_value("src!B2") << std::endl;
    std::cout << "copy!B1 = " << doc.get_numeric_value("copy!B1") << std::endl;
    std::cout << "copy!B2 = " << doc.get_numeric_value("copy!B2") << std::endl;
    //!code-end: doc-edit-src
}

void copy_with_model_context()
{
    //!code-start: cxt-setup
    ixion::model_context cxt;
    cxt.append_sheet("src");

    auto resolver = ixion::formula_name_resolver::get(
        ixion::formula_name_resolver_t::excel_a1, &cxt);

    ixion::abs_address_t src_A1{0, 0, 0};
    ixion::abs_address_t src_A2{0, 1, 0};
    ixion::abs_address_t src_B1{0, 0, 1};
    ixion::abs_address_t src_C1{0, 0, 2};
    cxt.set_numeric_cell(src_A1, 1.5);
    cxt.set_numeric_cell(src_A2, 2.25);
    set_formula(cxt, *resolver, src_B1, "SUM(A1:A2)");
    set_formula(cxt, *resolver, src_C1, "SHEET()");

    calculate(cxt, {src_B1, src_C1});

    cxt.dump_sheet(std::cout, 0, ixion::sheet_dump_mode_t::verbose);
    std::cout << std::endl;
    //!code-end: cxt-setup

    //!code-start: cxt-copy
    ixion::model_context::sheet_copy_result res = cxt.append_sheet_copy(0, "copy");

    std::cout << "new sheet index: " << res.sheet << std::endl;
    for (const ixion::abs_range_t& range : res.recalc_cells)
        std::cout << "needs recalculation: " << range << std::endl;

    cxt.dump_sheet(std::cout, res.sheet, ixion::sheet_dump_mode_t::verbose);
    std::cout << std::endl;
    //!code-end: cxt-copy

    //!code-start: cxt-register
    register_formula_cells(cxt, res.sheet);
    //!code-end: cxt-register

    //!code-start: cxt-recalc
    ixion::abs_address_t copy_C1{res.sheet, 0, 2};
    std::cout << "copy!C1 = " << cxt.get_numeric_value(copy_C1) << std::endl;

    calculate(cxt, res.recalc_cells);
    std::cout << "copy!C1 = " << cxt.get_numeric_value(copy_C1) << " (after calculate)" << std::endl;
    //!code-end: cxt-recalc
}

int main()
{
    copy_with_document();
    std::cout << "--" << std::endl;
    copy_with_model_context();

    return EXIT_SUCCESS;
}
