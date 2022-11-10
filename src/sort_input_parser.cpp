/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "sort_input_parser.hpp"
#include "app_common.hpp"

#include <ixion/global.hpp>

#include <fstream>
#include <iostream>
#include <algorithm>

using namespace std;

namespace ixion {

size_t hash_value(const std::string_view& s)
{
    size_t n = s.size();
    size_t hash_val = 0;
    for (size_t i = 0; i < n; ++i)
        hash_val += (s[i] << i);
    return hash_val;
}


namespace {

struct mem_str_buf_printer
{
    void operator() (const std::string_view& r) const
    {
        std::cout << r << std::endl;
    }
};

}

sort_input_parser::parse_error::parse_error(const string& msg) :
    general_error(msg) {}

sort_input_parser::parse_error::~parse_error() throw() {}

// ----------------------------------------------------------------------------

sort_input_parser::sort_input_parser(const string& filepath) :
    m_content(detail::load_file_content(filepath)),
    mp(nullptr), mp_last(nullptr)
{
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
    std::string_view cell, dep;
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

                cell = std::string_view{};
                dep = std::string_view{};
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
                {
                    if (cell.empty())
                        cell = std::string_view{mp, 1u};
                    else
                        cell = std::string_view{cell.data(), cell.size() + 1u};
                }
                else
                {
                    if (dep.empty())
                        dep = std::string_view{mp, 1u};
                    else
                        dep = std::string_view{dep.data(), dep.size() + 1u};
                }
        }
    }
}

void sort_input_parser::print()
{
    remove_duplicate_cells();

    // Run the depth first search.
    vector<std::string_view> sorted;
    sorted.reserve(m_all_cells.size());
    dfs_type::back_inserter handler(sorted);
    dfs_type dfs(m_all_cells.begin(), m_all_cells.end(), m_set, handler);
    dfs.run();

    // Print the result.
    for_each(sorted.begin(), sorted.end(), mem_str_buf_printer());
}

void sort_input_parser::remove_duplicate_cells()
{
    sort(m_all_cells.begin(), m_all_cells.end());
    vector<std::string_view>::iterator itr = unique(m_all_cells.begin(), m_all_cells.end());
    m_all_cells.erase(itr, m_all_cells.end());
}

void sort_input_parser::insert_depend(const std::string_view& cell, const std::string_view& dep)
{
    m_set.insert(cell, dep);
    m_all_cells.push_back(cell);
    m_all_cells.push_back(dep);
}

}
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
