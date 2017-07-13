/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "model_parser.hpp"

#include "ixion/formula.hpp"
#include "ixion/formula_name_resolver.hpp"
#include "ixion/formula_result.hpp"
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

long to_long(const mem_str_buf& value)
{
    char* pe = nullptr;
    long ret = std::strtol(value.get(), &pe, 10);

    if (value.get() == pe)
    {
        ostringstream os;
        os << "'" << value << "' is not a valid integer.";
        throw model_parser::parse_error(os.str());
    }

    return ret;
}

bool to_bool(const mem_str_buf& value)
{
    return value == "true";
}

bool is_separator(char c)
{
    switch (c)
    {
        case '=':
        case ':':
        case '@':
            return true;
        default:
            ;
    }

    return false;
}

mem_str_buf parse_command_to_buffer(const char*& p, const char* p_end)
{
    mem_str_buf buf;
    ++p; // skip '%'.
    buf.set_start(p);
    if (*p == '%')
    {
        // This line is a comment.  Skip the rest of the line.
        while (p != p_end && *p != '\n') ++p;
        return buf;
    }

    for (++p; p != p_end && *p != '\n'; ++p)
        buf.inc();

    return buf;
}

class string_printer : public std::unary_function<string_id_t, void>
{
    const model_context& m_cxt;
    char m_sep;
    bool m_first;

public:
    string_printer(const model_context& cxt, char sep) :
        m_cxt(cxt), m_sep(sep), m_first(true) {}

    void operator() (string_id_t sid)
    {
        if (m_first)
            m_first = false;
        else
            cout << m_sep;

        const std::string* p = m_cxt.get_string(sid);
        if (p)
            cout << *p;
    }
};

}

// ============================================================================

model_parser::parse_error::parse_error(const string& msg) : general_error()
{
    ostringstream os;
    os << "parse error: " << msg;
    set_message(os.str());
}

// ============================================================================

model_parser::check_error::check_error(const string& msg) :
    general_error(msg) {}

// ============================================================================

model_parser::model_parser(const string& filepath, size_t thread_count) :
    m_context(),
    m_table_handler(),
    m_session_handler_factory(m_context),
    mp_table_entry(nullptr),
    mp_name_resolver(formula_name_resolver::get(formula_name_resolver_t::excel_a1, &m_context)),
    m_filepath(filepath),
    m_thread_count(thread_count),
    mp_head(nullptr),
    mp_end(nullptr),
    mp_char(nullptr),
    m_row_limit(1048576),
    m_col_limit(1024),
    m_current_sheet(0),
    m_parse_mode(parse_mode_unknown),
    m_print_separator(false),
    m_print_sheet_name(false)
{
    m_context.set_session_handler_factory(&m_session_handler_factory);
    m_context.set_table_handler(&m_table_handler);

    global::load_file_content(m_filepath, m_strm);

    mp_head = m_strm.data();
    mp_end = mp_head + m_strm.size();
}

model_parser::~model_parser() {}

void model_parser::parse()
{
    mp_char = mp_head;
    m_parse_mode = parse_mode_unknown;

    for (; mp_char != mp_end; ++mp_char)
    {
        // In each iteration, the p always points to the 1st character of a
        // line.
        if (*mp_char== '%')
        {
            parse_command();
            if (m_parse_mode == parse_mode_exit)
                return;
            continue;
        }

        if (m_print_separator)
        {
            m_print_separator = false;
            cout << get_formula_result_output_separator() << endl;
        }

        switch (m_parse_mode)
        {
            case parse_mode_init:
                parse_init();
                break;
            case parse_mode_edit:
                parse_edit();
                break;
            case parse_mode_result:
                parse_result();
                break;
            case parse_mode_table:
                parse_table();
                break;
            case parse_mode_session:
                parse_session();
                break;
            case parse_mode_named_expression:
                parse_named_expression();
                break;
            default:
                throw parse_error("unknown parse mode");
        }
    }
}

