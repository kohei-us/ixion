#include <ixion/model_context.hpp>
#include <ixion/model_context_loader.hpp>
#include <ixion/formula_name_resolver.hpp>
#include <ixion/formula.hpp>
#include <ixion/formula_result.hpp>
#include <ixion/formula_tokens.hpp>
#include <ixion/address.hpp>

#include <iostream>
#include <cstdlib>

int main()
{
    //!code-start: create-loader
    ixion::model_context cxt;
    auto resolver = ixion::formula_name_resolver::get(
        ixion::formula_name_resolver_t::excel_a1, &cxt);

    ixion::model_context_loader loader(cxt);
    loader.append_sheet("Sheet1");
    //!code-end: create-loader

    //!code-start: values
    loader.set_cell_values(0, {  // Sheet1!A1:B3
        { 1.0, 10.0 },
        { 2.0, 20.0 },
        { 3.0, 30.0 },
    });
    //!code-end: values

    //!code-start: formulas
    ixion::abs_address_t C1{0, 0, 2};
    ixion::abs_address_t C2{0, 1, 2};

    ixion::formula_tokens_t tokens = ixion::parse_formula_string(cxt, C1, *resolver, "SUM(MyData)");
    loader.set_formula_cell(
        C1, ixion::formula_tokens_store::create(std::move(tokens)), ixion::formula_result(66.0));

    tokens = ixion::parse_formula_string(cxt, C2, *resolver, "A1+B1");
    loader.set_formula_cell(
        C2, ixion::formula_tokens_store::create(std::move(tokens)), ixion::formula_result(11.0));
    //!code-end: formulas

    //!code-start: name
    ixion::abs_address_t A1{0, 0, 0};
    tokens = ixion::parse_formula_string(cxt, A1, *resolver, "$A$1:$B$3");
    loader.set_named_expression("MyData", A1, std::move(tokens));
    //!code-end: name

    //!code-start: finalize
    loader.finalize();
    //!code-end: finalize

    //!code-start: use-model
    std::cout << "C1 = " << cxt.get_numeric_value(C1) << std::endl;
    std::cout << "C2 = " << cxt.get_numeric_value(C2) << std::endl;

    cxt.set_numeric_cell(A1, 100.0);
    std::vector<ixion::abs_range_t> sorted = ixion::query_and_sort_dirty_cells(cxt, {A1});
    ixion::calculate_sorted_cells(cxt, sorted, 0);

    std::cout << "C1 = " << cxt.get_numeric_value(C1) << std::endl;
    std::cout << "C2 = " << cxt.get_numeric_value(C2) << std::endl;
    //!code-end: use-model

    return EXIT_SUCCESS;
}
