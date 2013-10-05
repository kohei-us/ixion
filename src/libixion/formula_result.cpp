/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/formula_result.hpp"
#include "ixion/mem_str_buf.hpp"
#include "ixion/exceptions.hpp"
#include "ixion/interface/model_context.hpp"

#include <sstream>

#define DEBUG_FORMULA_RESULT 0

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
            m_str_identifier = r.m_str_identifier;
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

formula_result::formula_result(size_t strid) :
    m_type(rt_string), m_str_identifier(strid) {}

formula_result::formula_result(formula_error_t e) :
    m_type(rt_error), m_error(e) {}

formula_result::~formula_result() {}

void formula_result::reset()
{
    m_type = rt_value;
    m_value = 0.0;
}

void formula_result::set_value(double v)
{
    m_type = rt_value;
    m_value = v;
}

void formula_result::set_string(size_t strid)
{
    m_type = rt_string;
    m_str_identifier = strid;
}

void formula_result::set_error(formula_error_t e)
{
    m_type = rt_error;
    m_error = e;
}

double formula_result::get_value() const
{
    assert(m_type == rt_value);
    return m_value;
}

size_t formula_result::get_string() const
{
    assert(m_type == rt_string);
    return m_str_identifier;
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

string formula_result::str(const iface::model_context& cxt) const
{
    switch (m_type)
    {
        case rt_error:
            return string(get_formula_error_name(m_error));
        case rt_string:
        {
            const string* p = cxt.get_string(m_str_identifier);
            return p ? *p : string();
        }
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

void formula_result::parse(iface::model_context& cxt, const char* p, size_t n)
{
    if (!n)
        return;

    if (*p == '#')
        parse_error(p, n);
    else if (*p == '"')
        parse_string(cxt, p, n);
    else
    {
        // parse this as a number.
        m_value = global::to_double(p, n);
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
            m_str_identifier = r.m_str_identifier;
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
            return m_str_identifier == r.m_str_identifier;
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

void formula_result::parse_error(const char* p, size_t n)
{
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
    os << "malformed error string: " << string(p, n);
    throw general_error(os.str());
}

void formula_result::parse_string(iface::model_context& cxt, const char* p, size_t n)
{
    if (n <= 1)
        return;

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
    m_str_identifier = cxt.add_string(p_first, len);
}

}
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