void model_parser::init_model()
{
    if (m_context.empty())
        m_context.append_sheet(IXION_ASCII("sheet"), m_row_limit, m_col_limit);
}

void model_parser::parse_command()
{
    // This line contains a command.
    mem_str_buf buf_com = parse_command_to_buffer(mp_char, mp_end);

    if (buf_com.equals("%"))
    {
        // This is a comment line.  Just ignore it.
    }
    else if (buf_com.equals("calc"))
    {
        // Perform full calculation on currently stored cells.

        for (const abs_address_t& pos : m_dirty_cells)
            register_formula_cell(m_context, pos);

        calculate_cells(m_context, m_dirty_cells, m_thread_count);
    }
    else if (buf_com.equals("recalc"))
    {
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
        m_parse_mode = parse_mode_exit;
        return;
    }
    else if (buf_com.equals("push"))
    {
        switch (m_parse_mode)
        {
            case parse_mode_table:
                push_table();
                break;
            case parse_mode_named_expression:
                push_named_expression();
                break;
            default:
                throw parse_error("push command was used for wrong mode!");
        }
    }
    else if (buf_com.equals("mode init"))
    {
        cout << get_formula_result_output_separator() << endl
            << "initializing" << endl;

        m_parse_mode = parse_mode_init;
        m_print_separator = true;
    }
    else if (buf_com.equals("mode result"))
    {
        // Clear any previous result values.
        m_formula_results.clear();
        m_parse_mode = parse_mode_result;
    }
    else if (buf_com.equals("mode edit"))
    {
        cout << get_formula_result_output_separator() << endl
            << "editing" << endl;

        m_parse_mode = parse_mode_edit;
        m_dirty_cells.clear();
        m_dirty_cell_addrs.clear();
        m_print_separator = true;
    }
    else if (buf_com.equals("mode table"))
    {
        m_parse_mode = parse_mode_table;
        mp_table_entry.reset(new table_handler::entry);
    }
    else if (buf_com.equals("mode session"))
    {
        cout << get_formula_result_output_separator() << endl
            << "session" << endl;

        m_print_separator = true;
        m_parse_mode = parse_mode_session;
    }
    else if (buf_com.equals("mode named-expression"))
    {
        m_print_separator = true;
        m_parse_mode = parse_mode_named_expression;
        mp_named_expression = ixion::make_unique<named_expression_type>();
    }
    else
    {
        ostringstream os;
        os << "unknown command: " << buf_com.str() << endl;
        throw parse_error(os.str());
    }
}

void model_parser::parse_session()
{
    mem_str_buf cmd, value;
    mem_str_buf* buf = &cmd;

    for (; mp_char != mp_end && *mp_char != '\n'; ++mp_char)
    {
        if (*mp_char == ':')
        {
            if (buf == &value)
                throw parse_error("2nd ':' character is illegal.");

            buf = &value;
            continue;
        }

        if (buf->empty())
            buf->set_start(mp_char);
        else
            buf->inc();
    }

    if (cmd == "row-limit")
        m_row_limit = to_long(value);
    else if (cmd == "column-limit")
        m_col_limit = to_long(value);
    else if (cmd == "insert-sheet")
    {
        m_context.append_sheet(value.get(), value.size(), m_row_limit, m_col_limit);
        cout << "sheet: (name: " << value << ", rows: " << m_row_limit << ", columns: " << m_col_limit << ")" << endl;
    }
    else if (cmd == "current-sheet")
    {
        m_current_sheet = m_context.get_sheet_index(value.get(), value.size());

        if (m_current_sheet == invalid_sheet)
        {
            ostringstream os;
            os << "No sheet named '" << value << "' found.";
            throw parse_error(os.str());
        }

        cout << "current sheet: " << value << endl;
    }
    else if (cmd == "display-sheet-name")
    {
        cout << "display sheet name: " << value << endl;
        m_print_sheet_name = to_bool(value);
        m_session_handler_factory.show_sheet_name(m_print_sheet_name);
    }
}

