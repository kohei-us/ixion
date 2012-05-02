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

#include "ixion/formula_interpreter.hpp"
#include "ixion/cell.hpp"
#include "ixion/global.hpp"
#include "ixion/formula_functions.hpp"
#include "ixion/formula_name_resolver.hpp"
#include "ixion/interface/model_context.hpp"
#include "ixion/interface/session_handler.hpp"

#include <string>
#include <iostream>
#include <sstream>

#define DEBUG_FORMULA_INTERPRETER 0

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

formula_interpreter::formula_interpreter(const formula_cell* cell, iface::model_context& cxt) :
    m_parent_cell(cell),
    m_context(cxt),
    mp_handler(NULL),
    m_stack(cxt),
    m_error(fe_no_error)
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
    mp_handler = m_context.get_session_handler();
    if (mp_handler)
        mp_handler->begin_cell_interpret(m_parent_cell);

    try
    {
        init_tokens();

        if (m_tokens.empty())
        {
#if DEBUG_FORMULA_INTERPRETER
            __IXION_DEBUG_OUT__ << "interpreter error: no tokens to interpret" << endl;
#endif
            return false;
        }

        m_cur_token_itr = m_tokens.begin();
        m_error = fe_no_error;
        m_result.reset();

        expression();
        if (m_cur_token_itr != m_tokens.end())
        {
            if (mp_handler)
                mp_handler->set_invalid_expression("formula token interpretation ended prematurely.");
            return false;
        }
        pop_result();

        return true;
    }
    catch (const invalid_expression& e)
    {
        if (mp_handler)
            mp_handler->set_invalid_expression(e.what());

        m_error = fe_invalid_expression;
    }
    catch (const formula_error& e)
    {
#if DEBUG_FORMULA_INTERPRETER
        __IXION_DEBUG_OUT__ << "formula error" << endl;
#endif
        if (mp_handler)
            mp_handler->set_formula_error(e.what());

        m_error = e.get_error();
    }
    return false;
}

const formula_result& formula_interpreter::get_result() const
{
    return m_result;
}

formula_error_t formula_interpreter::get_error() const
{
    return m_error;
}

void formula_interpreter::init_tokens()
{
    name_set used_names;
    m_tokens.clear();
    m_stack.clear();
    const formula_tokens_t* orig_tokens = NULL;
    if (m_parent_cell->is_shared())
        orig_tokens = m_context.get_shared_formula_tokens(m_pos.sheet, m_parent_cell->get_identifier());
    else
        orig_tokens = m_context.get_formula_tokens(m_pos.sheet, m_parent_cell->get_identifier());

    if (!orig_tokens)
        return;

    formula_tokens_t::const_iterator itr = orig_tokens->begin(), itr_end = orig_tokens->end();
    for (; itr != itr_end; ++itr)
    {
        const formula_token_base* p = &(*itr);
        assert(p);
        if (p->get_opcode() == fop_named_expression)
        {
            // Named expression.  Expand it.
            const formula_cell* expr = m_context.get_named_expression(p->get_name());
            used_names.insert(p->get_name());
            expand_named_expression(p->get_name(), expr, used_names);
        }
        else
            // Normal token.
            m_tokens.push_back(p);
    }
    m_end_token_pos = m_tokens.end();
}

namespace {

void get_result_from_cell(const iface::model_context& cxt, const abs_address_t& addr, formula_result& res)
{
    if (cxt.is_empty(addr))
        return;

    switch (cxt.get_celltype(addr))
    {
        case celltype_formula:
        {
            const formula_cell* fcell = cxt.get_formula_cell(addr);
            if (!fcell)
                return;

            const formula_result* fres = fcell->get_result_cache();
            if (fres)
                res = *fres;
        }
        break;
        case celltype_numeric:
            res.set_value(cxt.get_numeric_value(addr));
        break;
        case celltype_string:
            res.set_string(cxt.get_string_identifier(addr));
        break;
        case celltype_unknown:
        default:
            ;
    }
}

}

void formula_interpreter::pop_result()
{
    // there should only be one stack value left for the result value.
    assert(m_stack.size() == 1);
    const stack_value& res = m_stack.back();
    switch (res.get_type())
    {
        case sv_range_ref:
        {
            const abs_range_t& range = res.get_range();
            get_result_from_cell(m_context, range.first, m_result);
        }
        break;
        case sv_single_ref:
        {
            get_result_from_cell(m_context, res.get_address(), m_result);
        }
        break;
        case sv_string:
            m_result.set_string(res.get_string());
        break;
        case sv_value:
            m_result.set_value(res.get_value());
        break;
        default:
            ;
    }

    if (mp_handler)
        mp_handler->set_result(m_result);
}

