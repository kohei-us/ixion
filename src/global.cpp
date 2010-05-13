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

#include <sstream>

using namespace std;

namespace ixion {

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
    switch (m_ferror)
    {
        case fe_ref_result_not_available:
            return "#REF!";
        case fe_division_by_zero:
            return "#DIV/0!";
        default:
            ;
    }
    return "#ERR!";
}

formula_error_t formula_error::get_error() const
{
    return m_ferror;
}

// ============================================================================

formula_result::formula_result() :
    m_numeric(true), m_value(0.0) {}

formula_result::formula_result(double v) :
    m_numeric(true), m_value(v) {}

formula_result::formula_result(string* p) :
    m_numeric(false), m_string(p) {}

formula_result::~formula_result()
{
    if (!m_numeric)
        delete m_string;
}

void formula_result::set_value(double v)
{
    if (!m_numeric)
        delete m_string;

    m_numeric = true;
    m_value = v;
}

void formula_result::set_string(string* p)
{
    if (!m_numeric)
        delete m_string;

    m_string = p;
}

double formula_result::get_value() const
{
    return m_value;
}

const string& formula_result::get_string() const
{
    return *m_string;
}

bool formula_result::numeric() const
{
    return m_numeric;
}

}
