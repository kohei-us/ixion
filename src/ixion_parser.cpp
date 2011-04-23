/*************************************************************************
 *
 * Copyright (c) 2010 Kohei Yoshida
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

#include "ixion/model_parser.hpp"

#include <string>
#include <vector>
#include <iostream>

#include <boost/thread.hpp>
#include <boost/program_options.hpp>

using namespace std;
using namespace ixion;

int main (int argc, char** argv)
{
    namespace po = ::boost::program_options;

    size_t thread_count = 0;
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "print this help.")
        ("thread,t", po::value<size_t>(), 
         "specify the number of threads to use for calculation.  Note that the number specified by this option corresponds with the number of calculation threads i.e. those child threads that perform cell interpretations.  The main thread does not perform any calculations; instead, it creates a new child thread to manage the calculation threads, the number of which is specified by the arg.  Therefore, the total number of threads used by this program will be arg + 2.");

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

    if (vm.count("thread"))
        thread_count = vm["thread"].as<size_t>();

    vector<string> files;
    if (vm.count("input-file"))
        files = vm["input-file"].as< vector<string> >();

    if (thread_count > 0)
    {
        cout << "Using " << thread_count << " threads" << endl;
        cout << "Number of CPUS: " << boost::thread::hardware_concurrency() << endl;
    }

    // Parse all files one at a time.
    vector<string>::const_iterator itr = files.begin(), itr_end = files.end();
    for (; itr != itr_end; ++itr)
    {
        const string& fpath = *itr;
        double start_time = global::get_current_time();
        cout << get_formula_result_output_separator() << endl;
        cout << "parsing " << fpath << endl;

        try
        {
            model_parser parser(fpath, thread_count);
            parser.parse();
        }
        catch (const exception& e)
        {
            cerr << e.what() << endl;
            cerr << "failed to parse " << fpath << endl;
            return EXIT_FAILURE;
        }

        cout << get_formula_result_output_separator() << endl;
        cout << "(duration: " << global::get_current_time() - start_time << " sec)" << endl;
        cout << get_formula_result_output_separator() << endl;
    }

    return EXIT_SUCCESS;
}

