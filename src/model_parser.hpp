/*************************************************************************
 *
 * Copyright (c) 2010, 2011 Kohei Yoshida
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

#ifndef __IXION_MODEL_PARSER_HPP__
#define __IXION_MODEL_PARSER_HPP__

#include "ixion/cell.hpp"
#include "ixion/exceptions.hpp"
#include "ixion/model_context.hpp"
#include "ixion/hash_container/map.hpp"

#include <string>
#include <exception>
#include <vector>

#include <boost/noncopyable.hpp>

namespace ixion {

class formula_result;

class model_parser : public ::boost::noncopyable
{
public:
    typedef _ixion_unordered_map_type< ::std::string, formula_result> results_type;

    class parse_error : public ::std::exception
    {
    public:
        explicit parse_error(const ::std::string& msg);
        ~parse_error() throw();
        virtual const char* what() const throw();
    private:
        ::std::string m_msg;
    };

    class check_error : public general_error
    {
    public:
        check_error(const ::std::string& msg);
    };

    /**
     * Right-hand-side cell content type.
     */
    enum cell_type
    {
        ct_unknown = 0,
        ct_formula,
        ct_value,
        ct_string
    };

    model_parser(const ::std::string& filepath, size_t thread_count);
    ~model_parser();

    void parse();

private:
    model_parser(); // disabled

    void parse_init(const char*& p);
    void parse_result(const char*& p);

    void check();

private:
    model_context m_context;
    std::string m_filepath;
    size_t m_thread_count;
    dirty_formula_cells_t m_dirty_cells;
    modified_cells_t m_dirty_cell_addrs;
    results_type m_formula_results;
    bool m_print_separator:1;
};

}

#endif
