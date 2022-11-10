/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <ixion/formula_result.hpp>
#include <ixion/exceptions.hpp>
#include <ixion/config.hpp>
#include <ixion/matrix.hpp>
#include <ixion/model_context.hpp>

#include <cassert>
#include <sstream>
#include <iomanip>
#include <ostream>
#include <variant>

#include "debug.hpp"

namespace ixion {

struct formula_result::impl
{
    using result_value_type = std::variant<bool, double, formula_error_t, matrix, std::string>;

    result_type type;
    result_value_type value;

    impl() : type(result_type::value), value(0.0) {}
    impl(bool b) : type(result_type::boolean), value(b) {}
    impl(double v) : type(result_type::value), value(v) {}
    impl(std::string str) : type(result_type::string), value(std::move(str)) {}
    impl(formula_error_t e) : type(result_type::error), value(e) {}
    impl(matrix mtx) : type(result_type::matrix), value(std::move(mtx)) {}
    impl(const impl& other) : type(other.type), value(other.value) {}

    void reset()
    {
        type = result_type::value;
        value = 0.0;
    }

    void set_boolean(bool b)
    {
        type = result_type::boolean;
        value = b;
    }

    void set_value(double v)
    {
        IXION_TRACE("v=" << v);
        type = result_type::value;
        value = v;
    }

    void set_string_value(std::string str)
    {
        type = result_type::string;
        value = std::move(str);
    }

    void set_error(formula_error_t e)
    {
        type = result_type::error;
        value = e;
    }

    void set_matrix(matrix mtx)
    {
        type = result_type::matrix;
        value = std::move(mtx);
    }

    bool get_boolean() const
    {
        assert(type == result_type::boolean);
        return std::get<bool>(value);
    }

    double get_value() const
    {
        assert(type == result_type::value);
        return std::get<double>(value);
    }

    const std::string& get_string_value() const
    {
        assert(type == result_type::string);
        return std::get<std::string>(value);
    }

    formula_error_t get_error() const
    {
        assert(type == result_type::error);
        return std::get<formula_error_t>(value);
    }

    const matrix& get_matrix() const
    {
        assert(type == result_type::matrix);
        return std::get<matrix>(value);
    }

    matrix& get_matrix()
    {
        assert(type == result_type::matrix);
        return std::get<matrix>(value);
    }

    result_type get_type() const
    {
        return type;
    }

    std::string str(const model_context& cxt) const
    {
        switch (type)
        {
            case result_type::error:
            {
                std::string_view s = get_formula_error_name(std::get<formula_error_t>(value));
                return std::string(s);
            }
            case result_type::string:
                return std::get<std::string>(value);
            case result_type::boolean:
            {
                std::ostringstream os;
                os << std::boolalpha << std::get<bool>(value);
                return os.str();
            }
            case result_type::value:
            {
                std::ostringstream os;
                if (cxt.get_config().output_precision >= 0)
                    os << std::fixed << std::setprecision(cxt.get_config().output_precision);
                os << std::get<double>(value);
                return os.str();
            }
            case result_type::matrix:
            {
                const matrix& m = std::get<matrix>(value);

                std::ostringstream os;

                os << '{';

                for (size_t row = 0; row < m.row_size(); ++row)
                {
                    if (row > 0)
                        os << cxt.get_config().sep_matrix_row;

                    for (size_t col = 0; col < m.col_size(); ++col)
                    {
                        if (col > 0)
                            os << cxt.get_config().sep_matrix_column;

                        matrix::element e = m.get(row, col);

                        switch (e.type)
                        {
                            case matrix::element_type::numeric:
                            {
                                os << std::get<double>(e.value);
                                break;
                            }
                            case matrix::element_type::string:
                            {
                                os << '"' << std::get<std::string_view>(e.value) << '"';
                                break;
                            }
                            case matrix::element_type::error:
                            {
                                os << get_formula_error_name(std::get<formula_error_t>(e.value));
                                break;
                            }
                            case matrix::element_type::boolean:
                            {
                                os << std::get<bool>(e.value);
                                break;
                            }
                            default:
                                ;
                        }
                    }
                }

                os << '}';

                return os.str();
            }
            default:
                assert(!"unknown formula result type!");
        }
        return std::string{};
    }

