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

#include "global.hpp"
#include "mem_str_buf.hpp"

#include <iostream>
#include <cstdlib>
#include <sstream>
#include <sys/time.h>

using namespace std;

namespace ixion {

const char* get_formula_result_output_separator()
{
    static const char* sep = 
        "---------------------------------------------------------";
    return sep;
}

string get_cell_name(const cell_ptr_name_map_t& names, const base_cell* cell)
{
    cell_ptr_name_map_t::const_iterator itr = names.find(cell);
    return itr == names.end() ? string() : itr->second;
}

double get_current_time()
{
    timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

const char* get_formula_error_name(formula_error_t fe)
{
    static const char* names[] = {
        "",        // no error
        "#REF!",   // result not available
        "#DIV/0!", // division by zero
        "#NUM!"    // invalid expression
    };
    static const size_t name_size = 4;
    if (static_cast<size_t>(fe) < name_size)
        return names[fe];

    return "#ERR!";
}

// ============================================================================

general_error::general_error(const string& msg) :
    m_msg(msg)
{
}

general_error::~general_error() throw()
{
}

const char* general_error::what() const throw()
{
    return m_msg.c_str();
}

// ============================================================================

formula_error::formula_error(formula_error_t fe) :
    m_ferror(fe)
{
}

formula_error::~formula_error() throw()
{
}

const char* formula_error::what() const throw()
{
    return get_formula_error_name(m_ferror);
}

formula_error_t formula_error::get_error() const
{
    return m_ferror;
}

}