void model_parser::parse_init()
{
    init_model();

    cell_def_type cell_def = parse_cell_definition();
    if (cell_def.name.empty() && cell_def.value.empty())
        return;

    m_dirty_cell_addrs.push_back(cell_def.pos);

    switch (cell_def.type)
    {
        case ct_formula:
        {
            m_context.set_formula_cell(cell_def.pos, cell_def.value.get(), cell_def.value.size(), *mp_name_resolver);
            m_dirty_cells.insert(cell_def.pos);

            address_t pos_display(cell_def.pos);
            pos_display.set_absolute(false);
            cout << mp_name_resolver->get_name(pos_display, abs_address_t(), m_print_sheet_name)
                << ": (f) " << cell_def.value.str() << endl;
            break;
        }
        case ct_string:
        {
            m_context.set_string_cell(cell_def.pos, cell_def.value.get(), cell_def.value.size());

            address_t pos_display(cell_def.pos);
            pos_display.set_absolute(false);
            cout << mp_name_resolver->get_name(pos_display, abs_address_t(), m_print_sheet_name)
                << ": (s) " << cell_def.value.str() << endl;
            break;
        }
        case ct_value:
        {
            double v = global::to_double(cell_def.value.get(), cell_def.value.size());
            m_context.set_numeric_cell(cell_def.pos, v);

            address_t pos_display(cell_def.pos);
            pos_display.set_absolute(false);
            cout << mp_name_resolver->get_name(pos_display, abs_address_t(), m_print_sheet_name)
                << ": (n) " << v << endl;
            break;
        }
        case ct_boolean:
        {
            bool b = global::to_bool(cell_def.value.get(), cell_def.value.size());
            m_context.set_boolean_cell(cell_def.pos, b);

            address_t pos_display(cell_def.pos);
            pos_display.set_absolute(false);
            cout << mp_name_resolver->get_name(pos_display, abs_address_t(), m_print_sheet_name)
                << ": (b) " << (b ? "true" : "false") << endl;
            break;
        }
        default:
            throw model_parser::parse_error("unknown content type");
    }
}

void model_parser::parse_edit()
{
    cell_def_type cell_def = parse_cell_definition();
    if (cell_def.name.empty() && cell_def.value.empty())
        return;

    m_dirty_cell_addrs.push_back(cell_def.pos);
    unregister_formula_cell(m_context, cell_def.pos);

    if (cell_def.value.empty())
    {
        // A valid name is given but with empty definition.  Just remove the
        // existing cell.
        m_context.erase_cell(cell_def.pos);
        return;
    }

    switch (cell_def.type)
    {
        case ct_formula:
        {
#if DEBUG_MODEL_PARSER
            __IXION_DEBUG_OUT__ << "pos: " << resolver.get_name(cell_def.pos, false) << " type: formula" << endl;
#endif
            unregister_formula_cell(m_context, cell_def.pos);
            m_context.set_formula_cell(cell_def.pos, cell_def.value.get(), cell_def.value.size(), *mp_name_resolver);
            m_dirty_cells.insert(cell_def.pos);
            register_formula_cell(m_context, cell_def.pos);
            cout << cell_def.name.str() << ": (f) " << cell_def.value.str() << endl;
#if DEBUG_MODEL_PARSER
            std::string s = print_formula_tokens(m_context, cell_def.pos, *tokens);
            __IXION_DEBUG_OUT__ << "formula tokens: " << s << endl;
#endif
        }
        break;
        case ct_string:
        {
#if DEBUG_MODEL_PARSER
            __IXION_DEBUG_OUT__ << "pos: " << resolver.get_name(cell_def.pos, false) << " type: string" << endl;
#endif
            m_context.set_string_cell(cell_def.pos, cell_def.value.get(), cell_def.value.size());
            cout << cell_def.name.str() << ": (s) " << cell_def.value.str() << endl;
        }
        break;
        case ct_value:
        {
#if DEBUG_MODEL_PARSER
            __IXION_DEBUG_OUT__ << "pos: " << resolver.get_name(cell_def.pos, false) << " type: numeric" << endl;
#endif
            double v = global::to_double(cell_def.value.get(), cell_def.value.size());
            m_context.set_numeric_cell(cell_def.pos, v);

            address_t pos_display(cell_def.pos);
            pos_display.set_absolute(false);
            cout << mp_name_resolver->get_name(pos_display, abs_address_t(), false) << ": (n) " << v << endl;
        }
        break;
        default:
            throw model_parser::parse_error("unknown content type");
    }
}

