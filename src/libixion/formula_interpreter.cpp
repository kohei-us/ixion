/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "formula_interpreter.hpp"
#include "formula_functions.hpp"
#include "debug.hpp"

#include <ixion/cell.hpp>
#include <ixion/global.hpp>
#include <ixion/matrix.hpp>
#include <ixion/formula.hpp>
#include <ixion/interface/session_handler.hpp>
#include <ixion/interface/table_handler.hpp>
#include <ixion/config.hpp>
#include <ixion/cell_access.hpp>

#include <cassert>
#include <string>
#include <iostream>
#include <sstream>
#include <cmath>
#include <optional>

namespace ixion {

namespace {

class invalid_expression : public general_error
{
public:
    invalid_expression(const std::string& msg) : general_error(msg) {}
};

const formula_token paren_open = formula_token{fop_open};
const formula_token paren_close = formula_token{fop_close};

}

formula_interpreter::formula_interpreter(const formula_cell* cell, model_context& cxt) :
    m_parent_cell(cell),
    m_context(cxt),
    m_error(formula_error_t::no_error)
{
}

formula_interpreter::~formula_interpreter()
{
}

void formula_interpreter::set_origin(const abs_address_t& pos)
{
    m_pos = pos;
}

bool formula_interpreter::interpret()
{
    mp_handler = m_context.create_session_handler();
    if (mp_handler)
        mp_handler->begin_cell_interpret(m_pos);

    try
    {
        init_tokens();

        if (m_tokens.empty())
        {
            IXION_DEBUG("interpreter has no tokens to interpret");
            return false;
        }

        m_cur_token_itr = m_tokens.begin();
        m_error = formula_error_t::no_error;
        m_result.reset();

        expression();

        if (m_cur_token_itr != m_tokens.end())
        {
            if (mp_handler)
                mp_handler->set_invalid_expression("formula token interpretation ended prematurely.");
            return false;
        }

        pop_result();

        IXION_TRACE("interpretation successfully finished");

        if (mp_handler)
            mp_handler->end_cell_interpret();

        return true;
    }
    catch (const invalid_expression& e)
    {
        if (mp_handler)
            mp_handler->set_invalid_expression(e.what());

        m_error = formula_error_t::invalid_expression;
    }
    catch (const formula_error& e)
    {
        IXION_DEBUG("formula error: " << e.what());

        if (mp_handler)
            mp_handler->set_formula_error(e.what());

        m_error = e.get_error();
    }

    if (mp_handler)
        mp_handler->end_cell_interpret();

    return false;
}

formula_result formula_interpreter::transfer_result()
{
    return std::move(m_result);
}

formula_error_t formula_interpreter::get_error() const
{
    return m_error;
}

void formula_interpreter::init_tokens()
{
    clear_stacks();

    name_set used_names;
    m_tokens.clear();

    const formula_tokens_store_ptr_t& ts = m_parent_cell->get_tokens();
    if (!ts)
        return;

    for (const formula_token& t : ts->get())
    {
        if (t.opcode == fop_named_expression)
        {
            // Named expression.  Expand it.
            const auto& name = std::get<std::string>(t.value);
            const named_expression_t* expr = m_context.get_named_expression(
                m_pos.sheet, name);

            used_names.insert(name);
            expand_named_expression(expr, used_names);
        }
        else
            // Normal token.
            m_tokens.push_back(&t);
    }

    m_end_token_pos = m_tokens.end();
}

namespace {

void get_result_from_cell(const model_context& cxt, const abs_address_t& addr, formula_result& res)
{
    switch (cxt.get_celltype(addr))
    {
        case celltype_t::formula:
        {
            res = cxt.get_formula_result(addr);
            break;
        }
        case celltype_t::boolean:
            res.set_boolean(cxt.get_boolean_value(addr));
            break;
        case celltype_t::numeric:
            res.set_value(cxt.get_numeric_value(addr));
            break;
        case celltype_t::string:
        {
            std::string_view s = cxt.get_string_value(addr);
            res.set_string_value(std::string{s});
            break;
        }
        case celltype_t::unknown:
        default:
            ;
    }
}

}

void formula_interpreter::pop_result()
{
    // there should only be one stack value left for the result value.
    assert(get_stack().size() == 1);
    stack_value& res = get_stack().back();
    switch (res.get_type())
    {
        case stack_value_t::range_ref:
        {
            const abs_range_t& range = res.get_range();
            get_result_from_cell(m_context, range.first, m_result);
            break;
        }
        case stack_value_t::single_ref:
        {
            get_result_from_cell(m_context, res.get_address(), m_result);
            break;
        }
        case stack_value_t::string:
        {
            m_result.set_string_value(res.get_string());
            break;
        }
        case stack_value_t::boolean:
        {
            m_result.set_boolean(res.get_boolean());
            break;
        }
        case stack_value_t::value:
            IXION_TRACE("value=" << res.get_value());
            m_result.set_value(res.get_value());
            break;
        case stack_value_t::matrix:
            m_result.set_matrix(res.pop_matrix());
            break;
        case stack_value_t::error:
            m_result.set_error(res.get_error());
            break;
    }

    if (mp_handler)
        mp_handler->set_result(m_result);
}

void formula_interpreter::expand_named_expression(const named_expression_t* expr, name_set& used_names)
{
    if (!expr)
        throw formula_error(formula_error_t::name_not_found);

    m_tokens.push_back(&paren_open);
    for (const auto& t : expr->tokens)
    {
        if (t.opcode == fop_named_expression)
        {
            const auto& expr_name = std::get<std::string>(t.value);
            if (used_names.count(expr_name) > 0)
            {
                // Circular reference detected.
                throw invalid_expression("circular referencing of named expressions");
            }
            const named_expression_t* this_expr = m_context.get_named_expression(m_pos.sheet, expr_name);
            used_names.insert(expr_name);
            expand_named_expression(this_expr, used_names);
        }
        else
            m_tokens.push_back(&t);
    }
    m_tokens.push_back(&paren_close);
}

void formula_interpreter::ensure_token_exists() const
{
    if (!has_token())
        throw invalid_expression("formula expression ended prematurely");
}

bool formula_interpreter::has_token() const
{
    return m_cur_token_itr != m_end_token_pos;
}

void formula_interpreter::next()
{
    ++m_cur_token_itr;
}

const formula_token& formula_interpreter::token() const
{
    assert(m_cur_token_itr != m_end_token_pos);
    return *(*m_cur_token_itr);
}

const formula_token& formula_interpreter::token_or_throw() const
{
    ensure_token_exists();
    return *(*m_cur_token_itr);
}

const formula_token& formula_interpreter::next_token()
{
    next();
    if (!has_token())
        throw invalid_expression("expecting a token but no more tokens found.");

    return token();
}

const std::string& formula_interpreter::string_or_throw() const
{
    assert(token().opcode == fop_string);

    const string_id_t sid = std::get<string_id_t>(token().value);
    const std::string* p = m_context.get_string(sid);
    if (!p)
        throw general_error("no string found for the specified string ID.");

    if (mp_handler)
        mp_handler->push_string(sid);

    return *p;
}

namespace {

bool valid_expression_op(fopcode_t oc)
{
    switch (oc)
    {
        case fop_plus:
        case fop_minus:
        case fop_equal:
        case fop_not_equal:
        case fop_less:
        case fop_less_equal:
        case fop_greater:
        case fop_greater_equal:
            return true;
        default:
            ;
    }
    return false;
}

/**
 * Pop the value off of the stack but only as one of the following type:
 *
 * <ul>
 * <li>value</li>
 * <li>string</li>
 * <li>matrix</li>
 * </ul>
 */
std::optional<stack_value> pop_stack_value(const model_context& cxt, formula_value_stack& stack)
{
    switch (stack.get_type())
    {
        case stack_value_t::boolean:
            return stack_value{stack.pop_boolean() ? 1.0 : 0.0};
        case stack_value_t::value:
            return stack_value{stack.pop_value()};
        case stack_value_t::string:
            return stack_value{stack.pop_string()};
        case stack_value_t::matrix:
            return stack_value{stack.pop_matrix()};
        case stack_value_t::single_ref:
        {
            const abs_address_t& addr = stack.pop_single_ref();
            auto ca = cxt.get_cell_access(addr);

            switch (ca.get_type())
            {
                case celltype_t::empty:
                {
                    // empty cell has a value of 0.
                    return stack_value{0.0};
                }
                case celltype_t::boolean:
                    // TODO : Decide whether we need to treat this as a
                    // distinct boolean value.  For now, let's treat this as a
                    // numeric value equivalent.
                case celltype_t::numeric:
                {
                    double val = ca.get_numeric_value();
                    return stack_value{val};
                }
                case celltype_t::string:
                {
                    std::size_t strid = ca.get_string_identifier();
                    const std::string* ps = cxt.get_string(strid);
                    if (!ps)
                    {
                        IXION_DEBUG("fail to get a string value for the id of " << strid);
                        return {};
                    }

                    return stack_value{*ps};
                }
                case celltype_t::formula:
                {
                    formula_result res = ca.get_formula_result();

                    switch (res.get_type())
                    {
                        case formula_result::result_type::boolean:
                            return stack_value{res.get_boolean() ? 1.0 : 0.0};
                        case formula_result::result_type::value:
                            return stack_value{res.get_value()};
                        case formula_result::result_type::string:
                            return stack_value{res.get_string()};
                        case formula_result::result_type::error:
                        default:
                            return {};
                    }
                }
                default:
                    return {};
            }
            break;
        }
        case stack_value_t::range_ref:
        default:;
    }

    return {};
}

void compare_values(formula_value_stack& vs, fopcode_t oc, double val1, double val2)
{
    switch (oc)
    {
        case fop_plus:
            vs.push_value(val1 + val2);
        break;
        case fop_minus:
            vs.push_value(val1 - val2);
        break;
        case fop_equal:
            vs.push_value(val1 == val2);
        break;
        case fop_not_equal:
            vs.push_value(val1 != val2);
        break;
        case fop_less:
            vs.push_value(val1 < val2);
        break;
        case fop_less_equal:
            vs.push_value(val1 <= val2);
        break;
        case fop_greater:
            vs.push_value(val1 > val2);
        break;
        case fop_greater_equal:
            vs.push_value(val1 >= val2);
        break;
        default:
            throw invalid_expression("unknown expression operator.");
    }
}

void compare_strings(formula_value_stack& vs, fopcode_t oc, const std::string& str1, const std::string& str2)
{
    switch (oc)
    {
        case fop_plus:
        case fop_minus:
            throw formula_error(formula_error_t::invalid_expression);
        case fop_equal:
            vs.push_value(str1 == str2);
        break;
        case fop_not_equal:
            vs.push_value(str1 != str2);
        break;
        case fop_less:
            vs.push_value(str1 < str2);
        break;
        case fop_less_equal:
            vs.push_value(str1 <= str2);
        break;
        case fop_greater:
            vs.push_value(str1 > str2);
        break;
        case fop_greater_equal:
            vs.push_value(str1 >= str2);
        break;
        default:
            throw invalid_expression("unknown expression operator.");
    }
}

void compare_value_to_string(
    formula_value_stack& vs, fopcode_t oc, double /*val1*/, const std::string& /*str2*/)
{
    // Value 1 is numeric while value 2 is string.  String is
    // always greater than numeric value.
    switch (oc)
    {
        case fop_plus:
        case fop_minus:
            throw formula_error(formula_error_t::invalid_expression);
        case fop_equal:
            vs.push_value(false);
        break;
        case fop_not_equal:
            vs.push_value(true);
        break;
        case fop_less:
        case fop_less_equal:
            // val1 < str2
            vs.push_value(true);
        break;
        case fop_greater:
        case fop_greater_equal:
            // val1 > str2
            vs.push_value(false);
        break;
        default:
            throw invalid_expression("unknown expression operator.");
    }
}

void compare_string_to_value(
    formula_value_stack& vs, fopcode_t oc, const std::string& /*str1*/, double /*val2*/)
{
    switch (oc)
    {
        case fop_plus:
        case fop_minus:
            throw formula_error(formula_error_t::invalid_expression);
        case fop_equal:
            vs.push_value(false);
        break;
        case fop_not_equal:
            vs.push_value(true);
        break;
        case fop_less:
        case fop_less_equal:
            // str1 < val2
            vs.push_value(false);
        break;
        case fop_greater:
        case fop_greater_equal:
            // str1 > val2
            vs.push_value(true);
        break;
        default:
            throw invalid_expression("unknown expression operator.");
    }
}

namespace {

template<typename Op>
matrix operate_all_elements(const matrix& mtx, double val)
{
    matrix res = mtx;

    for (std::size_t col = 0; col < mtx.col_size(); ++col)
    {
        for (std::size_t row = 0; row < mtx.row_size(); ++row)
        {
            auto elem = mtx.get(row, col);

            switch (elem.type)
            {
                case matrix::element_type::numeric:
                    res.set(row, col, Op{}(std::get<double>(elem.value), val));
                    break;
                case matrix::element_type::string:
                    break;
                case matrix::element_type::boolean:
                    res.set(row, col, Op{}(std::get<bool>(elem.value), val));
                    break;
                case matrix::element_type::error:
                    res.set(row, col, std::get<formula_error_t>(elem.value));
                    break;
                case matrix::element_type::empty:
                    break;
            }
        }
    }

    return res;
}

template<typename Op>
matrix operate_all_elements(double val, const matrix& mtx)
{
    matrix res = mtx;

    for (std::size_t col = 0; col < mtx.col_size(); ++col)
    {
        for (std::size_t row = 0; row < mtx.row_size(); ++row)
        {
            auto elem = mtx.get(row, col);

            switch (elem.type)
            {
                case matrix::element_type::numeric:
                    res.set(row, col, Op{}(val, std::get<double>(elem.value)));
                    break;
                case matrix::element_type::string:
                    break;
                case matrix::element_type::boolean:
                    res.set(row, col, Op{}(val, std::get<bool>(elem.value)));
                    break;
                case matrix::element_type::error:
                    res.set(row, col, std::get<formula_error_t>(elem.value));
                    break;
                case matrix::element_type::empty:
                    break;
            }
        }
    }

    return res;
}

struct add_op
{
    double operator()(double v1, double v2) const
    {
        return v1 + v2;
    }
};

struct sub_op
{
    double operator()(double v1, double v2) const
    {
        return v1 - v2;
    }
};

struct equal_op
{
    bool operator()(double v1, double v2) const
    {
        return v1 == v2;
    }
};

struct not_equal_op
{
    bool operator()(double v1, double v2) const
    {
        return v1 != v2;
    }
};

struct less_op
{
    bool operator()(double v1, double v2) const
    {
        return v1 < v2;
    }
};

struct less_equal_op
{
    bool operator()(double v1, double v2) const
    {
        return v1 <= v2;
    }
};

struct greater_op
{
    bool operator()(double v1, double v2) const
    {
        return v1 > v2;
    }
};

struct greater_equal_op
{
    bool operator()(double v1, double v2) const
    {
        return v1 >= v2;
    }
};

}

void compare_matrix_to_value(formula_value_stack& vs, fopcode_t oc, const matrix& mtx, double val)
{
    switch (oc)
    {
        case fop_minus:
            val = -val;
            // fallthrough
        case fop_plus:
        {
            matrix res = operate_all_elements<add_op>(mtx, val);
            vs.push_matrix(res);
            break;
        }
        case fop_equal:
        {
            matrix res = operate_all_elements<equal_op>(mtx, val);
            vs.push_matrix(res);
            break;
        }
        case fop_not_equal:
        {
            matrix res = operate_all_elements<not_equal_op>(mtx, val);
            vs.push_matrix(res);
            break;
        }
        case fop_less:
        {
            matrix res = operate_all_elements<less_op>(mtx, val);
            vs.push_matrix(res);
            break;
        }
        case fop_less_equal:
        {
            matrix res = operate_all_elements<less_equal_op>(mtx, val);
            vs.push_matrix(res);
            break;
        }
        case fop_greater:
        {
            matrix res = operate_all_elements<greater_op>(mtx, val);
            vs.push_matrix(res);
            break;
        }
        case fop_greater_equal:
        {
            matrix res = operate_all_elements<greater_equal_op>(mtx, val);
            vs.push_matrix(res);
            break;
        }
        default:
            throw invalid_expression("unknown expression operator.");
    }
}

void compare_value_to_matrix(formula_value_stack& vs, fopcode_t oc, double val, const matrix& mtx)
{
    switch (oc)
    {
        case fop_minus:
        {
            matrix res = operate_all_elements<sub_op>(val, mtx);
            vs.push_matrix(res);
            break;
        }
        case fop_plus:
        {
            matrix res = operate_all_elements<add_op>(val, mtx);
            vs.push_matrix(res);
            break;
        }
        case fop_equal:
        {
            matrix res = operate_all_elements<equal_op>(val, mtx);
            vs.push_matrix(res);
            break;
        }
        case fop_not_equal:
        {
            matrix res = operate_all_elements<not_equal_op>(val, mtx);
            vs.push_matrix(res);
            break;
        }
        case fop_less:
        {
            matrix res = operate_all_elements<less_op>(val, mtx);
            vs.push_matrix(res);
            break;
        }
        case fop_less_equal:
        {
            matrix res = operate_all_elements<less_equal_op>(val, mtx);
            vs.push_matrix(res);
            break;
        }
        case fop_greater:
        {
            matrix res = operate_all_elements<greater_op>(val, mtx);
            vs.push_matrix(res);
            break;
        }
        case fop_greater_equal:
        {
            matrix res = operate_all_elements<greater_equal_op>(val, mtx);
            vs.push_matrix(res);
            break;
        }
        default:
            throw invalid_expression("unknown expression operator.");
    }
}

} // anonymous namespace

void formula_interpreter::expression()
{
    // <term> + <term> + <term> + ... + <term>
    // valid operators are: +, -, =, <, >, <=, >=, <>.

    term();
    while (has_token())
    {
        fopcode_t oc = token().opcode;
        if (!valid_expression_op(oc))
            return;

        auto sv1 = pop_stack_value(m_context, get_stack());
        if (!sv1)
        {
            IXION_DEBUG("failed to pop value from the stack");
            throw formula_error(formula_error_t::general_error);
        }

        if (mp_handler)
            mp_handler->push_token(oc);

        next();
        term();

        auto sv2 = pop_stack_value(m_context, get_stack());
        if (!sv2)
        {
            IXION_DEBUG("failed to pop value from the stack");
            throw formula_error(formula_error_t::general_error);
        }

        switch (sv1->get_type())
        {
            case stack_value_t::value:
            {
                switch (sv2->get_type())
                {
                    case stack_value_t::value:
                    {
                        // Both are numeric values.
                        compare_values(get_stack(), oc, sv1->get_value(), sv2->get_value());
                        break;
                    }
                    case stack_value_t::string:
                    {
                        compare_value_to_string(get_stack(), oc, sv1->get_value(), sv2->get_string());
                        break;
                    }
                    case stack_value_t::matrix:
                    {
                        compare_value_to_matrix(get_stack(), oc, sv1->get_value(), sv2->get_matrix());
                        break;
                    }
                    default:
                    {
                        IXION_DEBUG("unsupported value type for value 2: " << sv2->get_type());
                        throw formula_error(formula_error_t::general_error);
                    }
                }
                break;
            }
            case stack_value_t::string:
            {
                switch (sv2->get_type())
                {
                    case stack_value_t::value:
                    {
                        // Value 1 is string while value 2 is numeric.
                        compare_string_to_value(get_stack(), oc, sv1->get_string(), sv2->get_value());
                        break;
                    }
                    case stack_value_t::string:
                    {
                        // Both are strings.
                        compare_strings(get_stack(), oc, sv1->get_string(), sv2->get_string());
                        break;
                    }
                    default:
                    {
                        IXION_DEBUG("unsupported value type for value 2: " << sv2->get_type());
                        throw formula_error(formula_error_t::general_error);
                    }
                }
                break;
            }
            case stack_value_t::matrix:
            {
                switch (sv2->get_type())
                {
                    case stack_value_t::value:
                    {
                        compare_matrix_to_value(get_stack(), oc, sv1->get_matrix(), sv2->get_value());
                        break;
                    }
                    default:
                    {
                        IXION_DEBUG("unsupported value type for value 2: " << sv2->get_type());
                        throw formula_error(formula_error_t::general_error);
                    }
                }
                break;
            }
            default:
            {
                IXION_DEBUG("unsupported value type for value 1: " << sv1->get_type());
                throw formula_error(formula_error_t::general_error);
            }
        }
    }
}

void formula_interpreter::term()
{
    // <factor> || <factor> * <term>

    factor();
    if (!has_token())
        return;

    fopcode_t oc = token().opcode;
    switch (oc)
    {
        case fop_multiply:
        {
            if (mp_handler)
                mp_handler->push_token(oc);

            next();
            double val = get_stack().pop_value();
            term();
            get_stack().push_value(val*get_stack().pop_value());
            return;
        }
        case fop_exponent:
        {
            if (mp_handler)
                mp_handler->push_token(oc);

            next();
            double base = get_stack().pop_value();
            term();
            double exp = get_stack().pop_value();
            get_stack().push_value(std::pow(base, exp));
            return;
        }
        case fop_concat:
        {
            if (mp_handler)
                mp_handler->push_token(oc);

            next();
            std::string s1 = get_stack().pop_string();
            term();
            std::string s2 = get_stack().pop_string();
            std::string s = s1 + s2;
            get_stack().push_string(std::move(s));
            return;
        }
        case fop_divide:
        {
            if (mp_handler)
                mp_handler->push_token(oc);

            next();
            double val = get_stack().pop_value();
            term();
            double val2 = get_stack().pop_value();
            if (val2 == 0.0)
                throw formula_error(formula_error_t::division_by_zero);
            get_stack().push_value(val/val2);
            return;
        }
        default:
            ;
    }
}

void formula_interpreter::factor()
{
    // <constant> || <variable> || '(' <expression> ')' || <function>

    bool negative_sign = sign(); // NB: may be precedeed by a '+' or '-' sign.
    fopcode_t oc = token().opcode;

    switch (oc)
    {
        case fop_open:
            paren();
            break;
        case fop_named_expression:
        {
            // All named expressions are supposed to be expanded prior to interpretation.
            IXION_DEBUG("named expression encountered in factor.");
            throw formula_error(formula_error_t::general_error);
        }
        case fop_value:
        {
            constant();
            break;
        }
        case fop_single_ref:
            single_ref();
            break;
        case fop_range_ref:
            range_ref();
            break;
        case fop_table_ref:
            table_ref();
            break;
        case fop_function:
            function();
            break;
        case fop_string:
            literal();
            break;
        case fop_array_open:
            array();
            break;
        default:
            std::ostringstream os;
            os << "factor: unexpected token type: <" << get_opcode_name(oc) << ">";
            throw invalid_expression(os.str());
    }

    if (negative_sign)
    {
        double v = get_stack().pop_value();
        get_stack().push_value(v * -1.0);
    }
}

bool formula_interpreter::sign()
{
    ensure_token_exists();

    fopcode_t oc = token().opcode;
    bool sign_set = false;

    switch (oc)
    {
        case fop_minus:
            sign_set = true;
            // fall through
        case fop_plus:
        {
            if (mp_handler)
                mp_handler->push_token(oc);

            next();

            if (!has_token())
                throw invalid_expression("sign: a sign cannot be the last token");
        }
        default:
            ;
    }

    return sign_set;
}

void formula_interpreter::paren()
{
    if (mp_handler)
        mp_handler->push_token(fop_open);

    next();
    expression();
    if (token_or_throw().opcode != fop_close)
        throw invalid_expression("paren: expected close paren");

    if (mp_handler)
        mp_handler->push_token(fop_close);

    next();
}

void formula_interpreter::single_ref()
{
    const address_t& addr = std::get<address_t>(token().value);
    IXION_TRACE("ref=" << addr.get_name() << "; origin=" << m_pos.get_name());

    if (mp_handler)
        mp_handler->push_single_ref(addr, m_pos);

    abs_address_t abs_addr = addr.to_abs(m_pos);
    IXION_TRACE("ref=" << abs_addr.get_name() << " (converted to absolute)");

    if (abs_addr == m_pos)
    {
        // self-referencing is not permitted.
        throw formula_error(formula_error_t::ref_result_not_available);
    }

    get_stack().push_single_ref(abs_addr);
    next();
}

void formula_interpreter::range_ref()
{
    const range_t& range = std::get<range_t>(token().value);
    IXION_TRACE("ref-start=" << range.first.get_name() << "; ref-end=" << range.last.get_name() << "; origin=" << m_pos.get_name());

    if (mp_handler)
        mp_handler->push_range_ref(range, m_pos);

    abs_range_t abs_range = range.to_abs(m_pos);
    abs_range.reorder();

    IXION_TRACE("ref-start=" << abs_range.first.get_name() << "; ref-end=" << abs_range.last.get_name() << " (converted to absolute)");

    // Check the reference range to make sure it doesn't include the parent cell.
    if (abs_range.contains(m_pos))
    {
        // Referenced range contains the address of this cell.  Not good.
        throw formula_error(formula_error_t::ref_result_not_available);
    }

    get_stack().push_range_ref(abs_range);
    next();
}

void formula_interpreter::table_ref()
{
    const iface::table_handler* table_hdl = m_context.get_table_handler();
    if (!table_hdl)
    {
        IXION_DEBUG("failed to get a table_handler instance.");
        throw formula_error(formula_error_t::ref_result_not_available);
    }

    const table_t& table = std::get<table_t>(token().value);

    if (mp_handler)
        mp_handler->push_table_ref(table);

    abs_range_t range(abs_range_t::invalid);
    if (table.name != empty_string_id)
    {
        range = table_hdl->get_range(table.name, table.column_first, table.column_last, table.areas);
    }
    else
    {
        // Table name is not given.  Use the current cell position to infer
        // which table to use.
        range = table_hdl->get_range(m_pos, table.column_first, table.column_last, table.areas);
    }

    get_stack().push_range_ref(range);
    next();
}

void formula_interpreter::constant()
{
    double val = std::get<double>(token().value);
    next();
    get_stack().push_value(val);
    if (mp_handler)
        mp_handler->push_value(val);
}

void formula_interpreter::literal()
{
    const std::string& s = string_or_throw();

    next();
    get_stack().push_string(s);
}

void formula_interpreter::array()
{
    // '{' <constant> or <literal> ',' or ';' <constant> or <literal> ',' or ';' .... '}'
    assert(token().opcode == fop_array_open);

    if (mp_handler)
        mp_handler->push_token(fop_array_open);

    next(); // skip '{'

    std::vector<double> values;
    std::vector<std::tuple<std::size_t, std::size_t, std::string>> strings;
    std::size_t row = 0;
    std::size_t col = 0;
    std::optional<std::size_t> prev_col;

    fopcode_t prev_op = fop_array_open;

    for (; has_token(); next())
    {
        bool has_sign = false;

        switch (prev_op)
        {
            case fop_array_open:
            case fop_sep:
            case fop_array_row_sep:
                has_sign = sign();
                break;
            default:;
        }

        switch (token().opcode)
        {
            case fop_string:
            {
                switch (prev_op)
                {
                    case fop_array_open:
                    case fop_sep:
                    case fop_array_row_sep:
                        break;
                    default:
                        throw invalid_expression("array: invalid placement of value");
                }

                strings.emplace_back(row, col, string_or_throw());
                values.push_back(0); // placeholder value, will be replaced

                ++col;
                break;
            }
            case fop_value:
            {
                switch (prev_op)
                {
                    case fop_minus:
                    case fop_plus:
                    case fop_array_open:
                    case fop_sep:
                    case fop_array_row_sep:
                        break;
                    default:
                        throw invalid_expression("array: invalid placement of value");
                }

                double v = std::get<double>(token().value);

                if (mp_handler)
                    mp_handler->push_value(v);

                if (has_sign)
                    v = -v;

                values.push_back(v);

                ++col;
                break;
            }
            case fop_sep:
            {
                switch (prev_op)
                {
                    case fop_value:
                    case fop_string:
                        break;
                    default:
                        throw invalid_expression("array: unexpected separator");
                }

                if (mp_handler)
                    mp_handler->push_token(fop_sep);
                break;
            }
            case fop_array_row_sep:
            {
                switch (prev_op)
                {
                    case fop_value:
                    case fop_string:
                        break;
                    default:
                        throw invalid_expression("array: unexpected row separator");
                }

                if (mp_handler)
                    mp_handler->push_token(fop_array_row_sep);

                ++row;

                if (prev_col && *prev_col != col)
                    throw invalid_expression("array: inconsistent column width");

                prev_col = col;
                col = 0;
                break;
            }
            case fop_array_close:
            {
                switch (prev_op)
                {
                    case fop_array_open:
                    case fop_value:
                    case fop_string:
                        break;
                    default:
                        throw invalid_expression("array: invalid placement of array close operator");
                }

                if (prev_col && *prev_col != col)
                    throw invalid_expression("array: inconsistent column width");

                ++row;

                // Stored values are in row-major order, but the matrix expects a column-major array.
                numeric_matrix num_mtx_transposed(std::move(values), col, row);
                numeric_matrix num_mtx(row, col);

                for (std::size_t r = 0; r < row; ++r)
                    for (std::size_t c = 0; c < col; ++c)
                        num_mtx(r, c) = num_mtx_transposed(c, r);

                if (strings.empty())
                    // pure numeric matrix
                    get_stack().push_matrix(std::move(num_mtx));
                else
                {
                    // multi-type matrix
                    matrix mtx(num_mtx);
                    for (const auto& [r, c, str] : strings)
                        mtx.set(r, c, str);

                    get_stack().push_matrix(std::move(mtx));
                }

                if (mp_handler)
                    mp_handler->push_token(fop_array_close);

                next(); // skip '}'
                return;
            }
            default:
            {
                std::ostringstream os;
                os << "array: unexpected token type: <" << get_opcode_name(token().opcode) << ">";
                throw invalid_expression(os.str());
            }
        }

        prev_op = token().opcode;
    }

    throw invalid_expression("array: ended prematurely");
}

void formula_interpreter::function()
{
    // <func name> '(' <expression> ',' <expression> ',' ... ',' <expression> ')'
    ensure_token_exists();
    assert(token().opcode == fop_function);
    formula_function_t func_oc = formula_functions::get_function_opcode(token());
    if (mp_handler)
        mp_handler->push_function(func_oc);

    push_stack();

    IXION_TRACE("function='" << get_formula_function_name(func_oc) << "'");
    assert(get_stack().empty());

    if (next_token().opcode != fop_open)
        throw invalid_expression("expecting a '(' after a function name.");

    if (mp_handler)
        mp_handler->push_token(fop_open);

    fopcode_t oc = next_token().opcode;
    bool expect_sep = false;
    while (oc != fop_close)
    {
        if (expect_sep)
        {
            if (oc != fop_sep)
                throw invalid_expression("argument separator is expected, but not found.");
            next();
            expect_sep = false;

            if (mp_handler)
                mp_handler->push_token(oc);
        }
        else
        {
            expression();
            expect_sep = true;
        }
        oc = token_or_throw().opcode;
    }

    if (mp_handler)
        mp_handler->push_token(oc);

    next();

    // Function call pops all stack values pushed onto the stack this far, and
    // pushes the result onto the stack.
    formula_functions(m_context, m_pos).interpret(func_oc, get_stack());
    assert(get_stack().size() == 1);

    pop_stack();
}

void formula_interpreter::clear_stacks()
{
    m_stacks.clear();
    m_stacks.emplace_back(m_context);
}

void formula_interpreter::push_stack()
{
    m_stacks.emplace_back(m_context);
}

void formula_interpreter::pop_stack()
{
    assert(m_stacks.size() >= 2);
    assert(m_stacks.back().size() == 1);
    auto tmp = m_stacks.back().release_back();
    m_stacks.pop_back();
    m_stacks.back().push_back(std::move(tmp));
}

formula_value_stack& formula_interpreter::get_stack()
{
    assert(!m_stacks.empty());
    return m_stacks.back();
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
