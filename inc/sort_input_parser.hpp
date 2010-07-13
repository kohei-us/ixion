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

#ifndef __IXION_SORT_INPUT_PARSER_HXX__
#define __IXION_SORT_INPUT_PARSER_HXX__

#include "depth_first_search.hpp"
#include "mem_str_buf.hpp"
#include "global.hpp"

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

    typedef depth_first_search<mem_str_buf, cell_handler> dfs_type;

public:
    sort_input_parser(const ::std::string& filepath);
    ~sort_input_parser();

    void parse();
    void print();

private:
    void insert_depend(const mem_str_buf& cell, const mem_str_buf& dep);

private:
    dfs_type::depend_set    m_set;
    ::std::string           m_content;
    ::std::vector<mem_str_buf>   m_all_cells;

    const char* mp;
    const char* mp_last;
};

}

#endif
