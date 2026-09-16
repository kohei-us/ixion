#include <ixion/model_context.hpp>
#include <ixion/formula_name_resolver.hpp>
#include <ixion/formula.hpp>
#include <ixion/formula_tokens.hpp>
#include <ixion/formula_result.hpp>
#include <ixion/exceptions.hpp>
#include <ixion/address.hpp>

#include <iostream>
#include <cstdlib>

//!code-start: helpers
void set_formula(
    ixion::model_context& cxt, const ixion::formula_name_resolver& resolver,
    const ixion::abs_address_t& pos, std::string_view formula)
{
    ixion::formula_tokens_t tokens = ixion::parse_formula_string(cxt, pos, resolver, formula);
    cxt.set_formula_cell(pos, std::move(tokens));

    ixion::register_formula_cell(cxt, pos);
}

void calculate(
    ixion::model_context& cxt, const ixion::abs_range_set_t& modified_cells,
    const ixion::abs_range_set_t& new_formula_cells)
{
    std::vector<ixion::abs_range_t> sorted =
        ixion::query_and_sort_dirty_cells(cxt, modified_cells, &new_formula_cells);

    ixion::calculate_sorted_cells(cxt, sorted, 0);
}
//!code-end: helpers

//!code-start: print-result
void print_result(
    const ixion::model_context& cxt, const char* name, const ixion::abs_address_t& pos)
{
    ixion::formula_result result = cxt.get_formula_result(pos);
    std::cout << name << ": " << result.str(cxt) << " (" << result.get_type() << ")" << std::endl;
}
//!code-end: print-result

int main()
{
    std::cout << std::boolalpha;

    //!code-start: setup
    ixion::model_context cxt;
    cxt.append_sheet("Sheet1");

    auto resolver = ixion::formula_name_resolver::get(
        ixion::formula_name_resolver_t::excel_a1, &cxt);

    ixion::abs_address_t Sheet1_A1{0, 0, 0};
    ixion::abs_address_t Sheet1_B1{0, 0, 1};
    ixion::abs_address_t Sheet1_C1{0, 0, 2};
    ixion::abs_address_t Sheet1_D1{0, 0, 3};
    ixion::abs_address_t Sheet1_E1{0, 0, 4};

    cxt.set_numeric_cell(Sheet1_A1, 10.0);
    cxt.set_numeric_cell(Sheet1_B1, 0.0);
    set_formula(cxt, *resolver, Sheet1_C1, "A1/B1");
    set_formula(cxt, *resolver, Sheet1_D1, "C1+1");
    set_formula(cxt, *resolver, Sheet1_E1, "ISERROR(C1)");
    calculate(cxt, {}, {Sheet1_C1, Sheet1_D1, Sheet1_E1});

    print_result(cxt, "C1", Sheet1_C1);
    print_result(cxt, "D1", Sheet1_D1);
    print_result(cxt, "E1", Sheet1_E1);
    //!code-end: setup

    //!code-start: error-type
    ixion::formula_result C1_result = cxt.get_formula_result(Sheet1_C1);
    if (C1_result.get_type() == ixion::formula_result::result_type::error)
    {
        ixion::formula_error_t error = C1_result.get_error();
        std::cout << "C1 error: " << ixion::get_formula_error_name(error) << std::endl;
        std::cout << "C1 is division by zero: "
            << (error == ixion::formula_error_t::division_by_zero) << std::endl;
    }
    //!code-end: error-type

    //!code-start: numeric-throws
    try
    {
        double value = cxt.get_numeric_value(Sheet1_C1);
        std::cout << "C1 value: " << value << std::endl;
    }
    catch (const ixion::formula_error& e)
    {
        std::cout << "C1 cannot be read as a number: " << e.what() << std::endl;
    }
    //!code-end: numeric-throws

    //!code-start: fix-error
    cxt.set_numeric_cell(Sheet1_B1, 4.0);
    calculate(cxt, {Sheet1_B1}, {});

    print_result(cxt, "C1", Sheet1_C1);
    print_result(cxt, "D1", Sheet1_D1);
    print_result(cxt, "E1", Sheet1_E1);
    //!code-end: fix-error

    //!code-start: circular
    ixion::abs_address_t Sheet1_A3{0, 2, 0};
    ixion::abs_address_t Sheet1_B3{0, 2, 1};
    ixion::abs_address_t Sheet1_C3{0, 2, 2};

    set_formula(cxt, *resolver, Sheet1_A3, "B3+1");
    set_formula(cxt, *resolver, Sheet1_B3, "A3+1");
    set_formula(cxt, *resolver, Sheet1_C3, "A1*2");
    calculate(cxt, {}, {Sheet1_A3, Sheet1_B3, Sheet1_C3});

    print_result(cxt, "A3", Sheet1_A3);
    print_result(cxt, "B3", Sheet1_B3);
    print_result(cxt, "C3", Sheet1_C3);
    //!code-end: circular

    //!code-start: volatile
    ixion::abs_address_t Sheet1_A5{0, 4, 0};
    ixion::abs_address_t Sheet1_B5{0, 4, 1};

    set_formula(cxt, *resolver, Sheet1_A5, "RAND()");
    set_formula(cxt, *resolver, Sheet1_B5, "A5*100");
    calculate(cxt, {}, {Sheet1_A5, Sheet1_B5});

    double A5_first = cxt.get_numeric_value(Sheet1_A5);

    // Nothing has been modified, yet the dirty query still returns cells.
    std::vector<ixion::abs_range_t> sorted = ixion::query_and_sort_dirty_cells(cxt, {}, nullptr);
    std::cout << "dirty cells with nothing modified: " << sorted.size() << std::endl;

    ixion::calculate_sorted_cells(cxt, sorted, 0);
    std::cout << "A5 changed: " << (cxt.get_numeric_value(Sheet1_A5) != A5_first) << std::endl;
    double A5_value = cxt.get_numeric_value(Sheet1_A5);
    double B5_value = cxt.get_numeric_value(Sheet1_B5);
    std::cout << "B5 follows A5: " << (B5_value == A5_value * 100) << std::endl;
    //!code-end: volatile

    //!code-start: not-yet
    ixion::abs_address_t Sheet1_A7{0, 6, 0};
    set_formula(cxt, *resolver, Sheet1_A7, "A1+1");

    try
    {
        double value = cxt.get_numeric_value(Sheet1_A7);
        std::cout << "A7 value: " << value << std::endl;
    }
    catch (const ixion::formula_error& e)
    {
        std::cout << "A7 not calculated yet: " << e.what() << std::endl;
    }

    calculate(cxt, {}, {Sheet1_A7});
    std::cout << "A7 value: " << cxt.get_numeric_value(Sheet1_A7) << std::endl;
    //!code-end: not-yet

    return EXIT_SUCCESS;
}
