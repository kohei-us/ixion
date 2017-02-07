/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef IXION_FORMULA_PARSER_CPP
#define IXION_FORMULA_PARSER_CPP

#include <string>
#include <vector>
#include <iostream>

#include <boost/program_options.hpp>

#include "ixion/formula.hpp"
#include "ixion/model_context.hpp"
#include "ixion/formula_name_resolver.hpp"

using std::cout;
using std::endl;

void tokenize_formula(const std::string& formula)
{
    using namespace ixion;

    model_context cxt;

    std::unique_ptr<formula_name_resolver> resolver =
        formula_name_resolver::get(formula_name_resolver_t::excel_a1, &cxt);

    abs_address_t pos;

    formula_tokens_t tokens = parse_formula_string(
        cxt, pos, *resolver, formula.data(), formula.size());

    cout << "original formula string: " << formula << endl;

    std::string normalized = print_formula_tokens(cxt, pos, *resolver, tokens);
    cout << "normalized formula string: " << normalized << endl;

    cout << "individual tokens:" << endl;
    for (const formula_tokens_t::value_type& tp : tokens)
        cout << "  * " << *tp << endl;
}

int main (int argc, char** argv)
{
    namespace po = ::boost::program_options;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "print this help.")
        ("sheets", po::value<std::string>(), "Sheet names.");

    po::options_description hidden("Hidden options");
    hidden.add_options()
        ("formula-expression", po::value<std::string>(), "formula expression");

    po::options_description cmd_opt;
    cmd_opt.add(desc).add(hidden);

    po::positional_options_description po_desc;
    po_desc.add("formula-expression", -1);

    po::variables_map vm;
    try
    {
        po::store(
            po::command_line_parser(argc, argv).options(cmd_opt).positional(po_desc).run(), vm);
        po::notify(vm);
    }
    catch (const std::exception& e)
    {
        // Unknown options.
        cout << e.what() << endl;
        cout << desc;
        return EXIT_FAILURE;
    }

    auto print_help = [desc]()
    {
        cout << "Usage: ixion-formula-tokenizer [options] FORMULA_EXPRESSION" << endl
            << endl
            << desc;
    };

    if (vm.count("help"))
    {
        print_help();
        return EXIT_SUCCESS;
    }

    if (!vm.count("formula-expression"))
    {
        cout << "formula expression is not given." << endl;
        print_help();
        return EXIT_FAILURE;
    }

    std::string formula = vm["formula-expression"].as<std::string>();
    tokenize_formula(formula);

    return EXIT_SUCCESS;
}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
