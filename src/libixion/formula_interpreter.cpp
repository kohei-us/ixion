/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "formula_interpreter.hpp"
#include "formula_functions.hpp"
#include "concrete_formula_tokens.hpp"

#include "ixion/cell.hpp"
#include "ixion/global.hpp"
#include "ixion/matrix.hpp"
#include "ixion/formula.hpp"
#include "ixion/interface/formula_model_access.hpp"
#include "ixion/interface/session_handler.hpp"
#include "ixion/interface/table_handler.hpp"

#include <cassert>
#include <string>
#include <iostream>
#include <sstream>
#include <spdlog/spdlog.h>

using namespace std;

namespace ixion {

namespace {

class invalid_expression : public general_error
{
public:
    invalid_expression(const string& msg) : general_error(msg) {}
};

opcode_token paren_open = opcode_token(fop_open);
opcode_token paren_close = opcode_token(fop_close);

}

formula_interpreter::formula_interpreter(const formula_cell* cell, iface::formula_model_access& cxt) :
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
            SPDLOG_DEBUG(spdlog::get("ixion"), "interpret: interpreter has no tokens to interpret");
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

        SPDLOG_TRACE(spdlog::get("ixion"), "interpret: interpretation successfully finished");

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
        SPDLOG_DEBUG(spdlog::get("ixion"), "interpret: formula error: {}", e.what());

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

    const formula_tokens_t& src_tokens = ts->get();

    for (const std::unique_ptr<formula_token>& p : src_tokens)
    {
        if (p->get_opcode() == fop_named_expression)
        {
            // Named expression.  Expand it.
            const formula_tokens_t* expr = m_context.get_named_expression(
                m_pos.sheet, p->get_name());
            used_names.insert(p->get_name());
            expand_named_expression(expr, used_names);
        }
        else
            // Normal token.
            m_tokens.push_back(p.get());
    }

    m_end_token_pos = m_tokens.end();
}

