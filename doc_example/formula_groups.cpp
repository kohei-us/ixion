#include <ixion/model_context.hpp>
#include <ixion/formula_name_resolver.hpp>
#include <ixion/formula.hpp>
#include <ixion/formula_tokens.hpp>
#include <ixion/formula_result.hpp>
#include <ixion/cell.hpp>
#include <ixion/address.hpp>
#include <ixion/matrix.hpp>

#include <iostream>
#include <cstdlib>

//!code-start: calculate
void calculate(
    ixion::model_context& cxt, const ixion::abs_range_set_t& modified_cells,
    const ixion::abs_range_set_t& new_formula_cells)
{
    std::vector<ixion::abs_range_t> sorted =
        ixion::query_and_sort_dirty_cells(cxt, modified_cells, &new_formula_cells);

    ixion::calculate_sorted_cells(cxt, sorted, 0);
}
//!code-end: calculate

//!code-start: dump
void dump(const ixion::model_context& cxt)
{
    cxt.dump_sheet(std::cout, 0, ixion::sheet_dump_mode_t::verbose);
    std::cout << std::endl;
}
//!code-end: dump

int main()
{
    //!code-start: setup
    std::cout << std::boolalpha;

    ixion::model_context cxt;
    cxt.append_sheet("Sheet1");

    auto resolver = ixion::formula_name_resolver::get(
        ixion::formula_name_resolver_t::excel_a1, &cxt);

    cxt.set_cell_values(0, {
        {1.0, 2.0}, // A1:B1
        {3.0, 4.0}, // A2:B2
    });
    //!code-end: setup

    //!code-start: create-group
    ixion::abs_range_t D1E2{{0, 0, 3}, {0, 1, 4}};

    ixion::formula_tokens_t tokens = ixion::parse_formula_string(
        cxt, D1E2.first, *resolver, "A1:B2*10");
    cxt.set_grouped_formula_cells(D1E2, std::move(tokens));

    dump(cxt);
    //!code-end: create-group

    //!code-start: calculate-group
    calculate(cxt, {}, {D1E2});

    dump(cxt);
    //!code-end: calculate-group

    //!code-start: inspect
    const ixion::formula_cell* D1 = cxt.get_formula_cell(D1E2.first);

    struct named_cell
    {
        const char* name;
        ixion::abs_address_t pos;
    };

    named_cell cells[] = {
        {"D1", {0, 0, 3}},
        {"D2", {0, 1, 3}},
        {"E1", {0, 0, 4}},
        {"E2", {0, 1, 4}},
    };

    for (const named_cell& c : cells)
    {
        const ixion::formula_cell* cell = cxt.get_formula_cell(c.pos);
        ixion::abs_address_t parent = cell->get_parent_position(c.pos);

        std::cout << c.name << ": " << cell->get_group_properties() << std::endl;
        std::cout << "  parent is D1: " << (parent == D1E2.first) << std::endl;
        std::cout << "  same tokens as D1: " << (cell->get_tokens() == D1->get_tokens())
            << std::endl;
    }
    //!code-end: inspect

    //!code-start: results
    ixion::abs_address_t Sheet1_E2{0, 1, 4};
    const ixion::formula_cell* E2 = cxt.get_formula_cell(Sheet1_E2);

    const ixion::formula_result& raw = E2->get_raw_result_cache(
        ixion::formula_result_wait_policy_t::throw_exception);
    std::cout << "raw result: " << raw.str(cxt) << std::endl;

    ixion::formula_result single = E2->get_result_cache(
        ixion::formula_result_wait_policy_t::throw_exception);
    std::cout << "E2 result: " << single.str(cxt) << std::endl;
    std::cout << "E2 value: " << cxt.get_numeric_value(Sheet1_E2) << std::endl;
    //!code-end: results

    //!code-start: modify
    ixion::abs_address_t Sheet1_A1{0, 0, 0};
    cxt.set_numeric_cell(Sheet1_A1, 5.0);
    calculate(cxt, {Sheet1_A1}, {});

    dump(cxt);
    //!code-end: modify

    //!code-start: inline-array
    ixion::abs_address_t Sheet1_G1{0, 0, 6};
    tokens = ixion::parse_formula_string(cxt, Sheet1_G1, *resolver, "{1,2;3,4}*10");
    cxt.set_formula_cell(Sheet1_G1, std::move(tokens));
    calculate(cxt, {}, {Sheet1_G1});

    dump(cxt);

    const ixion::formula_cell* G1 = cxt.get_formula_cell(Sheet1_G1);
    std::cout << "G1 grouped: " << G1->get_group_properties().grouped << std::endl;
    //!code-end: inline-array

    return EXIT_SUCCESS;
}