void formula_interpreter::expand_named_expression(
    const string& expr_name, const formula_cell* expr, name_set& used_names)
{
    if (!expr)
    {
        ostringstream os;
        os << "unable to find named expression '" << expr_name << "'";
        throw invalid_expression(os.str());
    }

    size_t tokens_id = expr->get_identifier();
    const formula_tokens_t* expr_tokens = m_context.get_formula_tokens(global_scope, tokens_id);
    if (!expr_tokens)
        return;

    m_tokens.push_back(&paren_open);
    formula_tokens_t::const_iterator itr = expr_tokens->begin(), itr_end = expr_tokens->end();
    for (; itr != itr_end; ++itr)
    {
        if (itr->get_opcode() == fop_named_expression)
        {
            string expr_name = itr->get_name();
            if (used_names.count(expr_name) > 0)
            {
                // Circular reference detected.
                throw invalid_expression("circular referencing of named expressions");
            }
            const formula_cell* expr = m_context.get_named_expression(expr_name);
            used_names.insert(expr_name);
            expand_named_expression(expr_name, expr, used_names);
        }
        else
            m_tokens.push_back(&(*itr));
    }
    m_tokens.push_back(&paren_close);
}

bool formula_interpreter::has_token() const
{
    return m_cur_token_itr != m_end_token_pos;
}

void formula_interpreter::next()
{
    ++m_cur_token_itr;
}

const formula_token_base& formula_interpreter::token() const
{
    return *(*m_cur_token_itr);
}

const formula_token_base& formula_interpreter::next_token()
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

bool pop_stack_value_or_string(const iface::model_context& cxt,
    value_stack_t& stack, stack_value_t& vt, double& val, string& str)
{
    vt = stack.get_type();
    switch (vt)
    {
        case sv_value:
            val = stack.pop_value();
        break;
        case sv_string:
            str = stack.pop_string();
        break;
        case sv_single_ref:
        {
            const abs_address_t& addr = stack.pop_single_ref();
            if (cxt.is_empty(addr))
            {
                // empty cell has a value of 0.
                vt = sv_value;
                val = 0.0;
                return true;
            }

            switch (cxt.get_celltype(addr))
            {
                case celltype_numeric:
                {
                    vt = sv_value;
                    val = cxt.get_numeric_value(addr);
                    return true;
                }
                case celltype_string:
                {
                    vt = sv_string;
                    size_t strid = cxt.get_string_identifier(addr);
                    const string* ps = cxt.get_string(strid);
                    if (!ps)
                        return false;
                    str = *ps;
                    return true;
                }
                case celltype_formula:
                {
                    const formula_cell* fc = cxt.get_formula_cell(addr);
                    assert(fc);
                    const formula_result* res = fc->get_result_cache();
                    if (!res)
                        return false;

                    switch (res->get_type())
                    {
                        case formula_result::rt_value:
                        {
                            vt = sv_value;
                            val = res->get_value();
                            return true;
                        }
                        case formula_result::rt_string:
                        {
                            vt = sv_string;
                            size_t strid = res->get_string();
                            const string* ps = cxt.get_string(strid);
                            if (!ps)
                                return false;
                            str = *ps;
                            return true;
                        }
                        case formula_result::rt_error:
                        default:
                            return false;
                    }
                }
                default:
                    return false;
            }
        }
        break;
        case sv_range_ref:
        default:
            return false;
    }
    return true;
}

void compare_values(value_stack_t& vs, fopcode_t oc, double val1, double val2)
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

