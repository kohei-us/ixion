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

#ifndef __IXION_INPUTPARSER_HPP__
#define __IXION_INPUTPARSER_HPP__

#include "cell.hpp"
#include "lexer_tokens.hpp"

#include <string>
#include <exception>
#include <vector>

#include <boost/noncopyable.hpp>

namespace ixion {

bool parse_model_input(const ::std::string& fpath, const ::std::string& dotpath);

class model_parser : public ::boost::noncopyable
{
public:
    class file_not_found : public ::std::exception
    {
    public:
        explicit file_not_found(const ::std::string& fpath);
        ~file_not_found() throw();
        virtual const char* what() const throw();
    private:
        ::std::string m_fpath;
    };

    class parse_error : public ::std::exception
    {
    public:
        explicit parse_error(const ::std::string& msg);
        ~parse_error() throw();
        virtual const char* what() const throw();
    private:
        ::std::string m_msg;
    };

    class cell
    {
    public:
        cell(const ::std::string& name, lexer_tokens_t& tokens);
        cell(const cell& r);
        ~cell();
    
        const char* print() const;
        const ::std::string& get_name() const;
        const lexer_tokens_t& get_tokens() const;
    
    private:
        ::std::string m_name;
        lexer_tokens_t m_tokens;
    };

    model_parser(const ::std::string& filepath);
    ~model_parser();

    void parse();
    const ::std::vector<cell>& get_cells() const;
    const ::std::vector< ::std::string>& get_cell_names() const;
private:
    model_parser(); // disabled

private:
    ::std::vector<cell>             m_fcells;
    ::std::vector< ::std::string>   m_cell_names;
    ::std::string m_filepath;
};

}

#endif
