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

#include "ixion/global.hpp"
#include "ixion/mem_str_buf.hpp"
#include "ixion/address.hpp"
#include "ixion/matrix.hpp"
#include "ixion/cell.hpp"
#include "ixion/exceptions.hpp"
#include "ixion/interface/model_context.hpp"

#include <iostream>
#include <cstdlib>
#include <sstream>
#include <fstream>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/time.h>
#endif

#include <boost/thread/mutex.hpp>

using namespace std;

namespace ixion {

const char* get_formula_result_output_separator()
{
    static const char* sep =
        "---------------------------------------------------------";
    return sep;
}

double global::get_current_time()
{
#ifdef _WIN32
    FILETIME ft;
    __int64 *time64 = (__int64 *) &ft;
    GetSystemTimeAsFileTime (&ft);
    return *time64 / 10000000.0;
#else
    timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
#endif
}

void global::sleep(unsigned int seconds)
{
#ifdef _WIN32
    ::Sleep(1000*seconds);
#else
    ::sleep(seconds);
#endif
}

void global::load_file_content(const string& filepath, string& content)
{
    ifstream file(filepath.c_str());
    if (!file)
        // failed to open the specified file.
        throw file_not_found(filepath);

    ostringstream os;
    os << file.rdbuf() << ' '; // extra char as the end position.
    file.close();

    os.str().swap(content);
}

double global::to_double(const char* p, size_t n)
{
    if (!n)
        return 0.0;

    // First, use the standard C API.
    const char* p_last_check = p + n;
    char* p_last;
    double val = strtod(p, &p_last);
    if (p_last == p_last_check)
        return val;

    // If that fails, do the manual conversion, which may introduce rounding
    // errors.  Revise this to reduce the amount of rounding error.
    bool dot = false;
    double frac = 1.0;
    double sign = 1.0;
    for (size_t i = 0; i < n; ++i, ++p)
    {
        if (i == 0)
        {
            if (*p == '+')
                // Skip this.
                continue;

            if (*p == '-')
            {
                sign = -1.0;
                continue;
            }
        }
        if (*p == '.')
        {
            if (dot)
                // Second dot is not allowed.
                break;
            dot = true;
            continue;
        }

        if (*p < '0' || '9' < *p)
            // not a digit.  End the parse.
            break;

        int digit = *p - '0';
        if (dot)
        {
            frac *= 0.1;
            val += digit * frac;
        }
        else
        {
            val *= 10.0;
            val += digit;
        }
    }
    return sign*val;
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

stack_value::stack_value(double val) :
    m_type(sv_value), m_value(val) {}

stack_value::stack_value(const abs_address_t& val) :
    m_type(sv_single_ref), m_address(new abs_address_t(val)) {}

stack_value::stack_value(const abs_range_t& val) :
    m_type(sv_range_ref), m_range(new abs_range_t(val)) {}

stack_value::~stack_value()
{
    switch (m_type)
    {
        case sv_range_ref:
            delete m_range;
            break;
        case sv_single_ref:
            delete m_address;
            break;
        case sv_string:
            delete m_str;
            break;
        case sv_value:
        default:
            ; // do nothing
    }
}

stack_value_t stack_value::get_type() const
{
    return m_type;
}

double stack_value::get_value() const
{
    if (m_type == sv_value)
        return m_value;

    return 0.0;
}

const string* stack_value::get_string() const
{
    if (m_type == sv_string)
        return m_str;
    return NULL;
}

const abs_address_t& stack_value::get_address() const
{
    return *m_address;
}

const abs_range_t& stack_value::get_range() const
{
    return *m_range;
}

value_stack_t::value_stack_t(const interface::model_context& cxt) : m_context(cxt) {}

value_stack_t::const_iterator value_stack_t::begin() const
{
    return m_stack.begin();
}

value_stack_t::const_iterator value_stack_t::end() const
{
    return m_stack.end();
}

bool value_stack_t::empty() const
{
    return m_stack.empty();
}

size_t value_stack_t::size() const
{
    return m_stack.size();
}

void value_stack_t::clear()
{
    return m_stack.clear();
}

const stack_value& value_stack_t::back() const
{
    return m_stack.back();
}

void value_stack_t::push_value(double val)
{
    m_stack.push_back(new stack_value(val));
}

void value_stack_t::push_single_ref(const abs_address_t& val)
{
    m_stack.push_back(new stack_value(val));
}

void value_stack_t::push_range_ref(const abs_range_t& val)
{
    m_stack.push_back(new stack_value(val));
}

double value_stack_t::pop_value()
{
    double ret = 0.0;
    if (m_stack.empty())
        throw formula_error(fe_stack_error);

    const stack_value& v = m_stack.back();
    switch (v.get_type())
    {
        case sv_value:
            ret = v.get_value();
        break;
        case sv_single_ref:
        {
            // reference to a single cell.
            const abs_address_t& addr = v.get_address();
            const base_cell* p = m_context.get_cell(addr);
            if (p)
                ret = p->get_value();
            else
                // empty cell has a value of 0.
                ret = 0.0;
        }
        break;
        default:
            throw formula_error(fe_stack_error);
    }

    m_stack.pop_back();
    return ret;
}

matrix value_stack_t::pop_range_value()
{
    if (m_stack.empty())
        throw formula_error(fe_stack_error);

    const stack_value& v = m_stack.back();
    if (v.get_type() != sv_range_ref)
        throw formula_error(fe_stack_error);

    matrix ret = m_context.get_range_value(v.get_range());
    m_stack.pop_back();
    return ret;
}

stack_value_t value_stack_t::get_type() const
{
    if (m_stack.empty())
        throw formula_error(fe_stack_error);

    return m_stack.back().get_type();
}

}
