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

#include "sort_input_parser.hpp"
#include "global.hpp"

#include <fstream>
#include <iostream>

using namespace std;

namespace std {

// Template specialization for mem_str_buf.
template<>
size_t hash<ixion::mem_str_buf>::operator () (ixion::mem_str_buf s) const
{
    size_t n = s.size();
    size_t hash_val = 0;
    for (size_t i = 0; i < n; ++i)
        hash_val += (s[i] << i);
    return hash_val;
}

}

namespace ixion {

sort_input_parser::cell_handler::cell_handler(vector<mem_str_buf>& sorted) :
    m_sorted(sorted) {}

void sort_input_parser::cell_handler::operator() (const mem_str_buf& s)
{
    m_sorted.push_back(s);
}

// ============================================================================

sort_input_parser::sort_input_parser(const string& filepath)
{
    global::load_file_content(filepath, m_content);
}

sort_input_parser::~sort_input_parser()
{
}

void sort_input_parser::parse()
{
    cout << m_content << endl;
}

void sort_input_parser::print()
{
    vector<mem_str_buf> cells;
    cell_handler handler(cells);
    dfs_type dfs(cells, m_set.get(), handler);
}

void sort_input_parser::insert_depend(const mem_str_buf& cell, const mem_str_buf& dep)
{
    m_set.insert(cell, dep);
}


}
