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
#include "ixion/config.hpp"

#include <string>
#include <sstream>
#include <spdlog/spdlog.h>

namespace ixion {

namespace {

double get_numeric_value(const iface::formula_model_access& cxt, const stack_value& v)
{
    double ret = 0.0;
    switch (v.get_type())
    {
        case stack_value_t::value:
        case stack_value_t::matrix:
            ret = v.get_value();
            break;
        case stack_value_t::single_ref:
        {
            // reference to a single cell.
            const abs_address_t& addr = v.get_address();
            ret = cxt.get_numeric_value(addr);
            break;
        }
        default:
            SPDLOG_DEBUG(spdlog::get("ixion"), "value is being popped, but the stack value type is not appropriate.");
            throw formula_error(formula_error_t::stack_error);
    }
    return ret;
}

}

stack_value::stack_value(double val) :
    m_type(stack_value_t::value), m_value(val) {}

stack_value::stack_value(std::string str) :
    m_type(stack_value_t::string), m_str(new std::string(std::move(str))) {}

stack_value::stack_value(const abs_address_t& val) :
    m_type(stack_value_t::single_ref), m_address(new abs_address_t(val)) {}

stack_value::stack_value(const abs_range_t& val) :
    m_type(stack_value_t::range_ref), m_range(new abs_range_t(val)) {}

stack_value::stack_value(matrix mtx) :
    m_type(stack_value_t::matrix), m_matrix(new matrix(std::move(mtx))) {}

stack_value::stack_value(stack_value&& other) :
    m_type(other.m_type)
{
    other.m_type = stack_value_t::value;

    switch (m_type)
    {
        case stack_value_t::matrix:
            m_matrix = other.m_matrix;
            other.m_matrix = nullptr;
            break;
        case stack_value_t::range_ref:
            m_range = other.m_range;
            other.m_range = nullptr;
            break;
        case stack_value_t::single_ref:
            m_address = other.m_address;
            other.m_address = nullptr;
            break;
        case stack_value_t::string:
            m_str = other.m_str;
            other.m_str = nullptr;
            break;
        case stack_value_t::value:
            m_value = other.m_value;
            break;
        default:
            ;
    }
}

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
        case stack_value_t::matrix:
            delete m_matrix;
            break;
        case stack_value_t::string:
            delete m_str;
            break;
        case stack_value_t::value:
        default:
            ; // do nothing
    }
}

stack_value& stack_value::operator= (stack_value&& other)
{
    other.m_type = stack_value_t::value;

    switch (m_type)
    {
        case stack_value_t::matrix:
            m_matrix = other.m_matrix;
            other.m_matrix = nullptr;
            break;
        case stack_value_t::range_ref:
            m_range = other.m_range;
            other.m_range = nullptr;
            break;
        case stack_value_t::single_ref:
            m_address = other.m_address;
            other.m_address = nullptr;
            break;
        case stack_value_t::string:
            m_str = other.m_str;
            other.m_str = nullptr;
            break;
        case stack_value_t::value:
            m_value = other.m_value;
            break;
        default:
            ;
    }

    return *this;
}

stack_value_t stack_value::get_type() const
{
    return m_type;
}

double stack_value::get_value() const
{
    switch (m_type)
    {
        case stack_value_t::value:
            return m_value;
        case stack_value_t::matrix:
            return m_matrix->get_numeric(0, 0);
        default:
            ;
    }

    return 0.0;
}

const std::string& stack_value::get_string() const
{
    return *m_str;
}

const abs_address_t& stack_value::get_address() const
{
    return *m_address;
}

const abs_range_t& stack_value::get_range() const
{
    return *m_range;
}

matrix stack_value::pop_matrix()
{
    switch (m_type)
    {
        case stack_value_t::value:
        {
            matrix mtx(1, 1);
            mtx.set(0, 0, m_value);
            return mtx;
        }
        case stack_value_t::matrix:
        {
            matrix mtx;
            mtx.swap(*m_matrix);
            return mtx;
        }
        default:
            throw formula_error(formula_error_t::stack_error);
    }
}

formula_value_stack::formula_value_stack(const iface::formula_model_access& cxt) : m_context(cxt) {}

formula_value_stack::iterator formula_value_stack::begin()
{
    return m_stack.begin();
}

formula_value_stack::iterator formula_value_stack::end()
{
    return m_stack.end();
}

formula_value_stack::const_iterator formula_value_stack::begin() const
{
    return m_stack.begin();
}

formula_value_stack::const_iterator formula_value_stack::end() const
{
    return m_stack.end();
}

formula_value_stack::value_type formula_value_stack::release(iterator pos)
{
    auto tmp = std::move(*pos);
    m_stack.erase(pos);
    return tmp;
}

formula_value_stack::value_type formula_value_stack::release_back()
{
    assert(!m_stack.empty());
    auto tmp = std::move(m_stack.back());
    m_stack.pop_back();
    return tmp;
}

