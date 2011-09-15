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

#include "ixion/formula_result.hpp"
#include "ixion/mem_str_buf.hpp"

#include <sstream>

#define DEBUG_FORMULA_RESULT 1

#if DEBUG_FORMULA_RESULT
#include <iostream>
#endif

using namespace std;

namespace ixion {

formula_result::formula_result() :
    m_type(rt_value), m_value(0.0) {}

formula_result::formula_result(const formula_result& r) :
    m_type(r.m_type)
{
    switch (m_type)
    {
        case rt_value:
            m_value = r.m_value;
        break;
        case rt_string:
            m_string = new string(*r.m_string);
        break;
        case rt_error:
            m_error = r.m_error;
        break;
        default:
            assert(!"unknown formula result type specified during copy construction.");
    }
}

formula_result::formula_result(double v) :
    m_type(rt_value), m_value(v) {}

formula_result::formula_result(string* p) :
    m_type(rt_string), m_string(p) {}

formula_result::formula_result(formula_error_t e) :
    m_type(rt_error), m_error(e) {}

formula_result::~formula_result()
{
    if (m_type == rt_string)
        delete m_string;
}

void formula_result::set_value(double v)
{
    if (m_type == rt_string)
        delete m_string;

    m_type = rt_value;
    m_value = v;
}

void formula_result::set_string(string* p)
{
    if (m_type == rt_string)
        delete m_string;

    m_type = rt_string;
    m_string = p;
}

void formula_result::set_error(formula_error_t e)
{
    if (m_type == rt_string)
        delete m_string;

    m_type = rt_error;
    m_error = e;
}

double formula_result::get_value() const
{
    assert(m_type == rt_value);
    return m_value;
}

const string& formula_result::get_string() const
{
    assert(m_type == rt_string);
    return *m_string;
}

formula_error_t formula_result::get_error() const
{
    assert(m_type == rt_error);
    return m_error;
}

formula_result::result_type formula_result::get_type() const
{
    return m_type;
}

string formula_result::str() const
{
    switch (m_type)
    {
        case rt_error:
            return string(get_formula_error_name(m_error));
        case rt_string:
            return *m_string;
        case rt_value:
        {
            ostringstream os;
            os << m_value;
            return os.str();
        }
        default:
            assert(!"unknown formula result type!");
    }
    return string();
}

void formula_result::parse(const string& r)
{
    if (r.empty())
        return;

    const char* p = r.c_str();
    if (*p == '#')
        parse_error(r);
    else if (*p == '"')
        parse_string(r);
    else
    {
        // parse this as a number.
        m_value = strtod(p, NULL);
        m_type = rt_value;
    }

}

formula_result& formula_result::operator= (const formula_result& r)
{
    m_type = r.m_type;
    switch (m_type)
    {
        case rt_value:
            m_value = r.m_value;
        break;
        case rt_string:
            m_string = new string(*r.m_string);
        break;
        case rt_error:
            m_error = r.m_error;
        break;
        default:
            assert(!"unknown formula result type specified during copy construction.");
    }
    return *this;
}

bool formula_result::operator== (const formula_result& r) const
{
    if (m_type != r.m_type)
        return false;

    switch (m_type)
    {
        case rt_value:
            return m_value == r.m_value;
        break;
        case rt_string:
            return *m_string == *r.m_string;
        break;
        case rt_error:
            return m_error == r.m_error;
        break;
        default:
            assert(!"unknown formula result type specified during copy construction.");
    }

    assert(!"this should never be reached!");
    return false;
}

bool formula_result::operator!= (const formula_result& r) const
{
    return !operator== (r);
}

void formula_result::parse_error(const string& str)
{
    const char* p = &str[0];
    size_t n = str.size();

    assert(n);
    assert(*p == '#');

    ++p; // skip '#'.
    mem_str_buf buf;
    for (size_t i = 0; i < n; ++p, ++i)
    {
        if (*p == '!')
        {
            if (buf.empty())
                throw general_error("failed to parse error string: buffer is empty.");

            if (buf.equals("REF"))
            {
                m_error = fe_ref_result_not_available;
            }
            else if (buf.equals("DIV/0"))
            {
                m_error = fe_division_by_zero;
            }
            else
                throw general_error("failed to parse error string in formula_result::parse_error().");

            m_type = rt_error;
            return;
        }

        if (buf.empty())
            buf.set_start(p);
        else
            buf.inc();
    }

    ostringstream os;
    os << "malformed error string: " << str;
    throw general_error(os.str());
}

void formula_result::parse_string(const string& str)
{
    const char* p = &str[0];
    size_t n = str.size();
    assert(*p == '"');
    ++p;
    const char* p_first = p;
    size_t len = 0;
    for (size_t i = 1; i < n; ++i, ++len, ++p)
    {
        char c = *p;
        if (c == '"')
            break;
    }

    if (!len)
        throw general_error("failed to parse string result.");

    m_type = rt_string;
    m_string = new string(p_first, len);
}

}
