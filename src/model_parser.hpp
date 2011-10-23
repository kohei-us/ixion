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
#include "ixion/lexer_tokens.hpp"
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

    /**
     * 'cell' in this context is just a named storage containing lexer tokens.
     * A 'cell' may be either a formula cell, or a named expression.
     */
    class cell
    {
    public:
        cell(const mem_str_buf& name, cell_type type, lexer_tokens_t& tokens);
        cell(const cell& r);
        ~cell();

        ::std::string print() const;
        const mem_str_buf& get_name() const;
        cell_type get_type() const;
        const lexer_tokens_t& get_tokens() const;

    private:
        mem_str_buf m_name;
        cell_type m_type;
        lexer_tokens_t m_tokens;
    };

    model_parser(const ::std::string& filepath, size_t thread_count);
    ~model_parser();

    void parse();

private:
    model_parser(); // disabled

    void parse_init(const char*& p);

    void calc(dirty_cells_t& cells);
    void check(const results_type& formula_results);
    const base_cell* get_cell_from_name(const ::std::string& name);

private:
    std::string m_filepath;
    size_t m_thread_count;
    model_context m_context;
    dirty_cells_t m_dirty_cells;
    bool m_print_separator:1;
};

}

#endif
