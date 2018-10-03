/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __IXION_SORT_INPUT_PARSER_HXX__
#define __IXION_SORT_INPUT_PARSER_HXX__

#include "ixion/depth_first_search.hpp"
#include "ixion/mem_str_buf.hpp"
#include "ixion/exceptions.hpp"

#include <vector>

namespace ixion {

class sort_input_parser
{
    class parse_error : public general_error
    {
    public:
        parse_error(const ::std::string& msg);
        virtual ~parse_error() throw();
    };

    class cell_handler : public ::std::unary_function<mem_str_buf, void>
    {
    public:
        cell_handler(::std::vector<mem_str_buf>& sorted);
        void operator() (const mem_str_buf& s);
    private:
        ::std::vector<mem_str_buf>& m_sorted;
    };

    typedef depth_first_search<mem_str_buf, cell_handler, mem_str_buf::hash> dfs_type;

public:
    sort_input_parser(const ::std::string& filepath);
    ~sort_input_parser();

    void parse();
    void print();

private:
    void remove_duplicate_cells();
    void insert_depend(const mem_str_buf& cell, const mem_str_buf& dep);

private:
    dfs_type::relations    m_set;
    ::std::string           m_content;
    ::std::vector<mem_str_buf>   m_all_cells;

    const char* mp;
    const char* mp_last;
};

}

#endif
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