    void parse(std::string_view s)
    {
        if (s.empty())
            return;

        switch (s[0])
        {
            case '#':
            {
                parse_error(s);
                break;
            }
            case '"':
            {
                parse_string(s);
                break;
            }
            case 't':
            case 'f':
            {
                // parse this as a boolean value.
                value = to_bool(s);
                type = result_type::boolean;
                break;
            }
            default:
            {
                // parse this as a number.
                value = to_double(s);
                type = result_type::value;
            }
        }
    }

    void move_from(formula_result&& r)
    {
        type = r.mp_impl->type;
        value = std::move(r.mp_impl->value);
    }

    bool equals(const formula_result& r) const
    {
        if (type != r.mp_impl->type)
            return false;

        return value == r.mp_impl->value;
    }

    void parse_error(std::string_view s)
    {
        assert(!s.empty());
        assert(s[0] == '#');

        formula_error_t err = to_formula_error_type(s);

        if (err == formula_error_t::no_error)
        {
            std::ostringstream os;
            os << "malformed error string: " << s;
            throw general_error(os.str());
        }

        value = err;
        type = result_type::error;
    }

    void parse_string(std::string_view s)
    {
        if (s.size() < 2u)
            // It needs to at least have the opening and closing quotes.
            return;

        assert(s[0] == '"');
        auto pos = s.find_first_of('"', 1);
        if (pos == std::string_view::npos)
            throw general_error("failed to parse string result.");

        type = result_type::string;
        value = std::string(&s[1], pos - 1);
    }
};

formula_result::formula_result() :
    mp_impl(std::make_unique<impl>()) {}

formula_result::formula_result(const formula_result& r) :
    mp_impl(std::make_unique<impl>(*r.mp_impl)) {}

formula_result::formula_result(formula_result&& r) : mp_impl(std::move(r.mp_impl)) {}

formula_result::formula_result(bool b) : mp_impl(std::make_unique<impl>(b)) {}

formula_result::formula_result(double v) : mp_impl(std::make_unique<impl>(v)) {}

formula_result::formula_result(std::string str) : mp_impl(std::make_unique<impl>(std::move(str))) {}

formula_result::formula_result(formula_error_t e) : mp_impl(std::make_unique<impl>(e)) {}

formula_result::formula_result(matrix mtx) : mp_impl(std::make_unique<impl>(std::move(mtx))) {}

formula_result::~formula_result() {}

void formula_result::reset()
{
    mp_impl->reset();
}

void formula_result::set_boolean(bool b)
{
    mp_impl->set_boolean(b);
}

void formula_result::set_value(double v)
{
    mp_impl->set_value(v);
}

void formula_result::set_string_value(std::string str)
{
    mp_impl->set_string_value(std::move(str));
}

void formula_result::set_error(formula_error_t e)
{
    mp_impl->set_error(e);
}

void formula_result::set_matrix(matrix mtx)
{
    mp_impl->set_matrix(std::move(mtx));
}

bool formula_result::get_boolean() const
{
    return mp_impl->get_boolean();
}

double formula_result::get_value() const
{
    return mp_impl->get_value();
}

const std::string& formula_result::get_string() const
{
    return mp_impl->get_string_value();
}

formula_error_t formula_result::get_error() const
{
    return mp_impl->get_error();
}

const matrix& formula_result::get_matrix() const
{
    return mp_impl->get_matrix();
}

matrix& formula_result::get_matrix()
{
    return mp_impl->get_matrix();
}

formula_result::result_type formula_result::get_type() const
{
    return mp_impl->get_type();
}

std::string formula_result::str(const model_context& cxt) const
{
    return mp_impl->str(cxt);
}

void formula_result::parse(std::string_view s)
{
    mp_impl->parse(s);
}

formula_result& formula_result::operator= (formula_result r)
{
    mp_impl->move_from(std::move(r));
    return *this;
}

bool formula_result::operator== (const formula_result& r) const
{
    return mp_impl->equals(r);
}

bool formula_result::operator!= (const formula_result& r) const
{
    return !operator== (r);
}

std::ostream& operator<< (std::ostream& os, formula_result::result_type v)
{
    switch (v)
    {
        case formula_result::result_type::error:
            os << "error";
            break;
        case formula_result::result_type::matrix:
            os << "matrix";
            break;
        case formula_result::result_type::string:
            os << "string";
            break;
        case formula_result::result_type::value:
            os << "value";
            break;
        case formula_result::result_type::boolean:
            os << "boolean";
            break;
        default:
            ;
    }

    return os;
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