bool formula_value_stack::empty() const
{
    return m_stack.empty();
}

size_t formula_value_stack::size() const
{
    return m_stack.size();
}

void formula_value_stack::clear()
{
    return m_stack.clear();
}

void formula_value_stack::swap(formula_value_stack& other)
{
    m_stack.swap(other.m_stack);
}

stack_value& formula_value_stack::back()
{
    return m_stack.back();
}

const stack_value& formula_value_stack::back() const
{
    return m_stack.back();
}

const stack_value& formula_value_stack::operator[](size_t pos) const
{
    return m_stack[pos];
}

double formula_value_stack::get_value(size_t pos) const
{
    const stack_value& v = m_stack[pos];
    return get_numeric_value(m_context, v);
}

void formula_value_stack::push_back(value_type&& val)
{
    SPDLOG_TRACE(spdlog::get("ixion"), "push_back");
    m_stack.push_back(std::move(val));
}

void formula_value_stack::push_value(double val)
{
    SPDLOG_TRACE(spdlog::get("ixion"), "push_value: val={}", val);
    m_stack.emplace_back(val);
}

void formula_value_stack::push_string(std::string str)
{
    SPDLOG_TRACE(spdlog::get("ixion"), "push_string: sid={}", sid);
    m_stack.emplace_back(std::move(str));
}

void formula_value_stack::push_single_ref(const abs_address_t& val)
{
    SPDLOG_TRACE(spdlog::get("ixion"), "push_single_ref: val={}", val.get_name());
    m_stack.emplace_back(val);
}

void formula_value_stack::push_range_ref(const abs_range_t& val)
{
    assert(val.valid());
    SPDLOG_TRACE(spdlog::get("ixion"), "push_range_ref: start={}; end={}", val.first.get_name(), val.last.get_name());
    m_stack.emplace_back(val);
}

void formula_value_stack::push_matrix(matrix mtx)
{
    SPDLOG_TRACE(spdlog::get("ixion"), "push_matrix");
    m_stack.emplace_back(std::move(mtx));
}

double formula_value_stack::pop_value()
{
    double ret = 0.0;
    if (m_stack.empty())
        throw formula_error(formula_error_t::stack_error);

    const stack_value& v = m_stack.back();
    ret = get_numeric_value(m_context, v);
    m_stack.pop_back();
    SPDLOG_TRACE(spdlog::get("ixion"), "pop_value: ret={}", ret);
    return ret;
}

const std::string formula_value_stack::pop_string()
{
    SPDLOG_TRACE(spdlog::get("ixion"), "pop_string");

    if (m_stack.empty())
        throw formula_error(formula_error_t::stack_error);

    const stack_value& v = m_stack.back();
    switch (v.get_type())
    {
        case stack_value_t::string:
        {
            const std::string str = v.get_string();
            m_stack.pop_back();
            return str;
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
                    formula_result res = m_context.get_formula_result(addr);

                    switch (res.get_type())
                    {
                        case formula_result::result_type::error:
                            throw formula_error(res.get_error());
                        case formula_result::result_type::string:
                            return res.get_string_value();
                        case formula_result::result_type::value:
                        {
                            std::ostringstream os;
                            os << res.get_value();
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

abs_address_t formula_value_stack::pop_single_ref()
{
    SPDLOG_TRACE(spdlog::get("ixion"), "pop_single_ref");
    if (m_stack.empty())
        throw formula_error(formula_error_t::stack_error);

    const stack_value& v = m_stack.back();
    if (v.get_type() != stack_value_t::single_ref)
        throw formula_error(formula_error_t::stack_error);

    abs_address_t addr = v.get_address();
    m_stack.pop_back();
    return addr;
}

abs_range_t formula_value_stack::pop_range_ref()
{
    SPDLOG_TRACE(spdlog::get("ixion"), "pop_range_ref");

    if (m_stack.empty())
        throw formula_error(formula_error_t::stack_error);

    const stack_value& v = m_stack.back();
    if (v.get_type() != stack_value_t::range_ref)
        throw formula_error(formula_error_t::stack_error);

    abs_range_t range = v.get_range();
    m_stack.pop_back();
    return range;
}

matrix formula_value_stack::pop_range_value()
{
    SPDLOG_TRACE(spdlog::get("ixion"), "pop_range_value");

    if (m_stack.empty())
        throw formula_error(formula_error_t::stack_error);

    const stack_value& v = m_stack.back();
    if (v.get_type() != stack_value_t::range_ref)
        throw formula_error(formula_error_t::stack_error);

    matrix ret = m_context.get_range_value(v.get_range());
    m_stack.pop_back();
    return ret;
}

stack_value_t formula_value_stack::get_type() const
{
    if (m_stack.empty())
        throw formula_error(formula_error_t::stack_error);

    return m_stack.back().get_type();
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
