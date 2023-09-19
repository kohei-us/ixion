/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "formula_value_stack.hpp"
#include "debug.hpp"

#include <ixion/address.hpp>
#include <ixion/cell.hpp>
#include <ixion/cell_access.hpp>
#include <ixion/matrix.hpp>
#include <ixion/formula_result.hpp>
#include <ixion/config.hpp>
#include <ixion/exceptions.hpp>

#include <string>
#include <sstream>

namespace ixion {

namespace {

bool get_boolean_value(const model_context& cxt, const stack_value& v)
{
    switch (v.get_type())
    {
        case stack_value_t::boolean:
            return v.get_boolean();
        case stack_value_t::value:
        case stack_value_t::matrix:
            return v.get_value() != 0.0;
        case stack_value_t::single_ref:
        {
            // reference to a single cell.
            const abs_address_t& addr = v.get_address();
            auto ca = cxt.get_cell_access(addr);
            switch (ca.get_value_type())
            {
                case cell_value_t::numeric:
                case cell_value_t::boolean:
                    return ca.get_boolean_value();
                case cell_value_t::empty:
                    return false;
                default:;
            }
            break;
        }
        default:;
    }

    throw formula_error(formula_error_t::invalid_value_type);
}

double get_numeric_value(const model_context& cxt, const stack_value& v)
{
    switch (v.get_type())
    {
        case stack_value_t::boolean:
            return v.get_boolean() ? 1.0 : 0.0;
        case stack_value_t::value:
        case stack_value_t::matrix:
            return v.get_value();
        case stack_value_t::string:
            return 0.0;
        case stack_value_t::single_ref:
        {
            // reference to a single cell.
            const abs_address_t& addr = v.get_address();
            return cxt.get_numeric_value(addr);
        }
        default:;
    }

    throw formula_error(formula_error_t::invalid_value_type);
}

} // anonymous namespace

std::ostream& operator<<(std::ostream& os, stack_value_t sv)
{
    static constexpr std::string_view names[] = {
        "boolean",
        "error",
        "value",
        "string",
        "single_ref",
        "range_ref",
        "matrix",
    };

    auto pos = static_cast<std::size_t>(sv);
    if (pos < std::size(names))
        os << names[pos];
    else
        os << "???";

    return os;
}

stack_value::stack_value(bool b) :
    m_type(stack_value_t::boolean), m_value(b) {}

stack_value::stack_value(double val) :
    m_type(stack_value_t::value), m_value(val) {}

stack_value::stack_value(std::string str) :
    m_type(stack_value_t::string), m_value(std::move(str)) {}

stack_value::stack_value(const abs_address_t& val) :
    m_type(stack_value_t::single_ref), m_value(val) {}

stack_value::stack_value(const abs_range_t& val) :
    m_type(stack_value_t::range_ref), m_value(val) {}

stack_value::stack_value(formula_error_t err) :
    m_type(stack_value_t::error), m_value(err) {}

stack_value::stack_value(matrix mtx) :
    m_type(stack_value_t::matrix), m_value(std::move(mtx)) {}

stack_value::stack_value(stack_value&& other) :
    m_type(other.m_type), m_value(std::move(other.m_value)) {}

stack_value::~stack_value() = default;

stack_value& stack_value::operator= (stack_value&& other)
{
    m_type = other.m_type;
    m_value = std::move(other.m_value);
    return *this;
}

stack_value_t stack_value::get_type() const
{
    return m_type;
}

bool stack_value::get_boolean() const
{
    switch (m_type)
    {
        case stack_value_t::boolean:
            return std::get<bool>(m_value);
        case stack_value_t::value:
            return std::get<double>(m_value) != 0.0;
        case stack_value_t::matrix:
            return std::get<matrix>(m_value).get_boolean(0, 0);
        default:;
    }

    return false;
}

double stack_value::get_value() const
{
    switch (m_type)
    {
        case stack_value_t::boolean:
            return std::get<bool>(m_value) ? 1.0 : 0.0;
        case stack_value_t::value:
            return std::get<double>(m_value);
        case stack_value_t::matrix:
            return std::get<matrix>(m_value).get_numeric(0, 0);
        default:
            ;
    }

    return 0.0;
}

const std::string& stack_value::get_string() const
{
    return std::get<std::string>(m_value);
}

const abs_address_t& stack_value::get_address() const
{
    return std::get<abs_address_t>(m_value);
}

const abs_range_t& stack_value::get_range() const
{
    return std::get<abs_range_t>(m_value);
}

formula_error_t stack_value::get_error() const
{
    return std::get<formula_error_t>(m_value);
}

const matrix& stack_value::get_matrix() const
{
    return std::get<matrix>(m_value);
}

matrix stack_value::pop_matrix()
{
    switch (m_type)
    {
        case stack_value_t::boolean:
        {
            matrix mtx(1, 1);
            mtx.set(0, 0, std::get<bool>(m_value));
            return mtx;
        }
        case stack_value_t::value:
        {
            matrix mtx(1, 1);
            mtx.set(0, 0, std::get<double>(m_value));
            return mtx;
        }
        case stack_value_t::matrix:
        {
            matrix mtx;
            mtx.swap(std::get<matrix>(m_value));
            return mtx;
        }
        default:
            throw formula_error(formula_error_t::stack_error);
    }
}

formula_value_stack::formula_value_stack(const model_context& cxt) : m_context(cxt) {}

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
    IXION_TRACE("push_back");
    m_stack.push_back(std::move(val));
}

void formula_value_stack::push_boolean(bool b)
{
    IXION_TRACE("b=" << std::boolalpha << b);
    m_stack.emplace_back(b);
}

