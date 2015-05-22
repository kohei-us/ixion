/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/global.hpp"
#include "ixion/mem_str_buf.hpp"
#include "ixion/address.hpp"
#include "ixion/matrix.hpp"
#include "ixion/cell.hpp"
#include "ixion/exceptions.hpp"
#include "ixion/formula_result.hpp"
#include "ixion/interface/formula_model_access.hpp"

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

#define IXION_DEBUG_GLOBAL 0

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

void global::sleep(unsigned int mseconds)
{
#ifdef _WIN32
    ::Sleep(mseconds);
#else
    ::usleep(1000*mseconds);
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
    static const char* default_err_name = "#ERR!";
    static const char* names[] = {
        "",        // no error
        "#REF!",   // result not available
        "#DIV/0!", // division by zero
        "#NUM!"    // invalid expression
    };
    static const size_t name_size = 4;
    if (static_cast<size_t>(fe) < name_size)
        return names[fe];

    return default_err_name;
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

namespace {

double get_numeric_value(const iface::formula_model_access& cxt, const stack_value& v)
{
    double ret = 0.0;
    switch (v.get_type())
    {
        case stack_value_t::value:
            ret = v.get_value();
        break;
        case stack_value_t::single_ref:
        {
            // reference to a single cell.
            const abs_address_t& addr = v.get_address();
            ret = cxt.get_numeric_value(addr);
        }
        break;
        default:
#if IXION_DEBUG_GLOBAL
            __IXION_DEBUG_OUT__ << "value is being popped, but the stack value type is not appropriate." << endl;
#endif
            throw formula_error(fe_stack_error);
    }
    return ret;
}

}

stack_value::stack_value(double val) :
    m_type(stack_value_t::value), m_value(val) {}

stack_value::stack_value(size_t sid) :
    m_type(stack_value_t::string), m_str_identifier(sid) {}

stack_value::stack_value(const abs_address_t& val) :
    m_type(stack_value_t::single_ref), m_address(new abs_address_t(val)) {}

stack_value::stack_value(const abs_range_t& val) :
    m_type(stack_value_t::range_ref), m_range(new abs_range_t(val)) {}

stack_value::~stack_value()
{
    switch (m_type)
    {
        case stack_value_t::range_ref:
            delete m_range;
            break;
        case stack_value_t::single_ref:
            delete m_address;
            break;
        case stack_value_t::string:
        case stack_value_t::value:
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
    if (m_type == stack_value_t::value)
        return m_value;

    return 0.0;
}

size_t stack_value::get_string() const
{
    if (m_type == stack_value_t::string)
        return m_str_identifier;
    return 0;
}

const abs_address_t& stack_value::get_address() const
{
    return *m_address;
}

const abs_range_t& stack_value::get_range() const
{
    return *m_range;
}

value_stack_t::value_stack_t(const iface::formula_model_access& cxt) : m_context(cxt) {}

value_stack_t::iterator value_stack_t::begin()
{
    return m_stack.begin();
}

value_stack_t::iterator value_stack_t::end()
{
    return m_stack.end();
}

value_stack_t::const_iterator value_stack_t::begin() const
{
    return m_stack.begin();
}

value_stack_t::const_iterator value_stack_t::end() const
{
    return m_stack.end();
}

value_stack_t::auto_type value_stack_t::release(iterator pos)
{
    return m_stack.release(pos);
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

void value_stack_t::swap(value_stack_t& other)
{
    m_stack.swap(other.m_stack);
}

const stack_value& value_stack_t::back() const
{
    return m_stack.back();
}

const stack_value& value_stack_t::operator[](size_t pos) const
{
    return m_stack[pos];
}

double value_stack_t::get_value(size_t pos) const
{
    const stack_value& v = m_stack[pos];
    return get_numeric_value(m_context, v);
}

void value_stack_t::push_back(auto_type val)
{
    m_stack.push_back(val.release());
}

void value_stack_t::push_value(double val)
{
    m_stack.push_back(new stack_value(val));
}

void value_stack_t::push_string(size_t sid)
{
    m_stack.push_back(new stack_value(sid));
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
    ret = get_numeric_value(m_context, v);
    m_stack.pop_back();
    return ret;
}

const string value_stack_t::pop_string()
{
    if (m_stack.empty())
        throw formula_error(fe_stack_error);

    const stack_value& v = m_stack.back();
    switch (v.get_type())
    {
        case stack_value_t::string:
        {
            const string* p = m_context.get_string(v.get_string());
            m_stack.pop_back();
            return p ? *p : string();
        }
        break;
        case stack_value_t::value:
        {
            ostringstream os;
            os << v.get_value();
            m_stack.pop_back();
            return os.str();
        }
        break;
        case stack_value_t::single_ref:
        {
            // reference to a single cell.
            abs_address_t addr = v.get_address();
            m_stack.pop_back();

            switch (m_context.get_celltype(addr))
            {
                case celltype_t::empty:
                    return string();
                case celltype_t::formula:
                {
                    const formula_cell* fc = m_context.get_formula_cell(addr);
                    const formula_result* res = fc->get_result_cache();
                    if (!res)
                        break;

                    switch (res->get_type())
                    {
                        case formula_result::rt_error:
                            throw formula_error(res->get_error());
                        case formula_result::rt_string:
                        {
                            const string* ps = m_context.get_string(res->get_string());
                            if (!ps)
                                throw formula_error(fe_stack_error);
                            return *ps;
                        }
                        break;
                        case formula_result::rt_value:
                        {
                            ostringstream os;
                            os << res->get_value();
                            return os.str();
                        }
                        default:
                            throw formula_error(fe_stack_error);
                    }
                }
                break;
                case celltype_t::numeric:
                {
                    ostringstream os;
                    os << m_context.get_numeric_value(addr);
                    return os.str();
                }
                case celltype_t::string:
                {
                    const string* ps = m_context.get_string(m_context.get_string_identifier(addr));
                    if (!ps)
                        throw formula_error(fe_stack_error);
                    return *ps;
                }
                break;
                default:
                    throw formula_error(fe_stack_error);
            }
        }
        break;
        default:
            ;
    }
    throw formula_error(fe_stack_error);
}

abs_address_t value_stack_t::pop_single_ref()
{
    if (m_stack.empty())
        throw formula_error(fe_stack_error);

    const stack_value& v = m_stack.back();
    if (v.get_type() != stack_value_t::single_ref)
        throw formula_error(fe_stack_error);

    abs_address_t addr = v.get_address();
    m_stack.pop_back();
    return addr;
}

abs_range_t value_stack_t::pop_range_ref()
{
    if (m_stack.empty())
        throw formula_error(fe_stack_error);

    const stack_value& v = m_stack.back();
    if (v.get_type() != stack_value_t::range_ref)
        throw formula_error(fe_stack_error);

    abs_range_t range = v.get_range();
    m_stack.pop_back();
    return range;
}

matrix value_stack_t::pop_range_value()
{
    if (m_stack.empty())
        throw formula_error(fe_stack_error);

    const stack_value& v = m_stack.back();
    if (v.get_type() != stack_value_t::range_ref)
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
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