void compare_strings(value_stack_t& vs, fopcode_t oc, const std::string& str1, const std::string& str2)
{
    switch (oc)
    {
        case fop_plus:
        case fop_minus:
            throw formula_error(fe_invalid_expression);
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
    value_stack_t& vs, fopcode_t oc, double /*val1*/, const std::string& /*str2*/)
{
    // Value 1 is numeric while value 2 is string.  String is
    // always greater than numeric value.
    switch (oc)
    {
        case fop_plus:
        case fop_minus:
            throw formula_error(fe_invalid_expression);
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
    value_stack_t& vs, fopcode_t oc, const std::string& /*str1*/, double /*val2*/)
{
    switch (oc)
    {
        case fop_plus:
        case fop_minus:
            throw formula_error(fe_invalid_expression);
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
        if (!pop_stack_value_or_string(m_context, m_stack, vt, val1, str1))
            throw formula_error(fe_general_error);
        is_val1 = vt == sv_value;

        if (mp_handler)
            mp_handler->push_token(oc);

        next();
        term();

        if (!pop_stack_value_or_string(m_context, m_stack, vt, val2, str2))
            throw formula_error(fe_general_error);
        is_val2 = vt == sv_value;

        if (is_val1)
        {
            if (is_val2)
            {
                // Both are numeric values.
                compare_values(m_stack, oc, val1, val2);
            }
            else
            {
                compare_value_to_string(m_stack, oc, val1, str2);
            }
        }
        else
        {
            if (is_val2)
            {
                // Value 1 is string while value 2 is numeric.
                compare_string_to_value(m_stack, oc, str1, val2);
            }
            else
            {
                // Both are strings.
                compare_strings(m_stack, oc, str1, str2);
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
            double val = m_stack.pop_value();
            term();
            m_stack.push_value(val*m_stack.pop_value());
            return;
        }
        case fop_divide:
        {
            if (mp_handler)
                mp_handler->push_token(oc);

            next();
            double val = m_stack.pop_value();
            term();
            double val2 = m_stack.pop_value();
            if (val2 == 0.0)
                throw formula_error(fe_division_by_zero);
            m_stack.push_value(val/val2);
            return;
        }
        default:
            ;
    }
}

void formula_interpreter::factor()
{
    // <constant> || <variable> || '(' <expression> ')' || <function>

    fopcode_t oc = token().get_opcode();
    switch (oc)
    {
        case fop_open:
            paren();
            return;
        case fop_named_expression:
#if DEBUG_FORMULA_INTERPRETER
            __IXION_DEBUG_OUT__ << "named expression encountered in factor" << endl;
#endif
            // All named expressions are supposed to be expanded prior to interpretation.
            throw formula_error(fe_general_error);
        case fop_value:
            constant();
            return;
        case fop_single_ref:
            single_ref();
            return;
        case fop_range_ref:
            range_ref();
            return;
        case fop_function:
            function();
            return;
        case fop_string:
            literal();
            return;
        default:
        {
            ostringstream os;
            os << "factor: unexpected token type: <" << get_opcode_name(oc) << ">";
            throw invalid_expression(os.str());
        }
    }
    m_stack.push_value(0.0);
}

void formula_interpreter::paren()
{
    if (mp_handler)
        mp_handler->push_token(fop_open);

    next();
    expression();
    if (token().get_opcode() != fop_close)
        throw invalid_expression("paren: expected close paren");

    if (mp_handler)
        mp_handler->push_token(fop_close);

    next();
}

void formula_interpreter::single_ref()
{
    address_t addr = token().get_single_ref();
#if DEBUG_FORMULA_INTERPRETER
    __IXION_DEBUG_OUT__ << "formula_interpreter::single_ref: ref=" << addr.get_name() << endl;
    __IXION_DEBUG_OUT__ << "formula_interpreter::single_ref: origin=" << m_pos.get_name() << endl;
#endif
    if (mp_handler)
        mp_handler->push_single_ref(addr, m_pos);

    abs_address_t abs_addr = addr.to_abs(m_pos);
#if DEBUG_FORMULA_INTERPRETER
    __IXION_DEBUG_OUT__ << "formula_interpreter::single_ref: ref=" << abs_addr.get_name() << " (converted to absolute)" << endl;
#endif

    if (abs_addr == m_pos)
    {
        // self-referencing is not permitted.
        throw formula_error(fe_ref_result_not_available);
    }

    m_stack.push_single_ref(abs_addr);
    next();
}

void formula_interpreter::range_ref()
{
    range_t range = token().get_range_ref();
#if DEBUG_FORMULA_INTERPRETER
    __IXION_DEBUG_OUT__ << "formula_interpreter::range_ref: ref=" << range.first.get_name() << ":" << range.last.get_name() << endl;
    __IXION_DEBUG_OUT__ << "formula_interpreter::range_ref: origin=" << m_pos.get_name() << endl;
#endif
    if (mp_handler)
        mp_handler->push_range_ref(range, m_pos);

    abs_range_t abs_range = range.to_abs(m_pos);

#if DEBUG_FORMULA_INTERPRETER
    __IXION_DEBUG_OUT__ << "formula_interpreter::range_ref: ref=" << abs_range.first.get_name() << ":" << abs_range.last.get_name() << " (converted to absolute)" << endl;
#endif

    // Check the reference range to make sure it doesn't include the parent cell.
    if (abs_range.contains(m_pos))
    {
        // Referenced range contains the address of this cell.  Not good.
        throw formula_error(fe_ref_result_not_available);
    }

    m_stack.push_range_ref(abs_range);
    next();
}

void formula_interpreter::constant()
{
    double val = token().get_value();
    next();
    m_stack.push_value(val);
    if (mp_handler)
        mp_handler->push_value(val);
}

void formula_interpreter::literal()
{
    size_t sid = token().get_index();
    next();
    m_stack.push_string(sid);
    if (mp_handler)
        mp_handler->push_string(sid);
}

void formula_interpreter::function()
{
    // <func name> '(' <expression> ',' <expression> ',' ... ',' <expression> ')'
    assert(token().get_opcode() == fop_function);
    assert(m_stack.empty());
    formula_function_t func_oc = formula_functions::get_function_opcode(token());
    if (mp_handler)
        mp_handler->push_function(func_oc);

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
        oc = token().get_opcode();
    }

    if (mp_handler)
        mp_handler->push_token(oc);

    next();

    // Function call pops all stack values pushed onto the stack this far, and
    // pushes the result onto the stack.
    formula_functions(m_context).interpret(func_oc, m_stack);
    assert(m_stack.size() == 1);
}

}
