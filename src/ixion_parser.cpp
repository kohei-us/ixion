/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "model_parser.hpp"

#include <string>
#include <vector>
#include <iostream>
#include <thread>

#include <boost/program_options.hpp>

using namespace std;
using namespace ixion;

namespace {

class parse_file
{
    const size_t m_thread_count;
public:
    parse_file(size_t thread_count) : m_thread_count(thread_count) {}

    void operator() (const string& fpath) const
    {
        double start_time = global::get_current_time();
        cout << get_formula_result_output_separator() << endl;
        cout << "parsing " << fpath << endl;

        try
        {
            model_parser parser(fpath, m_thread_count);
            parser.parse();
        }
        catch (const exception& e)
        {
            cerr << e.what() << endl;
            cerr << "failed to parse " << fpath << endl;
            throw;
        }

        cout << get_formula_result_output_separator() << endl;
        cout << "(duration: " << global::get_current_time() - start_time << " sec)" << endl;
        cout << get_formula_result_output_separator() << endl;
    }
};

const char* help_thread =
"Specify the number of threads to use for calculation.  Note that the number "
"specified by this option corresponds with the number of calculation threads "
"i.e. those child threads that perform cell interpretations. The main thread "
"does not perform any calculations; instead, it creates a new child thread to "
"manage the calculation threads, the number of which is specified by the arg. "
"Therefore, the total number of threads used by this program will be arg + 1."
;

}

int main (int argc, char** argv)
{
    namespace po = ::boost::program_options;

    size_t thread_count = 0;
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "Print this help.")
        ("thread,t", po::value<size_t>(), help_thread);

    po::options_description hidden("Hidden options");
    hidden.add_options()
        ("input-file", po::value<std::vector<std::string>>(), "input file");

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
        cout << e.what() << endl;
        cout << desc;
        return EXIT_FAILURE;
    }

    if (vm.count("help"))
    {
        cout << "Usage: ixion-parser [options] FILE1 FILE2 ..." << endl
            << endl
            << "The FILE must contain the definitions of cells according to the cell definition rule." << endl << endl
            << desc;
        return EXIT_SUCCESS;
    }

    ixion::init();

    if (vm.count("thread"))
        thread_count = vm["thread"].as<size_t>();

    vector<string> files;
    if (vm.count("input-file"))
        files = vm["input-file"].as< vector<string> >();

    if (thread_count > 0)
    {
        cout << "Using " << thread_count << " threads" << endl;
        cout << "Number of CPUS: " << std::thread::hardware_concurrency() << endl;
    }

    try
    {
        // Parse all files one at a time.
        for_each(files.begin(), files.end(), parse_file(thread_count));
    }
    catch (const exception&)
    {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
