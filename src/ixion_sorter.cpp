/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "sort_input_parser.hpp"

#include <cstdlib>
#include <string>
#include <vector>
#include <iostream>

#include <boost/program_options.hpp>

using namespace std;

namespace po = ::boost::program_options;

void print_help(const po::options_description& desc)
{
    cout << "Usage: ixion-parser [options] FILE" << endl
        << endl
        << "FILE must contain a list of dependencies." << endl << endl
        << desc;
}

int main (int argc, char** argv)
{
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "print this help.");

    po::options_description hidden("Hidden options");
    hidden.add_options()
        ("input-file", po::value< vector<string> >(), "input file");

    po::options_description cmd_opt;
    cmd_opt.add(desc).add(hidden);

    po::positional_options_description po_desc;
    po_desc.add("input-file", -1);

    po::variables_map vm;
    try
    {
        po::store(
            po::command_line_parser(argc, argv).options(cmd_opt).positional(po_desc).run(), vm);
        po::notify(vm);
    }
    catch (const exception& e)
    {
        // Unknown options.
        cerr << e.what() << endl;
        print_help(desc);
        return EXIT_FAILURE;
    }

    if (vm.count("help"))
    {
        print_help(desc);
        return EXIT_SUCCESS;
    }

    vector<string> files;
    if (vm.count("input-file"))
        files = vm["input-file"].as< vector<string> >();

    if (files.size() != 1)
    {
        cerr << "Takes exactly one input file." << endl;
        print_help(desc);
        return EXIT_FAILURE;
    }

    const string& filepath = files[0];
    ::ixion::sort_input_parser parser(filepath);
    parser.parse();
    parser.print();

    return EXIT_SUCCESS;
}
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