namespace {

void get_result_from_cell(const iface::formula_model_access& cxt, const abs_address_t& addr, formula_result& res)
{
    switch (cxt.get_celltype(addr))
    {
        case celltype_t::formula:
        {
            const formula_cell* fcell = cxt.get_formula_cell(addr);
            if (!fcell)
                return;

            res = fcell->get_result_cache();
            break;
        }
        case celltype_t::boolean:
            // TODO : treat this as a numeric value for now.  Later we need to
            // decide whether we need to treat this as a distinct boolean
            // type.
        case celltype_t::numeric:
            res.set_value(cxt.get_numeric_value(addr));
            break;
        case celltype_t::string:
            res.set_string(cxt.get_string_identifier(addr));
            break;
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
            m_result.set_string(res.get_string());
            break;
        case stack_value_t::value:
            SPDLOG_TRACE(spdlog::get("ixion"), "pop_result: value={}", res.get_value());
            m_result.set_value(res.get_value());
            break;
        case stack_value_t::matrix:
            m_result.set_matrix(res.pop_matrix());
            break;
        default:
            ;
    }

    if (mp_handler)
        mp_handler->set_result(m_result);
}

void formula_interpreter::expand_named_expression(const formula_tokens_t* expr, name_set& used_names)
{
    if (!expr)
        throw formula_error(formula_error_t::name_not_found);

    m_tokens.push_back(&paren_open);
    formula_tokens_t::const_iterator itr = expr->begin(), itr_end = expr->end();
    for (; itr != itr_end; ++itr)
    {
        const formula_token& t = **itr;
        if (t.get_opcode() == fop_named_expression)
        {
            string expr_name = t.get_name();
            if (used_names.count(expr_name) > 0)
            {
                // Circular reference detected.
                throw invalid_expression("circular referencing of named expressions");
            }
            const formula_tokens_t* this_expr = m_context.get_named_expression(m_pos.sheet, expr_name);
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

bool pop_stack_value_or_string(const iface::formula_model_access& cxt,
    formula_value_stack& stack, stack_value_t& vt, double& val, string& str)
{
    vt = stack.get_type();
    switch (vt)
    {
        case stack_value_t::value:
            val = stack.pop_value();
        break;
        case stack_value_t::string:
            str = stack.pop_string();
        break;
        case stack_value_t::single_ref:
        {
            const abs_address_t& addr = stack.pop_single_ref();

            switch (cxt.get_celltype(addr))
            {
                case celltype_t::empty:
                {
                    // empty cell has a value of 0.
                    vt = stack_value_t::value;
                    val = 0.0;
                    return true;
                }
                case celltype_t::boolean:
                    // TODO : Decide whether we need to treat this as a
                    // distinct boolean value.  For now, let's treat this as a
                    // numeric value equivalent.
                case celltype_t::numeric:
                {
                    vt = stack_value_t::value;
                    val = cxt.get_numeric_value(addr);
                    return true;
                }
                case celltype_t::string:
                {
                    vt = stack_value_t::string;
                    size_t strid = cxt.get_string_identifier(addr);
                    const string* ps = cxt.get_string(strid);
                    if (!ps)
                        return false;
                    str = *ps;
                    return true;
                }
                case celltype_t::formula:
                {
                    const formula_cell* fc = cxt.get_formula_cell(addr);
                    assert(fc);
                    formula_result res = fc->get_result_cache();

                    switch (res.get_type())
                    {
                        case formula_result::result_type::value:
                        {
                            vt = stack_value_t::value;
                            val = res.get_value();
                            return true;
                        }
                        case formula_result::result_type::string:
                        {
                            vt = stack_value_t::string;
                            string_id_t strid = res.get_string();
                            const string* ps = cxt.get_string(strid);
                            if (!ps)
                                return false;
                            str = *ps;
                            return true;
                        }
                        case formula_result::result_type::error:
                        default:
                            return false;
                    }
                }
                default:
                    return false;
            }
        }
        break;
        case stack_value_t::range_ref:
        default:
            return false;
    }
    return true;
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

}

void formula_interpreter::expression()
{
    // <term> + <term> + <term> + ... + <term>
    // valid operators are: +, -, =, <, >, <=, >=, <>.

    term();
    while (has_token())
    {
        fopcode_t oc = token().get_opcode();
        if (!valid_expression_op(oc))
            return;

        double val1 = 0.0, val2 = 0.0;
        string str1, str2;
        bool is_val1 = true, is_val2 = true;

        stack_value_t vt;
        if (!pop_stack_value_or_string(m_context, get_stack(), vt, val1, str1))
            throw formula_error(formula_error_t::general_error);
        is_val1 = vt == stack_value_t::value;

        if (mp_handler)
            mp_handler->push_token(oc);

        next();
        term();

        if (!pop_stack_value_or_string(m_context, get_stack(), vt, val2, str2))
            throw formula_error(formula_error_t::general_error);
        is_val2 = vt == stack_value_t::value;

        if (is_val1)
        {
            if (is_val2)
            {
                // Both are numeric values.
                compare_values(get_stack(), oc, val1, val2);
            }
            else
            {
                compare_value_to_string(get_stack(), oc, val1, str2);
            }
        }
        else
        {
            if (is_val2)
            {
                // Value 1 is string while value 2 is numeric.
                compare_string_to_value(get_stack(), oc, str1, val2);
            }
            else
            {
                // Both are strings.
                compare_strings(get_stack(), oc, str1, str2);
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

    fopcode_t oc = token().get_opcode();
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
    fopcode_t oc = token().get_opcode();

    switch (oc)
    {
        case fop_open:
            paren();
            break;
        case fop_named_expression:
        {
            // All named expressions are supposed to be expanded prior to interpretation.
            SPDLOG_DEBUG(spdlog::get("ixion"), "factor: named expression encountered in factor.");
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
        default:
            ostringstream os;
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

    fopcode_t oc = token().get_opcode();
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
    if (token_or_throw().get_opcode() != fop_close)
        throw invalid_expression("paren: expected close paren");

    if (mp_handler)
        mp_handler->push_token(fop_close);

    next();
}

void formula_interpreter::single_ref()
{
    address_t addr = token().get_single_ref();
    SPDLOG_TRACE(spdlog::get("ixion"), "single_ref: ref={}; origin={}", addr.get_name(), m_pos.get_name());

    if (mp_handler)
        mp_handler->push_single_ref(addr, m_pos);

    abs_address_t abs_addr = addr.to_abs(m_pos);
    SPDLOG_TRACE(spdlog::get("ixion"), "single_ref: ref={} (converted to absolute)", abs_addr.get_name());

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
    range_t range = token().get_range_ref();
    SPDLOG_TRACE(spdlog::get("ixion"), "range_ref: ref-start={}; ref-end={}; origin={}",
        range.first.get_name(), range.last.get_name(), m_pos.get_name());

    if (mp_handler)
        mp_handler->push_range_ref(range, m_pos);

    abs_range_t abs_range = range.to_abs(m_pos);

    SPDLOG_TRACE(spdlog::get("ixion"), "range_ref: ref-start={}; ref-end={} (converted to absolute)",
        abs_range.first.get_name(), abs_range.last.get_name(), m_pos.get_name());

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
        SPDLOG_DEBUG(spdlog::get("ixion"), "table_ref: failed to get a table_handler instance.");
        throw formula_error(formula_error_t::ref_result_not_available);
    }

    table_t table = token().get_table_ref();

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
    double val = token().get_value();
    next();
    get_stack().push_value(val);
    if (mp_handler)
        mp_handler->push_value(val);
}

void formula_interpreter::literal()
{
    size_t sid = token().get_index();
    next();
    get_stack().push_string(sid);
    if (mp_handler)
        mp_handler->push_string(sid);
}

void formula_interpreter::function()
{
    // <func name> '(' <expression> ',' <expression> ',' ... ',' <expression> ')'
    ensure_token_exists();
    assert(token().get_opcode() == fop_function);
    formula_function_t func_oc = formula_functions::get_function_opcode(token());
    if (mp_handler)
        mp_handler->push_function(func_oc);

    push_stack();

    SPDLOG_TRACE(spdlog::get("ixion"), "function: function='{}'", get_formula_function_name(func_oc));
    assert(get_stack().empty());

    if (next_token().get_opcode() != fop_open)
        throw invalid_expression("expecting a '(' after a function name.");

    if (mp_handler)
        mp_handler->push_token(fop_open);

    fopcode_t oc = next_token().get_opcode();
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
        oc = token_or_throw().get_opcode();
    }

    if (mp_handler)
        mp_handler->push_token(oc);

    next();

    // Function call pops all stack values pushed onto the stack this far, and
    // pushes the result onto the stack.
    formula_functions(m_context).interpret(func_oc, get_stack());
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
