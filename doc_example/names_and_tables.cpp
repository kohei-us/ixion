#include <ixion/model_context.hpp>
#include <ixion/formula_name_resolver.hpp>
#include <ixion/formula.hpp>
#include <ixion/formula_tokens.hpp>
#include <ixion/named_expressions_iterator.hpp>
#include <ixion/address.hpp>
#include <ixion/table.hpp>

#include <iostream>
#include <cstdlib>

//!code-start: set-formula
void set_formula(
    ixion::model_context& cxt, const ixion::formula_name_resolver& resolver,
    const ixion::abs_address_t& pos, std::string_view formula)
{
    ixion::formula_tokens_t tokens = ixion::parse_formula_string(cxt, pos, resolver, formula);
    cxt.set_formula_cell(pos, std::move(tokens));

    ixion::register_formula_cell(cxt, pos);
}
//!code-end: set-formula

//!code-start: calculate
void calculate(ixion::model_context& cxt, const ixion::abs_range_set_t& new_formula_cells)
{
    std::vector<ixion::abs_range_t> sorted =
        ixion::query_and_sort_dirty_cells(cxt, {}, &new_formula_cells);

    ixion::calculate_sorted_cells(cxt, sorted, 0);
}
//!code-end: calculate

//!code-start: print-names
void print_names(
    const ixion::model_context& cxt, const ixion::formula_name_resolver& resolver,
    ixion::named_expressions_iterator iter)
{
    for (; iter.has(); iter.next())
    {
        auto entry = iter.get();
        const ixion::named_expression_t& expr = *entry.expression;
        std::cout << "  " << *entry.name << " = "
            << ixion::print_formula_tokens(cxt, expr.origin, resolver, expr.tokens) << std::endl;
    }
}
//!code-end: print-names

