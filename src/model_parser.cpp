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

#include "model_parser.hpp"

#include "ixion/cell.hpp"
#include "ixion/formula.hpp"
#include "ixion/formula_lexer.hpp"
#include "ixion/formula_parser.hpp"
#include "ixion/depends_tracker.hpp"
#include "ixion/formula_interpreter.hpp"
#include "ixion/formula_name_resolver.hpp"
#include "ixion/formula_result.hpp"
#include "ixion/mem_str_buf.hpp"
#include "ixion/cell_listener_tracker.hpp"
#include "ixion/function_objects.hpp"

#include <sstream>
#include <iostream>
#include <vector>
#include <functional>
#include <cstring>
#include <cassert>
#include <memory>

using namespace std;

#define DEBUG_MODEL_PARSER 0

namespace ixion {

namespace {

enum parse_mode_t
{
    parse_mode_unknown = 0,
    parse_mode_init,
    parse_mode_result,
    parse_mode_edit
};

bool is_separator(char c)
{
    return c == '=' || c == ':' || c == '@';
}

void parse_command(const char*& p, mem_str_buf& com)
{
    mem_str_buf _com;
    ++p; // skip '%'.
    _com.set_start(p);
    if (*p == '%')
    {
        // This line is a comment.  Skip the rest of the line.
        _com.swap(com);
        while (*p != '\n') ++p;
        return;
    }
    for (++p; *p != '\n'; ++p)
        _com.inc();
    _com.swap(com);
}

void unregister_existing_formula_cell(model_context& cxt, const abs_address_t& pos)
{
    // When there is a formula cell at this position, unregister it from
    // the dependency tree.
    base_cell* p = cxt.get_cell(pos);
    if (!p || p->get_celltype() != celltype_formula)
        // Not a formula cell. Bail out.
        return;

    formula_cell* fcell = static_cast<formula_cell*>(p);

    // Go through all its existing references, and remove
    // itself as their listener.  This step is important
    // especially during partial re-calculation.
    vector<const formula_token_base*> ref_tokens;
    fcell->get_ref_tokens(cxt, ref_tokens);
    for_each(ref_tokens.begin(), ref_tokens.end(),
             formula_cell_listener_handler(cxt,
                 pos, formula_cell_listener_handler::mode_remove));
}

/**
 * @return true if the formula cell is stored in the model with a shared
 *         formula token set, false if the formula cell has a non-shared
 *         formula token set, and is not yet stored in the model.
 */
bool set_shared_formula_tokens_to_cell(
    model_context& cxt, const abs_address_t& addr, formula_cell& fcell, const formula_tokens_t& new_tokens)
{
    if (addr.sheet == global_scope)
        return false;

    // Check its neighbors for adjacent formula cells.
    if (addr.row == 0)
        return false;

    abs_address_t test = addr;
    test.row -= 1;
    base_cell* p = cxt.get_cell(test);
    if (!p || p->get_celltype() != celltype_formula)
        // The neighboring cell is not a formula cell.
        return false;

    formula_cell* test_cell = static_cast<formula_cell*>(p);

    if (test_cell->is_shared())
    {
        size_t token_id = test_cell->get_identifier();
        const formula_tokens_t* tokens = cxt.get_shared_formula_tokens(addr.sheet, token_id);
        assert(tokens);

        if (new_tokens != *tokens)
            return false;

        // Make sure that we can extend the shared range properly.
        abs_range_t range = cxt.get_shared_formula_range(addr.sheet, token_id);
        if (range.first.sheet != addr.sheet)
            // Wrong sheet
            return false;

        if (range.first.column != range.last.column)
            // Must be a single column.
            return false;

        if (range.last.row != (addr.row - 1))
            // Last row is not immediately above the current cell.
            return false;

        fcell.set_identifier(token_id);
        fcell.set_shared(true);

        range.last.row += 1;
        cxt.set_shared_formula_range(addr.sheet, token_id, range);
    }
    else
    {
        size_t token_id = test_cell->get_identifier();
        const formula_tokens_t* tokens = cxt.get_formula_tokens(addr.sheet, token_id);
        assert(tokens);

        if (new_tokens != *tokens)
            return false;

        // Move the tokens of the master cell to the shared token storage.
        size_t shared_id = cxt.set_formula_tokens_shared(addr.sheet, token_id);
        test_cell->set_shared(true);
        test_cell->set_identifier(shared_id);
        assert(test_cell->is_shared());
        fcell.set_identifier(shared_id);
        fcell.set_shared(true);
        assert(fcell.is_shared());
        abs_range_t range;
        range.first = addr;
        range.last = addr;
        range.first.row -= 1;
        cxt.set_shared_formula_range(addr.sheet, shared_id, range);
    }
    return true;
}

}

// ============================================================================

model_parser::parse_error::parse_error(const string& msg)
{
    ostringstream os;
    os << "parse error: " << msg;
    m_msg = os.str();
}

model_parser::parse_error::~parse_error() throw()
{
}

const char* model_parser::parse_error::what() const throw()
{
    return m_msg.c_str();
}

// ============================================================================

model_parser::check_error::check_error(const string& msg) :
    general_error(msg) {}

// ============================================================================

model_parser::cell::cell(const mem_str_buf& name, cell_type type, lexer_tokens_t& tokens) :
    m_name(name), m_type(type)
{
    // Note that this will empty the passed token container !
    m_tokens.swap(tokens);
}

model_parser::cell::cell(const model_parser::cell& r) :
    m_name(r.m_name),
    m_type(r.m_type),
    m_tokens(r.m_tokens) {}

model_parser::cell::~cell() {}

string model_parser::cell::print() const
{
    return print_tokens(m_tokens, false);
}

const mem_str_buf& model_parser::cell::get_name() const
{
    return m_name;
}

model_parser::cell_type model_parser::cell::get_type() const
{
    return m_type;
}

const lexer_tokens_t& model_parser::cell::get_tokens() const
{
    return m_tokens;
}

// ============================================================================

model_parser::model_parser(const string& filepath, size_t thread_count) :
    m_filepath(filepath), m_thread_count(thread_count), m_print_separator(true) {}

model_parser::~model_parser() {}

void model_parser::parse()
{
    string strm;
    global::load_file_content(m_filepath, strm);

    parse_mode_t parse_mode = parse_mode_unknown;
    const char *p = &strm[0], *p_last = &strm[strm.size()-1];

    for (; p != p_last; ++p)
    {
        // In each iteration, the p always points to the 1st character of a
        // line.
        if (*p == '%')
        {
            // This line contains a command.
            mem_str_buf buf_com;
            parse_command(p, buf_com);
            if (buf_com.equals("%"))
            {
                // This is a comment line.  Just ignore it.
            }
            else if (buf_com.equals("calc"))
            {
                if (parse_mode != parse_mode_init)
                    throw parse_error("'calc' command must be used in the init mode.");

                // Perform full calculation on currently stored cells.
                calculate_cells(m_context, m_dirty_cells, m_thread_count);
            }
            else if (buf_com.equals("recalc"))
            {
                if (parse_mode != parse_mode_edit)
                    throw parse_error("'recalc' command must be used in the edit mode.");

                cout << get_formula_result_output_separator() << endl
                    << "recalculating" << endl;

                get_all_dirty_cells(m_context, m_dirty_cell_addrs, m_dirty_cells);
                calculate_cells(m_context, m_dirty_cells, m_thread_count);
            }
            else if (buf_com.equals("check"))
            {
                // Check cell results.
                check();
            }
            else if (buf_com.equals("exit"))
            {
                // Exit the loop.
                return;
            }
            else if (buf_com.equals("mode init"))
            {
                parse_mode = parse_mode_init;
                m_print_separator = true;
            }
            else if (buf_com.equals("mode result"))
            {
                // Clear any previous result values.
                m_formula_results.clear();
                parse_mode = parse_mode_result;
            }
            else if (buf_com.equals("mode edit"))
            {
                parse_mode = parse_mode_edit;
                m_dirty_cells.clear();
                m_dirty_cell_addrs.clear();
                m_print_separator = true;
            }
            else
            {
                ostringstream os;
                os << "unknown command: " << buf_com.str() << endl;
                throw parse_error(os.str());
            }
            continue;
        }

        switch (parse_mode)
        {
            case parse_mode_init:
                parse_init(p);
            break;
            case parse_mode_edit:
                parse_init(p);
            break;
            case parse_mode_result:
                parse_result(p);
            break;
            default:
                throw parse_error("unknown parse mode");
        }
    }
}

void model_parser::parse_init(const char*& p)
{
    model_parser::cell_type content_type = model_parser::ct_unknown;
    mem_str_buf buf, name, formula;
    for (; *p != '\n'; ++p)
    {
        if (name.empty() && is_separator(*p))
        {
            // Separator encountered.  Set the name and clear the buffer.
            if (buf.empty())
                throw model_parser::parse_error("left hand side is empty");

            name = buf;
            buf.clear();
            switch (*p)
            {
                case '=':
                    content_type = model_parser::ct_formula;
                break;
                case ':':
                    content_type = model_parser::ct_value;
                break;
                case '@':
                    content_type = model_parser::ct_string;
                break;
                default:
                    ;
            }
        }
        else
        {
            if (buf.empty())
                buf.set_start(p);
            else
                buf.inc();
        }
    }

#if DEBUG_MODEL_PARSER
    __IXION_DEBUG_OUT__ << "name: " << name.str() << "  buf: " << buf.str() << endl;
#endif

    if (name.empty())
    {
        if (buf.empty())
            // This is an empty line. Bail out.
            return;

        // Buffer is not empty but name is not given.  We must be missing a separator.
        throw model_parser::parse_error("separator is missing");
    }

    const formula_name_resolver& resolver = m_context.get_name_resolver();
    formula_name_type ret = resolver.resolve(name.get(), name.size(), abs_address_t());
    if (ret.type != formula_name_type::cell_reference)
    {
        ostringstream os;
        os << "invalid cell name: " << name.str();
        throw model_parser::parse_error(os.str());
    }

    abs_address_t pos(ret.address.sheet, ret.address.row, ret.address.col);
    m_dirty_cell_addrs.push_back(pos);
    unregister_existing_formula_cell(m_context, pos);

    if (buf.empty())
    {
        // A valid name is given but with empty definition.  Just remove the
        // existing cell.
        m_context.erase_cell(pos);
        return;
    }

    switch (content_type)
    {
        case ct_formula:
        {
#if DEBUG_MODEL_PARSER
            __IXION_DEBUG_OUT__ << "pos: " << resolver.get_name(pos, false) << " type: formula" << endl;
#endif
            unique_ptr<formula_tokens_t> tokens(new formula_tokens_t);
            parse_formula_string(m_context, pos, buf.get(), buf.size(), *tokens);
            unique_ptr<formula_cell> fcell(new formula_cell);
            if (!set_shared_formula_tokens_to_cell(m_context, pos, *fcell, *tokens))
            {
                size_t tkid = m_context.add_formula_tokens(0, tokens.release());
                fcell->set_identifier(tkid);
            }
            formula_cell* p = fcell.get();
            m_context.set_cell(pos, fcell.release());
            m_dirty_cells.insert(p);
            register_formula_cell(m_context, pos, p);
#if DEBUG_MODEL_PARSER
            std::string s;
            print_formula_tokens(m_context, pos, *tokens, s);
            __IXION_DEBUG_OUT__ << "formula tokens: " << s << endl;
#endif
        }
        break;
        case ct_string:
        {
#if DEBUG_MODEL_PARSER
            __IXION_DEBUG_OUT__ << "pos: " << resolver.get_name(pos, false) << " type: string" << endl;
#endif
            size_t str_id = m_context.add_string(buf.get(), buf.size());
            m_context.set_cell(pos, new string_cell(str_id));
            if (m_print_separator)
            {
                m_print_separator = false;
                cout << get_formula_result_output_separator() << endl;
            }
            cout << name.str() << ": (s) " << buf.str() << endl;
        }
        break;
        case ct_value:
        {
#if DEBUG_MODEL_PARSER
            __IXION_DEBUG_OUT__ << "pos: " << resolver.get_name(pos, false) << " type: numeric" << endl;
#endif
            double value = global::to_double(buf.get(), buf.size());
            m_context.set_cell(pos, new numeric_cell(value));

            if (m_print_separator)
            {
                m_print_separator = false;
                cout << get_formula_result_output_separator() << endl;
            }
            cout << resolver.get_name(pos, false) << ": (n) " << value << endl;
        }
        break;
        default:
            throw model_parser::parse_error("unknown content type");
    }
}

void model_parser::parse_result(const char*& p)
{
    mem_str_buf buf, name, result;
    for (; *p != '\n'; ++p)
    {
        if (*p == '=')
        {
            if (buf.empty())
                throw model_parser::parse_error("left hand side is empty");

            name = buf;
            buf.clear();
        }
        else
        {
            if (buf.empty())
                buf.set_start(p);
            else
                buf.inc();
        }
    }

    if (!buf.empty())
    {
        if (name.empty())
            throw model_parser::parse_error("'=' is missing");

        result = buf;
    }

    string name_s = name.str();
    formula_result res;
    res.parse(result.get(), result.size());
    model_parser::results_type::iterator itr = m_formula_results.find(name_s);
    if (itr == m_formula_results.end())
    {
        // This cell doesn't exist yet.
        pair<model_parser::results_type::iterator, bool> r =
            m_formula_results.insert(model_parser::results_type::value_type(name_s, res));
        if (!r.second)
            throw model_parser::parse_error("failed to insert a new result.");
    }
    else
        itr->second = res;
}

void model_parser::check()
{
    cout << get_formula_result_output_separator() << endl
         << "checking results" << endl
         << get_formula_result_output_separator() << endl;

    results_type::const_iterator itr = m_formula_results.begin(), itr_end = m_formula_results.end();
    for (; itr != itr_end; ++itr)
    {
        const string& name = itr->first;
        const formula_result& res = itr->second;
        cout << name << " : " << res.str() << endl;
        // resolve name and get cell instance from the context.  The cell may
        // be either a real cell, or a named expression.
        const base_cell* pcell = get_cell_from_name(name);
        if (!pcell)
        {
            ostringstream os;
            os << "specified cell instance not found: " << name;
            throw check_error(os.str());
        }

        switch (pcell->get_celltype())
        {
            case celltype_formula:
            {
                const formula_result* res_cell = static_cast<const formula_cell*>(pcell)->get_result_cache();
                if (!res_cell)
                    throw check_error("result is not cached");

                if (*res_cell != res)
                {
                    ostringstream os;
                    os << "unexpected result: (expected: " << res.str() << "; actual: " << res_cell->str() << ")";
                    throw check_error(os.str());
                }
            }
            break;
            case celltype_numeric:
            {
                if (pcell->get_value() != res.get_value())
                {
                    ostringstream os;
                    os << "unexpected numeric result: (expected: " << res.get_value() << "; actual: " << pcell->get_value() << ")";
                    throw check_error(os.str());
                }
            }
            break;
            case celltype_string:
            {
                size_t str_id = pcell->get_identifier();
                const string* ps = m_context.get_string(str_id);
                if (!ps)
                    throw check_error("failed to retrieve a string value for a string cell.");

                if (*ps != res.get_string())
                {
                    ostringstream os;
                    os << "unexpected string result: (expected: " << res.get_string() << "; actual: " << *ps << ")";
                    throw check_error(os.str());
                }
            }
            break;
            default:
                throw check_error("unhandled cell type.");
        }
    }
}

const base_cell* model_parser::get_cell_from_name(const string& name)
{
    if (name.empty())
        return NULL;

    const formula_name_resolver& resolver = m_context.get_name_resolver();
    formula_name_type name_type = resolver.resolve(&name[0], name.size(), abs_address_t());
    switch (name_type.type)
    {
        case formula_name_type::cell_reference:
        {
            const formula_name_type::address_type& addr = name_type.address;
            return m_context.get_cell(abs_address_t(addr.sheet, addr.row, addr.col));
        }
        case formula_name_type::named_expression:
            return m_context.get_named_expression(name);
        default:
            ;
    }
    return NULL;
}

}