void model_parser::parse_result()
{
    parsed_assignment_type res = parse_assignment();

    string name_s = res.first.str();

    formula_result fres;
    fres.parse(m_context, res.second.get(), res.second.size());
    model_parser::results_type::iterator itr = m_formula_results.find(name_s);
    if (itr == m_formula_results.end())
    {
        // This cell doesn't exist yet.
        pair<model_parser::results_type::iterator, bool> r =
            m_formula_results.insert(model_parser::results_type::value_type(name_s, fres));
        if (!r.second)
            throw model_parser::parse_error("failed to insert a new result.");
    }
    else
        itr->second = fres;
}

void model_parser::parse_table()
{
    assert(mp_table_entry);

    // In table mode, each line must be attribute=value.
    parsed_assignment_type res = parse_assignment();
    const mem_str_buf& name = res.first;
    const mem_str_buf& value = res.second;

    table_handler::entry& entry = *mp_table_entry;

    if (name == "name")
        entry.name = m_context.add_string(value.get(), value.size());
    else if (name == "range")
    {
        if (!mp_name_resolver)
            return;

        abs_address_t pos(m_current_sheet,0,0);
        formula_name_t ret = mp_name_resolver->resolve(value.get(), value.size(), pos);
        if (ret.type != formula_name_t::range_reference)
            throw parse_error("range of a table is expected to be given as a range reference.");

        entry.range = to_range(ret.range).to_abs(pos);
    }
    else if (name == "columns")
        parse_table_columns(value);
    else if (name == "totals-row-count")
        entry.totals_row_count = global::to_double(value.get(), value.size());
}

void model_parser::push_table()
{
    cout << get_formula_result_output_separator() << endl;

    if (!mp_table_entry)
        return;

    table_handler::entry& entry = *mp_table_entry;

    const string* ps = m_context.get_string(entry.name);
    if (ps)
        cout << "name: " << *ps << endl;

    if (mp_name_resolver)
        cout << "range: " << mp_name_resolver->get_name(entry.range, abs_address_t(m_current_sheet,0,0), false) << endl;

    cout << "columns: ";
    std::for_each(entry.columns.begin(), entry.columns.end(), string_printer(m_context, ','));
    cout << endl;

    cout << "totals row count: " << mp_table_entry->totals_row_count << endl;
    m_table_handler.insert(mp_table_entry);
    assert(!mp_table_entry);
}

void model_parser::parse_named_expression()
{
    assert(mp_named_expression);

    parsed_assignment_type res = parse_assignment();
    if (res.first == "name")
        mp_named_expression->name = std::move(res.second.str());
    else if (res.first == "expression")
        mp_named_expression->expression = std::move(res.second.str());
    else if (res.first == "origin")
    {
        const mem_str_buf& s = res.second;

        formula_name_t name =
            mp_name_resolver->resolve(
                s.get(), s.size(), abs_address_t(m_current_sheet,0,0));

        if (name.type != formula_name_t::name_type::cell_reference)
        {
            ostringstream os;
            os << "'" << s << "' is not a valid named expression origin.";
            throw parse_error(os.str());
        }

        mp_named_expression->origin = to_address(name.address).to_abs(abs_address_t(m_current_sheet,0,0));
    }
    else if (res.first == "scope")
    {
        // Resolve it as a sheet name and store the sheet index if found.
        const mem_str_buf& s = res.second;
        mp_named_expression->scope = m_context.get_sheet_index(s.get(), s.size());
        if (mp_named_expression->scope == invalid_sheet)
        {
            ostringstream os;
            os << "no sheet named '" << s << "' exists in the model.";
            throw parse_error(os.str());
        }
    }
    else
    {
        ostringstream os;
        os << "unknown property of named expression '" << res.first << "'";
        throw parse_error(os.str());
    }
}

