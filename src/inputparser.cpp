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
#include "cell.hpp"
#include "formula_lexer.hpp"
#include "formula_parser.hpp"

#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>
#include <functional>

using namespace std;

namespace ixion {

namespace {

void flush_buffer(vector<char>& buf, string& str)
{
    buf.push_back(0); // null-terminate the buffer.
    str = &buf[0];
    buf.clear();
}

}

bool parse_model_input(const string& fpath)
{
    model_parser parser(fpath);
    try
    {
        parser.parse();
        const vector<model_parser::cell>& cells = parser.get_cells();
        for (size_t i = 0; i < cells.size(); ++i)
        {    
            cout << "cell (" << cells[i].get_name() << "): " << cells[i].print() << endl;
            formula_parser fparser(cells[i].get_tokens());
            fparser.parse();
        }
    }
    catch (const exception& e)
    {
        cout << e.what() << endl;
        return false;
    }
    return true;
}

// ============================================================================

model_parser::file_not_found::file_not_found(const string& fpath) : 
    m_fpath(fpath)
{
}

model_parser::file_not_found::~file_not_found() throw()
{
}

const char* model_parser::file_not_found::what() const throw()
{
    ostringstream oss;
    oss << "specified file not found: " << m_fpath;
    return oss.str().c_str();
}

// ============================================================================

model_parser::parse_error::parse_error(const string& msg) :
    m_msg(msg)
{
}

model_parser::parse_error::~parse_error() throw()
{
}

const char* model_parser::parse_error::what() const throw()
{
    ostringstream oss;
    oss << "parse error: " << m_msg;
    return oss.str().c_str();
}

// ============================================================================

model_parser::cell::cell(const string& name, lexer_tokens_t& tokens) :
    m_name(name)
{
    // Note that this will empty the passed token container !
    m_tokens.swap(tokens);
}

model_parser::cell::cell(const model_parser::cell& r) :
    m_name(r.m_name),
    m_tokens(r.m_tokens)
{
}

model_parser::cell::~cell()
{
}

const char* model_parser::cell::print() const
{
    return print_tokens(m_tokens, false);
}

const string& model_parser::cell::get_name() const
{
    return m_name;
}

const lexer_tokens_t& model_parser::cell::get_tokens() const
{
    return m_tokens;
}

// ============================================================================

model_parser::model_parser(const string& filepath) :
    m_filepath(filepath)
{
}

model_parser::~model_parser()
{
}

void model_parser::parse()
{
    ifstream file(m_filepath.c_str());
    if (!file)
        // failed to open the specified file.
        throw file_not_found(m_filepath);

    char c;
    string name, formula;
    vector<char> buf;
    buf.reserve(255);
    vector<cell> fcells;
    while (file.get(c))
    {
        switch (c)
        {
            case '=':
                if (buf.empty())
                    throw parse_error("left hand side is empty");
                flush_buffer(buf, name);
                break;
            case '\n':
                if (!buf.empty())
                {
                    if (name.empty())
                        throw parse_error("'=' is missing");

                    flush_buffer(buf, formula);

                    // tokenize the formula string, and create a formula cell 
                    // with the tokens.
                    formula_lexer lexer(formula);
                    lexer.tokenize();
                    lexer_tokens_t tokens;
                    lexer.swap_tokens(tokens);

                    // test-print tokens.
                    cout << "tokens from lexer: " << print_tokens(tokens, true) << endl;

                    cell fcell(name, tokens);
                    fcells.push_back(fcell);
                }
                name.clear();
                formula.clear();
                break;
            default:
                buf.push_back(c);
        }
    }
    m_fcells.swap(fcells);
}

const vector<model_parser::cell>& model_parser::get_cells() const
{
    return m_fcells;
}

}

