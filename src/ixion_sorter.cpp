/*************************************************************************
 *
 * Copyright (c) 2010, 2011 Kohei Yoshida
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 ************************************************************************/

#include "ixion/sort_input_parser.hpp"

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