void model_parser::push_named_expression()
{
    assert(mp_named_expression);

    std::unique_ptr<formula_tokens_t> tokens =
        ixion::make_unique<formula_tokens_t>(
            parse_formula_string(
                m_context, mp_named_expression->origin, *mp_name_resolver,
                mp_named_expression->expression.data(),
                mp_named_expression->expression.size()));

    std::string exp_s = print_formula_tokens(
        m_context, mp_named_expression->origin, *mp_name_resolver, *tokens);

    cout << "name: " << mp_named_expression->name << endl;
    cout << "expression: " << exp_s << endl;
    cout << "origin: " << mp_named_expression->origin << endl;

    cout << "scope: ";

    if (mp_named_expression->scope == global_scope)
        cout << "(global)";
    else
    {
        std::string sheet_name =
            m_context.get_sheet_name(mp_named_expression->scope);

        if (sheet_name.empty())
        {
            ostringstream os;
            os << "no sheet exists with a sheet index of " << mp_named_expression->scope;
            throw std::runtime_error(os.str());
        }

        cout << sheet_name;
    }

    cout << endl;

    if (mp_named_expression->scope == global_scope)
    {
        m_context.set_named_expression(
            mp_named_expression->name.data(), mp_named_expression->name.size(), std::move(tokens));
    }
    else
    {
        m_context.set_named_expression(
            mp_named_expression->scope,
            mp_named_expression->name.data(), mp_named_expression->name.size(), std::move(tokens));
    }

    mp_named_expression.reset();
}

