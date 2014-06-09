/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "model_parser.hpp"

#include "ixion/cell.hpp"
#include "ixion/formula.hpp"
#include "ixion/formula_parser.hpp"
#include "ixion/depends_tracker.hpp"
#include "ixion/formula_interpreter.hpp"
#include "ixion/formula_name_resolver.hpp"
#include "ixion/formula_result.hpp"
#include "ixion/mem_str_buf.hpp"
#include "ixion/cell_listener_tracker.hpp"
#include "ixion/function_objects.hpp"
#include "ixion/macros.hpp"

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

model_parser::model_parser(const string& filepath, size_t thread_count) :
    m_context(),
    mp_name_resolver(formula_name_resolver::get(formula_name_resolver_excel_a1, &m_context)),
    m_filepath(filepath),
    m_thread_count(thread_count),
    m_print_separator(true)
{
    m_context.append_sheet(IXION_ASCII("sheet"), 1048576, 1024);
}

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

    formula_name_type ret = mp_name_resolver->resolve(name.get(), name.size(), abs_address_t());
    if (ret.type != formula_name_type::cell_reference)
    {
        ostringstream os;
        os << "invalid cell name: " << name.str();
        throw model_parser::parse_error(os.str());
    }

    abs_address_t pos(ret.address.sheet, ret.address.row, ret.address.col);
    m_dirty_cell_addrs.push_back(pos);
    unregister_formula_cell(m_context, pos);

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
            unregister_formula_cell(m_context, pos);
            m_context.set_formula_cell(pos, buf.get(), buf.size(), *mp_name_resolver);
            formula_cell* p = m_context.get_formula_cell(pos);
            assert(p);
            m_dirty_cells.insert(pos);
            register_formula_cell(m_context, pos);
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
            m_context.set_string_cell(pos, buf.get(), buf.size());
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
            m_context.set_numeric_cell(pos, value);

            if (m_print_separator)
            {
                m_print_separator = false;
                cout << get_formula_result_output_separator() << endl;
            }
            cout << mp_name_resolver->get_name(pos, false) << ": (n) " << value << endl;
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
    res.parse(m_context, result.get(), result.size());
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
        if (name.empty())
            throw check_error("empty cell name");

        const formula_result& res = itr->second;
        cout << name << " : " << res.str(m_context) << endl;

        formula_name_type name_type = mp_name_resolver->resolve(&name[0], name.size(), abs_address_t());
        switch (name_type.type)
        {
            case formula_name_type::cell_reference:
            {
                const formula_name_type::address_type& _addr = name_type.address;
                abs_address_t addr(_addr.sheet, _addr.row, _addr.col);

                switch (m_context.get_celltype(addr))
                {
                    case celltype_formula:
                    {
                        const formula_cell* fcell = m_context.get_formula_cell(addr);
                        const formula_result* res_cell = fcell->get_result_cache();
                        if (!res_cell)
                            throw check_error("result is not cached");

                        if (*res_cell != res)
                        {
                            ostringstream os;
                            os << "unexpected result: (expected: " << res.str(m_context) << "; actual: " << res_cell->str(m_context) << ")";
                            throw check_error(os.str());
                        }
                    }
                    break;
                    case celltype_numeric:
                    {
                        double actual_val = m_context.get_numeric_value(addr);
                        if (actual_val != res.get_value())
                        {
                            ostringstream os;
                            os << "unexpected numeric result: (expected: " << res.get_value() << "; actual: " << actual_val << ")";
                            throw check_error(os.str());
                        }
                    }
                    break;
                    case celltype_string:
                    {
                        size_t str_id = m_context.get_string_identifier(addr);

                        if (str_id != res.get_string())
                        {
                            const string* ps = m_context.get_string(str_id);
                            if (!ps)
                                throw check_error("failed to retrieve a string value for a string cell.");

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
            break;
            case formula_name_type::named_expression:
            {
                const formula_cell* fcell = m_context.get_named_expression(name);
                const formula_result* res_cell = fcell->get_result_cache();
                if (!res_cell)
                    throw check_error("result is not cached");

                if (*res_cell != res)
                {
                    ostringstream os;
                    os << "unexpected result: (expected: " << res.str(m_context) << "; actual: " << res_cell->str(m_context) << ")";
                    throw check_error(os.str());
                }
            }
            break;
            default:
                ;
        }
    }
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
