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

#include "model_parser.hpp"

#ifdef WIN32
#define USE_BOOST_PROGRAM_OPTIONS 1
#else
#define USE_BOOST_PROGRAM_OPTIONS 0
#endif

#include <stdio.h>
#include <stdlib.h>
#if USE_BOOST_PROGRAM_OPTIONS
#include <boost/program_options.hpp>
#else
#include <getopt.h>
#endif

#include <string>
#include <vector>
#include <iostream>

#include <boost/thread.hpp>

using namespace std;
using namespace ixion;

namespace {

#if !USE_BOOST_PROGRAM_OPTIONS
void print_help()
{
    cout << "usage: ixion-parser [options] FILE1 FILE2 ..." << endl
         << endl
         << "The FILE must contain the definitions of cells according to the cell difintion rule." << endl << endl
         << "Options:" << endl
         << "  -h                print this help." << endl
         << "  -t n,--thread=n   specify the number of threads to use during calculation.  Note that the number" << endl
         << "                    specified by this option corresponds with the number of calculation threads i.e." << endl
         << "                    those threads that perform cell interpretations.  The total number of threads " << endl
         << "                    used by this program will be n + 2." << endl;
}
#endif

}

int main (int argc, char** argv)
{
    size_t thread_count = 0;
#if USE_BOOST_PROGRAM_OPTIONS
    int optind = 1;
    namespace po = ::boost::program_options;
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "print this help.")
        ("thread,t", po::value<size_t>(), 
         "specify the number of threads to use for calculation.  Note that the number specified by this option corresponds with the number of calculation threads i.e. those threads that perform cell interpretations.  The total number of threads used by this program will be n + 2.");

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
            << "The FILE must contain the definitions of cells according to the cell difintion rule." << endl << endl
            << desc;
        return (EXIT_SUCCESS);
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
    vector<string>::const_iterator itr = files.begin(), itr_end = files.end();
    for (; itr != itr_end; ++itr)
    {
        const string& fpath = *itr;
        double start_time = get_current_time();
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
            exit (EXIT_FAILURE);
        }

        cout << get_formula_result_output_separator() << endl;
        cout << "(duration: " << get_current_time() - start_time << " sec)" << endl;
        cout << get_formula_result_output_separator() << endl;
    }

#else
    /* Flag set by '--verbose'. */
    static int verbose_flag;
    static struct option long_options[] =
    {
        /* These options set a flag. */
        {"verbose", no_argument,       &verbose_flag, 1},
        {"brief",   no_argument,       &verbose_flag, 0},
        {"thread", required_argument, 0, 't'},
        /* These options don't set a flag.
           We distinguish them by their indices. */
//      {"add",     no_argument,       0, 'a'},
//      {"append",  no_argument,       0, 'b'},
//      {"delete",  required_argument, 0, 'd'},
//      {"create",  required_argument, 0, 'c'},
        {"model-list",    required_argument, 0, 'l'},
        {0, 0, 0, 0}
    };

    string model_list_path;
    string dotgraph_path;

    while (true)
    {
        /* getopt_long stores the option index here. */
        int option_index = 0;
        int c = getopt_long (argc, argv, "ht:c:d:l:", long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch (c)
        {
            case 0:
            {
                /* If this option set a flag, do nothing else now. */
                if (long_options[option_index].flag != 0)
                    break;

                const char* opt_name = long_options[option_index].name;

                printf ("option %s", opt_name);
                if (optarg)
                    printf (" with arg %s", optarg);
                printf ("\n");
                break;
            }
            case 'h':
                print_help();
                exit(EXIT_SUCCESS);

            case 'c':
                printf ("option -c with value `%s'\n", optarg);
                break;
            case 't':
                thread_count = static_cast<size_t>(strtod(optarg, NULL));
                if (!thread_count)
                {
                    cerr << "thread count must be a positive integer." << endl;
                    abort();
                }
                break;

            case 'd':
                dotgraph_path = optarg;
                break;

            case 'l':
                model_list_path = optarg;
                break;

            case '?':
                /* getopt_long already printed an error message. */
                exit (EXIT_FAILURE);
                break;

            default:
                abort ();
        }
    }
    if (thread_count > 0)
    {
        cout << "Using " << thread_count << " threads" << endl;
        cout << "Number of CPUS: " << boost::thread::hardware_concurrency() << endl;
    }

    for (int i = optind; i < argc; ++i)
    {
        string fpath = argv[i];
        double start_time = get_current_time();
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
            exit (EXIT_FAILURE);
        }

        cout << get_formula_result_output_separator() << endl;
        cout << "(duration: " << get_current_time() - start_time << " sec)" << endl;
        cout << get_formula_result_output_separator() << endl;
    }
#endif
    return (EXIT_SUCCESS);
}

