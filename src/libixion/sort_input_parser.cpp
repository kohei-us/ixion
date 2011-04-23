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

#include "ixion/sort_input_parser.hpp"
#include "ixion/global.hpp"

#include <fstream>
#include <iostream>

using namespace std;

namespace ixion {

size_t hash_value(const ixion::mem_str_buf& s)
{
    size_t n = s.size();
    size_t hash_val = 0;
    for (size_t i = 0; i < n; ++i)
        hash_val += (s[i] << i);
    return hash_val;
}


namespace {

struct mem_str_buf_printer : unary_function<mem_str_buf, void>
{
    void operator() (const mem_str_buf& r) const
    {
        cout << r.str() << endl;
    }
};

}

sort_input_parser::cell_handler::cell_handler(vector<mem_str_buf>& sorted) :
    m_sorted(sorted) {}

void sort_input_parser::cell_handler::operator() (const mem_str_buf& s)
{
    m_sorted.push_back(s);
}

// ============================================================================

sort_input_parser::parse_error::parse_error(const string& msg) : 
    general_error(msg) {}

sort_input_parser::parse_error::~parse_error() throw() {}

// ----------------------------------------------------------------------------

sort_input_parser::sort_input_parser(const string& filepath)
{
    global::load_file_content(filepath, m_content);
}

sort_input_parser::~sort_input_parser()
{
}

void sort_input_parser::parse()
{
    if (m_content.empty())
        return;

    mp = &m_content[0];
    mp_last = &m_content[m_content.size()-1];
    mem_str_buf cell, dep;
    bool in_name = true;
    for (;mp != mp_last; ++mp)
    {
        switch (*mp)
        {
            case ' ':
                // Let's skip blanks for now.
                break;
            case '\n':
                // end of line.
                if (cell.empty())
                {
                    if (!dep.empty())
                        throw parse_error("cell name is emtpy but dependency name isn't.");
                }
                else
                {
                    if (dep.empty())
                        throw parse_error("dependency name is empty.");
                    else
                        insert_depend(cell, dep);
                }

                cell.clear();
                dep.clear();
                in_name = true;
            break;
            case ':':
                if (!dep.empty())
                    throw parse_error("more than one separator in a single line!");
                if (cell.empty())
                    throw parse_error("cell name is empty");
                in_name = false;
            break;
            default:
                if (in_name)
                    cell.append(mp);
                else
                    dep.append(mp);
        }
    }
}

void sort_input_parser::print()
{
    remove_duplicate_cells();

    // Run the depth first search.
    vector<mem_str_buf> sorted;
    sorted.reserve(m_all_cells.size());
    cell_handler handler(sorted);
    dfs_type dfs(m_all_cells, m_set.get(), handler);
    dfs.run();

    // Print the result.
    for_each(sorted.begin(), sorted.end(), mem_str_buf_printer());
}

void sort_input_parser::remove_duplicate_cells()
{
    sort(m_all_cells.begin(), m_all_cells.end());
    vector<mem_str_buf>::iterator itr = unique(m_all_cells.begin(), m_all_cells.end());
    m_all_cells.erase(itr, m_all_cells.end());
}

void sort_input_parser::insert_depend(const mem_str_buf& cell, const mem_str_buf& dep)
{
    m_set.insert(cell, dep);
    m_all_cells.push_back(cell);
    m_all_cells.push_back(dep);
}

}
