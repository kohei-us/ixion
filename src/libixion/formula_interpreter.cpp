/*************************************************************************
 *
 * Copyright (c) 2010 Kohei Yoshida
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
#include "ixion/model_context.hpp"

#include <string>
#include <iostream>
#include <sstream>

#define DEBUG_FORMULA_INTERPRETER 1

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

formula_interpreter::formula_interpreter(const formula_cell* cell, const model_context& cxt) :
    m_parent_cell(cell),
    m_original_tokens(cell->get_tokens()),
    m_context(cxt),
    m_result(0.0),
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
    string cell_name("<unknown cell>");
    const string* p = m_context.get_named_expression_name(m_parent_cell);
    if (p)
        cell_name = *p;
    else
        cell_name = m_context.get_cell_name(m_parent_cell);

    try
    {
        init_tokens();

        if (m_tokens.empty())
        {
            cout << "interpreter error: no tokens to interpret" << endl;
            return false;
        }

        m_cur_token_itr = m_tokens.begin();
        m_error = fe_no_error;
        m_result = 0.0;
        m_outbuf.clear();
        m_outbuf << get_formula_result_output_separator() << endl;
        m_outbuf << cell_name << ": ";

        expression();
        // there should only be one stack value left for the result value.
        assert(m_stack.size() == 1);
        m_result = pop_value();
        m_outbuf << endl << cell_name << ": result = " << m_result << endl;
        cout << m_outbuf.str();
        return true;
    }
    catch (const invalid_expression& e)
    {
        m_outbuf << endl << cell_name << ": invalid expression: " << e.what() << endl;
        cout << m_outbuf.str();
        m_error = fe_invalid_expression;
    }
    catch (const formula_error& e)
    {
        m_outbuf << endl << "result = " << e.what() << endl;
        cout <<  m_outbuf.str();
        m_error = e.get_error();
    }
    return false;
}

double formula_interpreter::get_result() const
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
    formula_tokens_t::const_iterator itr = m_original_tokens.begin(), itr_end = m_original_tokens.end();
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

void formula_interpreter::expand_named_expression(
    const string& expr_name, const formula_cell* expr, name_set& used_names)
{
    if (!expr)
    {
        ostringstream os;
        os << "unable to find named expression '" << expr_name << "'";
        throw invalid_expression(os.str());
    }

    const formula_tokens_t& expr_tokens = expr->get_tokens();
    m_tokens.push_back(&paren_open);
    formula_tokens_t::const_iterator itr = expr_tokens.begin(), itr_end = expr_tokens.end();
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

void formula_interpreter::expression()
{
    // <term> + <term> + <term> + ... + <term>

    term();
    double val = pop_value();
    while (has_token())
    {
        fopcode_t oc = token().get_opcode();
        switch (oc)
        {
            case fop_plus:
            {
                m_outbuf << "+";
                next();
                term();
                val += pop_value();
            }
            break;
            case fop_minus:
            {
                m_outbuf << "-";
                next();
                term();
                val -= pop_value();
            }
            break;
            default:
                push_value(val);
                return;
        }
    }
    push_value(val);
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
            m_outbuf << "*";
            next();
            double val = pop_value();
            term();
            push_value(val*pop_value());
            return;
        }
        case fop_divide:
        {
            m_outbuf << "/";
            next();
            double val = pop_value();
            term();
            double val2 = pop_value();
            if (val2 == 0.0)
                throw formula_error(fe_division_by_zero);
            push_value(val/val2);
            return;
        }
        default:
            ;
    }
}

void formula_interpreter::factor()
{
    // <constant> || <variable> || <named expression> || '(' <expression> ')' || <function>

    fopcode_t oc = token().get_opcode();
    switch (oc)
    {
        case fop_open:
            paren();
            return;
        case fop_named_expression:
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
        default:
        {
            ostringstream os;
            os << "factor: unexpected token type: <" << get_opcode_name(oc) << ">";
            throw invalid_expression(os.str());
        }
    }
    push_value(0.0);
}

void formula_interpreter::paren()
{
    m_outbuf << "(";
    next();
    expression();
    if (token().get_opcode() != fop_close)
        throw invalid_expression("paren: expected close paren");

    m_outbuf << ")";
    next();
}

void formula_interpreter::single_ref()
{
    address_t addr = token().get_single_ref();
#if DEBUG_FORMULA_INTERPRETER
    cout << "formula_interpreter::variable: ref=" << addr.get_name() << endl;
    cout << "formula_interpreter::variable: origin=" << m_pos.get_name() << endl;
#endif
    abs_address_t abs_addr = addr.to_abs(m_pos);
#if DEBUG_FORMULA_INTERPRETER
    cout << "formula_interpreter::variable: ref=" << abs_addr.get_name() << " (converted to absolute)" << endl;
#endif
    const base_cell* pref = m_context.get_cell(abs_addr);

    if (pref == m_parent_cell)
    {
        // self-referencing is not permitted.
        throw formula_error(fe_ref_result_not_available);
    }

    double val = 0.0;
    if (pref)
        val = pref->get_value();
    m_outbuf << m_context.get_name_resolver().get_name(addr);
    next();

    push_value(val);
}

void formula_interpreter::range_ref()
{
    // TODO: Properly implement this.
    push_value(-1);
    next();
}

void formula_interpreter::constant()
{
    double val = token().get_value();
    m_outbuf << val;
    next();

    push_value(val);
}

void formula_interpreter::function()
{
    // <func name> '(' <expression> ',' <expression> ',' ... ',' <expression> ')'
    assert(token().get_opcode() == fop_function);
    formula_function_t func_oc = formula_functions::get_function_opcode(token());
    m_outbuf << formula_functions::get_function_name(func_oc);

    if (next_token().get_opcode() != fop_open)
        throw invalid_expression("expecting a '(' after a function name.");

    m_outbuf << "(";

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
            m_outbuf << ",";
        }
        else
        {
            expression();
            expect_sep = true;
        }
        oc = token().get_opcode();
    }

    m_outbuf << ")";
    next();

    // Function call uses all stack values pushed onto the stack so far, which
    // gets cleared after the call returns.
    double val = formula_functions::interpret(func_oc, m_stack);
    m_stack.clear();
    push_value(val);
}

void formula_interpreter::push_value(double val)
{
    m_stack.push_back(new stack_value(val));
}

double formula_interpreter::pop_value()
{
    double ret = 0.0;
    if (m_stack.empty())
        throw formula_error(fe_stack_error);

    const stack_value& v = m_stack.back();
    if (v.get_type() == sv_value)
        ret = v.get_value();

    m_stack.pop_back();
    return ret;
}

}
