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

#include "inputparser.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include <string>
#include <iostream>

using namespace std;
using namespace ixion;

namespace {

void print_help()
{
    cout << "usage: inputparser [options] FILE" << endl
         << endl
         << "The FILE must contain the definitions of cells according to the cell difinion rule." << endl
         << "Options:" << endl
         << "  -h      print this help." << endl;
}

}

int main (int argc, char** argv)
{
    /* Flag set by '--verbose'. */
    static int verbose_flag;
    static struct option long_options[] =
    {
        /* These options set a flag. */
        {"verbose", no_argument,       &verbose_flag, 1},
        {"brief",   no_argument,       &verbose_flag, 0},
        {"thread", no_argument, 0, 't'},
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
    bool use_thread = false;

    while (true)
    {
        /* getopt_long stores the option index here. */
        int option_index = 0;
        int c = getopt_long (argc, argv, "htc:d:l:", long_options, &option_index);

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
                use_thread = true;
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

    if (use_thread)
        cout << "Using threads" << endl;

    for (int i = optind; i < argc; ++i)
    {
        string fpath = argv[i];
        double start_time = get_current_time();
        cout << "----------------------------------------------------------------------" << endl;
        cout << "parsing " << fpath << endl;
        cout << "----------------------------------------------------------------------" << endl;
        if (!parse_model_input(fpath, dotgraph_path, use_thread))
        {
            cerr << "failed to parse " << fpath << endl;
            exit (EXIT_FAILURE);
        }

        cout << "(duration: " << get_current_time() - start_time << " sec)" << endl;
    }
}

