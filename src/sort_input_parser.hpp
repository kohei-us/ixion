/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __IXION_SORT_INPUT_PARSER_HXX__
#define __IXION_SORT_INPUT_PARSER_HXX__

#include <ixion/exceptions.hpp>

#include "depth_first_search.hpp"

#include <vector>
#include <string>

namespace ixion {

class sort_input_parser
{
    class parse_error : public general_error
    {
    public:
        parse_error(const ::std::string& msg);
        virtual ~parse_error() throw();
    };

    using dfs_type = depth_first_search<std::string_view, std::hash<std::string_view>>;

public:
    sort_input_parser(const ::std::string& filepath);
    ~sort_input_parser();

    void parse();
    void print();

private:
    void remove_duplicate_cells();
    void insert_depend(const std::string_view& cell, const std::string_view& dep);

private:
    dfs_type::relations    m_set;
    std::string m_content;
    std::vector<std::string_view> m_all_cells;

    const char* mp;
    const char* mp_last;
};

}

#endif
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