void model_parser::parse_table_columns(const mem_str_buf& str)
{
    assert(mp_table_entry);
    table_handler::entry& entry = *mp_table_entry;

    const char* p = str.get();
    const char* pend = p + str.size();
    mem_str_buf buf;
    for (; p != pend; ++p)
    {
        if (*p == ',')
        {
            // Flush the current column name buffer.
            string_id_t col_name = empty_string_id;
            if (!buf.empty())
                col_name = m_context.add_string(buf.get(), buf.size());

            entry.columns.push_back(col_name);
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

    string_id_t col_name = empty_string_id;
    if (!buf.empty())
        col_name = m_context.add_string(buf.get(), buf.size());

    entry.columns.push_back(col_name);
}

model_parser::parsed_assignment_type model_parser::parse_assignment()
{
    // Parse to get name and value strings.
    parsed_assignment_type res;
    mem_str_buf buf;

    for (; mp_char != mp_end && *mp_char != '\n'; ++mp_char)
    {
        if (*mp_char == '=')
        {
            if (buf.empty())
                throw model_parser::parse_error("left hand side is empty");

            res.first = buf;
            buf.clear();
        }
        else
        {
            if (buf.empty())
                buf.set_start(mp_char);
            else
                buf.inc();
        }
    }

    if (!buf.empty())
    {
        if (res.first.empty())
            throw model_parser::parse_error("'=' is missing");

        res.second = buf;
    }

    return res;
}

model_parser::cell_def_type model_parser::parse_cell_definition()
{
    cell_def_type ret;
    ret.type = model_parser::ct_unknown;

    mem_str_buf buf;

    for (; mp_char != mp_end  && *mp_char != '\n'; ++mp_char)
    {
        if (ret.name.empty() && is_separator(*mp_char))
        {
            // Separator encountered.  Set the name and clear the buffer.
            if (buf.empty())
                throw model_parser::parse_error("left hand side is empty");

            ret.name = buf;
            buf.clear();
            switch (*mp_char)
            {
                case '=':
                    ret.type = model_parser::ct_formula;
                    break;
                case ':':
                    ret.type = model_parser::ct_value;
                    break;
                case '@':
                    ret.type = model_parser::ct_string;
                    break;
                default:
                    ;
            }
        }
        else
        {
            if (buf.empty())
                buf.set_start(mp_char);
            else
                buf.inc();
        }
    }

    ret.value = buf;

    if (ret.type == model_parser::ct_value && !ret.value.empty())
    {
        // Check if this is a potential boolean value.
        if (ret.value[0] == 't' || ret.value[0] == 'f')
            ret.type = model_parser::ct_boolean;
    }

#if DEBUG_MODEL_PARSER
    __IXION_DEBUG_OUT__ << "name: " << ret.name.str() << "  value: " << ret.value.str() << endl;
#endif

    if (ret.name.empty())
    {
        if (ret.value.empty())
            // This is an empty line. Bail out.
            return ret;

        // Buffer is not empty but name is not given.  We must be missing a separator.
        std::ostringstream os;
        os << "separator may be missing (name='" << ret.name << "'; value='" << ret.value << "')";
        throw model_parser::parse_error(os.str());
    }

    formula_name_t fnt = mp_name_resolver->resolve(
        ret.name.get(), ret.name.size(), abs_address_t(m_current_sheet,0,0));

    if (fnt.type != formula_name_t::cell_reference)
    {
        ostringstream os;
        os << "invalid cell name: " << ret.name.str();
        throw model_parser::parse_error(os.str());
    }

    ret.pos = abs_address_t(fnt.address.sheet, fnt.address.row, fnt.address.col);

    return ret;
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

        formula_name_t name_type = mp_name_resolver->resolve(&name[0], name.size(), abs_address_t());
        if (name_type.type != formula_name_t::cell_reference)
        {
            ostringstream os;
            os << "unrecognized cell address: " << name;
            throw std::runtime_error(os.str());
        }

        const formula_name_t::address_type& _addr = name_type.address;
        abs_address_t addr(_addr.sheet, _addr.row, _addr.col);

        switch (m_context.get_celltype(addr))
        {
            case celltype_t::formula:
            {
                const formula_cell* fcell = m_context.get_formula_cell(addr);
                const formula_result& res_cell = fcell->get_result_cache();

                if (res_cell != res)
                {
                    ostringstream os;
                    os << "unexpected result: (expected: " << res.str(m_context) << "; actual: " << res_cell.str(m_context) << ")";
                    throw check_error(os.str());
                }
                break;
            }
            case celltype_t::numeric:
            {
                double actual_val = m_context.get_numeric_value(addr);
                if (actual_val != res.get_value())
                {
                    ostringstream os;
                    os << "unexpected numeric result: (expected: " << res.get_value() << "; actual: " << actual_val << ")";
                    throw check_error(os.str());
                }
                break;
            }
            case celltype_t::boolean:
            {
                bool actual = m_context.get_boolean_value(addr);
                bool expected = res.get_value() ? true : false;
                if (actual != expected)
                {
                    ostringstream os;
                    os << "unexpected boolean result: (expected: " << expected << "; actual: " << actual << ")";
                    throw check_error(os.str());
                }
                break;
            }
            case celltype_t::string:
            {
                string_id_t str_id = m_context.get_string_identifier(addr);

                if (str_id != res.get_string())
                {
                    const string* ps = m_context.get_string(str_id);
                    if (!ps)
                        throw check_error("failed to retrieve a string value for a string cell.");

                    ostringstream os;
                    os << "unexpected string result: (expected: " << res.get_string() << "; actual: " << *ps << ")";
                    throw check_error(os.str());
                }
                break;
            }
            default:
                throw check_error("unhandled cell type.");
        }
    }
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