void formula_value_stack::push_value(double val)
{
    IXION_TRACE("val=" << val);
    m_stack.emplace_back(val);
}

void formula_value_stack::push_string(std::string str)
{
    IXION_TRACE("str='" << str << "'");
    m_stack.emplace_back(std::move(str));
}

void formula_value_stack::push_single_ref(const abs_address_t& val)
{
    IXION_TRACE("val=" << val.get_name());
    m_stack.emplace_back(val);
}

void formula_value_stack::push_range_ref(const abs_range_t& val)
{
    assert(val.valid());
    IXION_TRACE("start=" << val.first.get_name() << "; end=" << val.last.get_name());
    m_stack.emplace_back(val);
}

void formula_value_stack::push_matrix(matrix mtx)
{
    IXION_TRACE("push_matrix");
    m_stack.emplace_back(std::move(mtx));
}

void formula_value_stack::push_error(formula_error_t err)
{
    IXION_TRACE("err=" << short(err) << " (" << get_formula_error_name(err) << ")");
    m_stack.emplace_back(err);
}

bool formula_value_stack::pop_boolean()
{
    if (m_stack.empty())
        throw formula_error(formula_error_t::stack_error);

    const stack_value& v = m_stack.back();
    bool ret = get_boolean_value(m_context, v);
    m_stack.pop_back();
    IXION_TRACE("ret=" << std::boolalpha << ret);
    return ret;
}

double formula_value_stack::pop_value()
{
    double ret = 0.0;
    if (m_stack.empty())
        throw formula_error(formula_error_t::stack_error);

    const stack_value& v = m_stack.back();
    ret = get_numeric_value(m_context, v);
    m_stack.pop_back();
    IXION_TRACE("ret=" << ret);
    return ret;
}

std::string formula_value_stack::pop_string()
{
    IXION_TRACE("pop_string");

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
        case stack_value_t::boolean:
        {
            std::ostringstream os;
            os << std::boolalpha << v.get_boolean();
            m_stack.pop_back();
            return os.str();
        }
        case stack_value_t::value:
        {
            std::ostringstream os;
            os << v.get_value();
            m_stack.pop_back();
            return os.str();
        }
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
                            return res.get_string();
                        case formula_result::result_type::boolean:
                        {
                            std::ostringstream os;
                            os << std::boolalpha << res.get_boolean();
                            return os.str();
                        }
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

            break;
        }
        default:
            ;
    }
    throw formula_error(formula_error_t::stack_error);
}

matrix formula_value_stack::pop_matrix()
{
    if (m_stack.empty())
        throw formula_error(formula_error_t::stack_error);

    stack_value& v = m_stack.back();
    switch (v.get_type())
    {
        case stack_value_t::matrix:
        {
            auto mtx = v.pop_matrix();
            m_stack.pop_back();
            return mtx;
        }
        default:;
    }

    throw formula_error(formula_error_t::stack_error);
}

abs_address_t formula_value_stack::pop_single_ref()
{
    IXION_TRACE("pop_single_ref");

    if (m_stack.empty())
        throw formula_error(formula_error_t::stack_error);

    const stack_value& v = m_stack.back();

    switch (v.get_type())
    {
        case stack_value_t::single_ref:
        {
            abs_address_t addr = v.get_address();
            m_stack.pop_back();
            return addr;
        }
        case stack_value_t::range_ref:
        {
            abs_range_t range = v.get_range();
            m_stack.pop_back();
            return range.first;
        }
        default:;
    }

    throw formula_error(formula_error_t::stack_error);
}

abs_range_t formula_value_stack::pop_range_ref()
{
    IXION_TRACE("pop_range_ref");

    if (m_stack.empty())
        throw formula_error(formula_error_t::stack_error);

    const stack_value& v = m_stack.back();

    switch (v.get_type())
    {
        case stack_value_t::single_ref:
        {
            abs_address_t addr = v.get_address();
            m_stack.pop_back();
            return addr;
        }
        case stack_value_t::range_ref:
        {
            abs_range_t range = v.get_range();
            m_stack.pop_back();
            return range;
        }
        default:
            throw formula_error(formula_error_t::stack_error);
    }
}

matrix formula_value_stack::pop_range_value()
{
    IXION_TRACE("pop_range_value");

    if (m_stack.empty())
        throw formula_error(formula_error_t::stack_error);

    const stack_value& v = m_stack.back();
    if (v.get_type() != stack_value_t::range_ref)
        throw formula_error(formula_error_t::stack_error);

    matrix ret = m_context.get_range_value(v.get_range());
    m_stack.pop_back();
    return ret;
}

formula_error_t formula_value_stack::pop_error()
{
    IXION_TRACE("pop_error");

    if (m_stack.empty())
        throw formula_error(formula_error_t::stack_error);

    const stack_value& v = m_stack.back();
    if (v.get_type() != stack_value_t::error)
        throw formula_error(formula_error_t::stack_error);

    formula_error_t ret = v.get_error();
    m_stack.pop_back();
    return ret;
}

matrix_or_value_t formula_value_stack::pop_matrix_or_value()
{
    if (m_stack.empty())
        throw formula_error(formula_error_t::stack_error);

    stack_value& v = m_stack.back();
    switch (v.get_type())
    {
        case stack_value_t::matrix:
        {
            auto mtx = v.pop_matrix();
            m_stack.pop_back();
            return mtx;
        }
        case stack_value_t::range_ref:
            return pop_range_value();
        default:;
    }

    // fall back to numeric value
    return pop_value();
}

void formula_value_stack::pop_back()
{
    m_stack.pop_back();
}

stack_value_t formula_value_stack::get_type() const
{
    if (m_stack.empty())
        throw formula_error(formula_error_t::stack_error);

    return m_stack.back().get_type();
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