int main()
{
    //!code-start: setup
    ixion::model_context cxt;
    cxt.append_sheet("Sheet1");
    cxt.append_sheet("Sheet2");
    cxt.append_sheet("Inventory");

    auto resolver = ixion::formula_name_resolver::get(
        ixion::formula_name_resolver_t::excel_a1, &cxt);
    //!code-end: setup

    //!code-start: global-name
    for (ixion::row_t row = 0; row < 5; ++row)
        cxt.set_numeric_cell({0, row, 0}, row + 1); // Sheet1!A1:A5 = 1, 2, 3, 4, 5

    ixion::abs_address_t Sheet1_A1{0, 0, 0};
    ixion::formula_tokens_t tokens = ixion::parse_formula_string(
        cxt, Sheet1_A1, *resolver, "Sheet1!$A$1:$A$5");
    cxt.set_named_expression("MyData", Sheet1_A1, std::move(tokens));

    ixion::abs_address_t Sheet1_C1{0, 0, 2};
    set_formula(cxt, *resolver, Sheet1_C1, "SUM(MyData)");
    calculate(cxt, {Sheet1_C1});

    std::cout << "Sheet1!C1 = " << cxt.get_numeric_value(Sheet1_C1) << std::endl;
    //!code-end: global-name

    //!code-start: print-expression
    const ixion::named_expression_t* expr = cxt.get_named_expression(0, "MyData");
    std::cout << "MyData = "
        << ixion::print_formula_tokens(cxt, expr->origin, *resolver, expr->tokens) << std::endl;
    //!code-end: print-expression

    //!code-start: relative-name
    ixion::abs_address_t Sheet1_B1{0, 0, 1};
    tokens = ixion::parse_formula_string(cxt, Sheet1_B1, *resolver, "A1");
    cxt.set_named_expression("LeftCell", Sheet1_B1, std::move(tokens));

    ixion::abs_address_t Sheet1_B2{0, 1, 1};
    ixion::abs_address_t Sheet1_C2{0, 1, 2};
    cxt.set_numeric_cell(Sheet1_B2, 7.0);
    set_formula(cxt, *resolver, Sheet1_C2, "LeftCell*10");
    calculate(cxt, {Sheet1_C2});

    std::cout << "Sheet1!C2 = " << cxt.get_numeric_value(Sheet1_C2) << std::endl;
    //!code-end: relative-name

    //!code-start: sheet-local
    for (ixion::row_t row = 0; row < 3; ++row)
        cxt.set_numeric_cell({1, row, 0}, (row + 1) * 10); // Sheet2!A1:A3 = 10, 20, 30

    ixion::abs_address_t Sheet2_A1{1, 0, 0};
    tokens = ixion::parse_formula_string(cxt, Sheet2_A1, *resolver, "Sheet2!$A$1:$A$3");
    cxt.set_named_expression(1, "MyData", Sheet2_A1, std::move(tokens));

    ixion::abs_address_t Sheet2_C1{1, 0, 2};
    set_formula(cxt, *resolver, Sheet2_C1, "SUM(MyData)");
    calculate(cxt, {Sheet2_C1});

    std::cout << "Sheet1!C1 = " << cxt.get_numeric_value(Sheet1_C1) << std::endl;
    std::cout << "Sheet2!C1 = " << cxt.get_numeric_value(Sheet2_C1) << std::endl;
    //!code-end: sheet-local

    //!code-start: iterate-names
    std::cout << "global names:" << std::endl;
    print_names(cxt, *resolver, cxt.get_named_expressions_iterator());
    std::cout << "names local to Sheet2:" << std::endl;
    print_names(cxt, *resolver, cxt.get_named_expressions_iterator(1));
    //!code-end: iterate-names

    //!code-start: table-data
    cxt.set_cell_values(2, {  // Inventory!A1:B5
        { "Item",  "Amount" },
        { "pen",     1.5    },
        { "book",   12.0    },
        { "bag",    30.0    },
        { "Total",  nullptr },
    });
    //!code-end: table-data

    //!code-start: set-table
    ixion::table_t table;
    table.name = "Table1";
    table.sheet = 2;
    table.range = {0, 0, 5, 2}; // A1:B5, as row, column, row span, column span
    table.columns = { "Item", "Amount" };
    table.totals_row_count = 1;
    cxt.set_table(std::move(table));
    //!code-end: set-table

    //!code-start: table-refs
    ixion::abs_address_t Inventory_B5{2, 4, 1};
    ixion::abs_address_t Inventory_D1{2, 0, 3};
    ixion::abs_address_t Inventory_D2{2, 1, 3};
    ixion::abs_address_t Inventory_D3{2, 2, 3};

    // inside the table
    set_formula(cxt, *resolver, Inventory_B5, "SUM([Amount])");
    // data rows only
    set_formula(cxt, *resolver, Inventory_D1, "SUM(Table1[Amount])");
    // header row
    set_formula(cxt, *resolver, Inventory_D2, "COUNTA(Table1[#Headers])");
    // totals row
    set_formula(cxt, *resolver, Inventory_D3, "SUM(Table1[[#Totals],[Amount]])");

    calculate(cxt, {Inventory_B5, Inventory_D1, Inventory_D2, Inventory_D3});

    std::cout << "Inventory!B5 = " << cxt.get_numeric_value(Inventory_B5) << std::endl;
    std::cout << "Inventory!D1 = " << cxt.get_numeric_value(Inventory_D1) << std::endl;
    std::cout << "Inventory!D2 = " << cxt.get_numeric_value(Inventory_D2) << std::endl;
    std::cout << "Inventory!D3 = " << cxt.get_numeric_value(Inventory_D3) << std::endl;
    //!code-end: table-refs

    //!code-start: table-range
    ixion::abs_range_t range = cxt.get_table_range("Table1", "Amount", {}, ixion::table_area_data);
    std::cout << "Table1[Amount] -> " << range << std::endl;

    range = cxt.get_table_range("Table1", {}, {}, ixion::table_area_all);
    std::cout << "Table1[#All]   -> " << range << std::endl;
    //!code-end: table-range

    //!code-start: list-tables
    for (const ixion::table_t* tab : cxt.get_tables(2))
        std::cout << "table on Inventory: " << tab->name << std::endl;
    //!code-end: list-tables

    return EXIT_SUCCESS;
}
