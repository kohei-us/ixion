#include <ixion/model_context.hpp>
#include <ixion/formula_name_resolver.hpp>
#include <ixion/formula.hpp>
#include <ixion/address.hpp>
#include <ixion/config.hpp>

#include <iostream>
#include <cstdlib>

int main()
{
    //!code-start: setup
    ixion::model_context cxt;
    cxt.append_sheet("Sheet1");
    cxt.append_sheet("Sheet2");
    cxt.append_sheet("Annual Report");

    auto excel_a1 = ixion::formula_name_resolver::get(
        ixion::formula_name_resolver_t::excel_a1, &cxt);

    auto excel_r1c1 = ixion::formula_name_resolver::get(
        ixion::formula_name_resolver_t::excel_r1c1, &cxt);

    auto calc_a1 = ixion::formula_name_resolver::get(
        ixion::formula_name_resolver_t::calc_a1, &cxt);

    auto odff = ixion::formula_name_resolver::get(
        ixion::formula_name_resolver_t::odff, &cxt);
    //!code-end: setup

    //!code-start: parse
    ixion::abs_address_t Sheet1_C1{0, 0, 2};
    std::string formula = "SUM(A1:A10)+Sheet2!B1*2";

    ixion::formula_tokens_t tokens = ixion::parse_formula_string(
        cxt, Sheet1_C1, *excel_a1, formula);

    std::cout << "number of tokens: " << tokens.size() << std::endl;
    for (const ixion::formula_token& t : tokens)
        std::cout << "  " << t << std::endl;
    //!code-end: parse

    //!code-start: print
    std::cout << "excel_a1:   "
        << ixion::print_formula_tokens(cxt, Sheet1_C1, *excel_a1, tokens) << std::endl;
    std::cout << "excel_r1c1: "
        << ixion::print_formula_tokens(cxt, Sheet1_C1, *excel_r1c1, tokens) << std::endl;
    std::cout << "calc_a1:    "
        << ixion::print_formula_tokens(cxt, Sheet1_C1, *calc_a1, tokens) << std::endl;
    std::cout << "odff:       "
        << ixion::print_formula_tokens(cxt, Sheet1_C1, *odff, tokens) << std::endl;
    //!code-end: print

    //!code-start: absolute
    ixion::abs_address_t Sheet1_B2{0, 1, 1};
    tokens = ixion::parse_formula_string(cxt, Sheet1_B2, *excel_a1, "$A$1+A$1+$A1+A1");

    std::cout << "excel_a1:   "
        << ixion::print_formula_tokens(cxt, Sheet1_B2, *excel_a1, tokens) << std::endl;
    std::cout << "excel_r1c1: "
        << ixion::print_formula_tokens(cxt, Sheet1_B2, *excel_r1c1, tokens) << std::endl;
    //!code-end: absolute

    //!code-start: sheet-relative
    ixion::abs_address_t Sheet2_B2{1, 1, 1};
    ixion::print_config config;
    config.display_sheet = ixion::display_sheet_t::always;

    // parse on Sheet1, print as if the formula had moved to Sheet2
    tokens = ixion::parse_formula_string(cxt, Sheet1_B2, *excel_a1, "A1+Sheet1!A1");
    std::cout << "excel_a1: "
        << ixion::print_formula_tokens(config, cxt, Sheet2_B2, *excel_a1, tokens) << std::endl;

    tokens = ixion::parse_formula_string(cxt, Sheet1_B2, *calc_a1, "A1+Sheet1.A1+$Sheet1.A1");
    std::cout << "calc_a1:  "
        << ixion::print_formula_tokens(config, cxt, Sheet2_B2, *calc_a1, tokens) << std::endl;
    //!code-end: sheet-relative

    //!code-start: quoted-sheet
    tokens = ixion::parse_formula_string(
        cxt, Sheet1_B2, *excel_a1, "'Annual Report'!A1:B2");

    std::cout << "excel_a1: "
        << ixion::print_formula_tokens(cxt, Sheet1_B2, *excel_a1, tokens) << std::endl;
    std::cout << "calc_a1:  "
        << ixion::print_formula_tokens(cxt, Sheet1_B2, *calc_a1, tokens) << std::endl;
    //!code-end: quoted-sheet

    //!code-start: literals
    tokens = ixion::parse_formula_string(
        cxt, Sheet1_B2, *excel_a1, "IF(A1>=10,\"big\",\"small\")&{1,2;3,4}");

    std::cout << ixion::print_formula_tokens(cxt, Sheet1_B2, *excel_a1, tokens) << std::endl;
    //!code-end: literals

    //!code-start: names
    tokens = ixion::parse_formula_string(cxt, Sheet1_B2, *excel_a1, "sum(MyRange)");

    for (const ixion::formula_token& t : tokens)
        std::cout << "  " << t << std::endl;
    //!code-end: names

    //!code-start: parse-error
    try
    {
        tokens = ixion::parse_formula_string(cxt, Sheet1_B2, *excel_a1, "A1+#FOO!");
    }
    catch (const std::exception& e)
    {
        std::cout << "parse failed: " << e.what() << std::endl;
    }
    //!code-end: parse-error

    return EXIT_SUCCESS;
}
