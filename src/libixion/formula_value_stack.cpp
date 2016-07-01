/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "formula_value_stack.hpp"

#include "ixion/address.hpp"
#include "ixion/cell.hpp"
#include "ixion/matrix.hpp"
#include "ixion/formula_result.hpp"
#include "ixion/interface/formula_model_access.hpp"

#include <string>

namespace ixion {

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
            throw formula_error(formula_error_t::stack_error);
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

value_stack_t::value_type value_stack_t::release(iterator pos)
{
    stack_value* p = pos->release();
    m_stack.erase(pos);
    return value_type(p);
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
    return *m_stack.back();
}

const stack_value& value_stack_t::operator[](size_t pos) const
{
    return *m_stack[pos];
}

double value_stack_t::get_value(size_t pos) const
{
    const stack_value& v = *m_stack[pos];
    return get_numeric_value(m_context, v);
}

void value_stack_t::push_back(value_type&& val)
{
    m_stack.push_back(std::move(val));
}

void value_stack_t::push_value(double val)
{
    m_stack.push_back(make_unique<stack_value>(val));
}

void value_stack_t::push_string(size_t sid)
{
    m_stack.push_back(make_unique<stack_value>(sid));
}

void value_stack_t::push_single_ref(const abs_address_t& val)
{
    m_stack.push_back(make_unique<stack_value>(val));
}

void value_stack_t::push_range_ref(const abs_range_t& val)
{
    m_stack.push_back(make_unique<stack_value>(val));
}

double value_stack_t::pop_value()
{
    double ret = 0.0;
    if (m_stack.empty())
        throw formula_error(formula_error_t::stack_error);

    const stack_value& v = *m_stack.back();
    ret = get_numeric_value(m_context, v);
    m_stack.pop_back();
    return ret;
}

const std::string value_stack_t::pop_string()
{
    if (m_stack.empty())
        throw formula_error(formula_error_t::stack_error);

    const stack_value& v = *m_stack.back();
    switch (v.get_type())
    {
        case stack_value_t::string:
        {
            const std::string* p = m_context.get_string(v.get_string());
            m_stack.pop_back();
            return p ? *p : std::string();
        }
        break;
        case stack_value_t::value:
        {
            std::ostringstream os;
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
                    return std::string();
                case celltype_t::formula:
                {
                    const formula_cell* fc = m_context.get_formula_cell(addr);
                    const formula_result* res = fc->get_result_cache();
                    if (!res)
                        break;

                    switch (res->get_type())
                    {
                        case formula_result::result_type::error:
                            throw formula_error(res->get_error());
                        case formula_result::result_type::string:
                        {
                            const std::string* ps = m_context.get_string(res->get_string());
                            if (!ps)
                                throw formula_error(formula_error_t::stack_error);
                            return *ps;
                        }
                        break;
                        case formula_result::result_type::value:
                        {
                            std::ostringstream os;
                            os << res->get_value();
                            return os.str();
                        }
                        default:
                            throw formula_error(formula_error_t::stack_error);
                    }
                }
                break;
                case celltype_t::numeric:
                {
                    std::ostringstream os;
                    os << m_context.get_numeric_value(addr);
                    return os.str();
                }
                case celltype_t::string:
                {
                    const std::string* ps = m_context.get_string(m_context.get_string_identifier(addr));
                    if (!ps)
                        throw formula_error(formula_error_t::stack_error);
                    return *ps;
                }
                break;
                default:
                    throw formula_error(formula_error_t::stack_error);
            }
        }
        break;
        default:
            ;
    }
    throw formula_error(formula_error_t::stack_error);
}

abs_address_t value_stack_t::pop_single_ref()
{
    if (m_stack.empty())
        throw formula_error(formula_error_t::stack_error);

    const stack_value& v = *m_stack.back();
    if (v.get_type() != stack_value_t::single_ref)
        throw formula_error(formula_error_t::stack_error);

    abs_address_t addr = v.get_address();
    m_stack.pop_back();
    return addr;
}

abs_range_t value_stack_t::pop_range_ref()
{
    if (m_stack.empty())
        throw formula_error(formula_error_t::stack_error);

    const stack_value& v = *m_stack.back();
    if (v.get_type() != stack_value_t::range_ref)
        throw formula_error(formula_error_t::stack_error);

    abs_range_t range = v.get_range();
    m_stack.pop_back();
    return range;
}

matrix value_stack_t::pop_range_value()
{
    if (m_stack.empty())
        throw formula_error(formula_error_t::stack_error);

    const stack_value& v = *m_stack.back();
    if (v.get_type() != stack_value_t::range_ref)
        throw formula_error(formula_error_t::stack_error);

    matrix ret = m_context.get_range_value(v.get_range());
    m_stack.pop_back();
    return ret;
}

stack_value_t value_stack_t::get_type() const
{
    if (m_stack.empty())
        throw formula_error(formula_error_t::stack_error);

    return m_stack.back()->get_type();
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
