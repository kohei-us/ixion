/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "model_parser.hpp"
#include "app_common.hpp"

#include "ixion/formula.hpp"
#include "ixion/formula_name_resolver.hpp"
#include "ixion/formula_result.hpp"
#include "ixion/macros.hpp"
#include "ixion/address_iterator.hpp"
#include "ixion/dirty_cell_tracker.hpp"
#include "ixion/cell_access.hpp"
#include "ixion/config.hpp"
#include "ixion/cell.hpp"

#include <sstream>
#include <iostream>
#include <vector>
#include <functional>
#include <cstring>
#include <cassert>
#include <memory>

#include <mdds/sorted_string_map.hpp>

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

class string_printer
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

void print_section_title(const char* title)
{
    std::cout << detail::get_formula_result_output_separator() << std::endl << title << std::endl;
}

namespace commands {

enum class type
{
    unknown,
    comment,
    calc,
    recalc,
    check,
    exit,
    push,
    mode_init,
    mode_edit,
    mode_result,
    mode_result_cache,
    mode_table,
    mode_session,
    mode_named_expression,
    print_dependency,
};

typedef mdds::sorted_string_map<type> map_type;

// Keys must be sorted.
const std::vector<map_type::entry> entries =
{
    { IXION_ASCII("%"),                     type::comment               },
    { IXION_ASCII("calc"),                  type::calc                  },
    { IXION_ASCII("check"),                 type::check                 },
    { IXION_ASCII("exit"),                  type::exit                  },
    { IXION_ASCII("mode edit"),             type::mode_edit             },
    { IXION_ASCII("mode init"),             type::mode_init             },
    { IXION_ASCII("mode named-expression"), type::mode_named_expression },
    { IXION_ASCII("mode result"),           type::mode_result           },
    { IXION_ASCII("mode result-cache"),     type::mode_result_cache     },
    { IXION_ASCII("mode session"),          type::mode_session          },
    { IXION_ASCII("mode table"),            type::mode_table            },
    { IXION_ASCII("print dependency"),      type::print_dependency      },
    { IXION_ASCII("push"),                  type::push                  },
    { IXION_ASCII("recalc"),                type::recalc                },
};

const map_type& get()
{
    static map_type mt(entries.data(), entries.size(), type::unknown);
    return mt;
}

} // namespace commands

} // anonymous namespace

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
    m_context({1048576, 1024}),
    m_table_handler(),
    m_session_handler_factory(m_context),
    mp_table_entry(nullptr),
    mp_name_resolver(formula_name_resolver::get(formula_name_resolver_t::excel_a1, &m_context)),
    m_filepath(filepath),
    m_strm(detail::load_file_content(m_filepath)),
    m_thread_count(thread_count),
    mp_head(nullptr),
    mp_end(nullptr),
    mp_char(nullptr),
    m_current_sheet(0),
    m_parse_mode(parse_mode_unknown),
    m_print_separator(false),
    m_print_sheet_name(false)
{
    m_context.set_session_handler_factory(&m_session_handler_factory);
    m_context.set_table_handler(&m_table_handler);

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
            cout << detail::get_formula_result_output_separator() << endl;
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
            case parse_mode_result_cache:
                parse_result_cache();
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
        m_context.append_sheet("sheet");
}

void model_parser::parse_command()
{
    // This line contains a command.
    mem_str_buf buf_cmd = parse_command_to_buffer(mp_char, mp_end);
    commands::type cmd = commands::get().find(buf_cmd.get(), buf_cmd.size());

    switch (cmd)
    {
        case commands::type::comment:
            // This is a comment line.  Just ignore it.
            break;
        case commands::type::calc:
        {
            print_section_title("calculating");

            // Perform full calculation on all currently stored formula cells.

            for (const abs_range_t& pos : m_dirty_formula_cells)
                register_formula_cell(m_context, pos.first);

            abs_range_set_t empty;
            std::vector<abs_range_t> sorted_cells =
                query_and_sort_dirty_cells(m_context, empty, &m_dirty_formula_cells);
            calculate_sorted_cells(m_context, sorted_cells, m_thread_count);
            break;
        }
        case commands::type::recalc:
        {
            print_section_title("recalculating");

            // Perform partial recalculation only on those formula cells that
            // need recalculation.

            std::vector<abs_range_t> sorted_cells =
                query_and_sort_dirty_cells(m_context, m_modified_cells, &m_dirty_formula_cells);

            calculate_sorted_cells(m_context, sorted_cells, m_thread_count);
            break;
        }
        case commands::type::check:
        {
            // Check cell results.
            check();
            break;
        }
        case commands::type::exit:
        {
            // Exit the loop.
            m_parse_mode = parse_mode_exit;
            return;
        }
        case commands::type::push:
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
            break;
        }
        case commands::type::mode_init:
        {
            print_section_title("initializing");

            m_parse_mode = parse_mode_init;
            m_print_separator = true;
            break;
        }
        case commands::type::mode_result:
        {
            // Clear any previous result values.
            m_formula_results.clear();
            m_parse_mode = parse_mode_result;
            break;
        }
        case commands::type::mode_result_cache:
        {
            print_section_title("caching formula results");

            m_parse_mode = parse_mode_result_cache;
            m_print_separator = true;
            break;
        }
        case commands::type::mode_edit:
        {
            print_section_title("editing");

            m_parse_mode = parse_mode_edit;
            m_dirty_formula_cells.clear();
            m_modified_cells.clear();
            m_print_separator = true;
            break;
        }
        case commands::type::mode_table:
        {
            m_parse_mode = parse_mode_table;
            mp_table_entry.reset(new table_handler::entry);
            break;
        }
        case commands::type::mode_session:
        {
            print_section_title("session");

            m_print_separator = true;
            m_parse_mode = parse_mode_session;
            break;
        }
        case commands::type::mode_named_expression:
        {
            m_print_separator = true;
            m_parse_mode = parse_mode_named_expression;
            mp_named_expression = std::make_unique<named_expression_type>();
            break;
        }
        case commands::type::print_dependency:
        {
            print_section_title("print dependency");
            print_dependency();
            break;
        }
        case commands::type::unknown:
        {
            ostringstream os;
            os << "unknown command: " << buf_cmd.str() << endl;
            throw parse_error(os.str());
        }
        default:
            ;
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
    {
        rc_size_t ss = m_context.get_sheet_size();
        ss.row = to_long(value);
        m_context.set_sheet_size(ss);
    }
    else if (cmd == "column-limit")
    {
        rc_size_t ss = m_context.get_sheet_size();
        ss.column = to_long(value);
        m_context.set_sheet_size(ss);
    }
    else if (cmd == "insert-sheet")
    {
        m_context.append_sheet({value.get(), value.size()});
        cout << "sheet: (name: " << value << ")" << endl;
    }
    else if (cmd == "current-sheet")
    {
        m_current_sheet = m_context.get_sheet_index({value.get(), value.size()});

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
        m_print_sheet_name = to_bool({value.get(), value.size()});
        m_session_handler_factory.show_sheet_name(m_print_sheet_name);
    }
}

void model_parser::parse_init()
{
    init_model();

    cell_def_type cell_def = parse_cell_definition();
    if (cell_def.name.empty() && cell_def.value.empty())
        return;

    if (cell_def.matrix_value)
    {
        assert(cell_def.type == ct_formula);
        const abs_address_t& pos = cell_def.pos.first;

        formula_tokens_t tokens =
            parse_formula_string(
                m_context, pos, *mp_name_resolver, {cell_def.value.get(), cell_def.value.size()});

        m_context.set_grouped_formula_cells(cell_def.pos, std::move(tokens));
        m_dirty_formula_cells.insert(cell_def.pos);

        cout << "{" << get_display_range_string(cell_def.pos) << "}: (m) " << cell_def.value.str() << endl;
        return;
    }

    abs_address_iterator iter(cell_def.pos, rc_direction_t::vertical);

    for (const abs_address_t& pos : iter)
    {
        m_modified_cells.insert(pos);

        switch (cell_def.type)
        {
            case ct_formula:
            {
                formula_tokens_t tokens =
                    parse_formula_string(
                        m_context, pos, *mp_name_resolver, {cell_def.value.get(), cell_def.value.size()});

                auto ts = formula_tokens_store::create();
                ts->get() = std::move(tokens);
                m_context.set_formula_cell(pos, ts);
                m_dirty_formula_cells.insert(pos);

                cout << get_display_cell_string(pos) << ": (f) " << cell_def.value.str() << endl;
                break;
            }
            case ct_string:
            {
                m_context.set_string_cell(pos, { cell_def.value.get(), cell_def.value.size() });

                cout << get_display_cell_string(pos) << ": (s) " << cell_def.value.str() << endl;
                break;
            }
            case ct_value:
            {
                double v = to_double({cell_def.value.get(), cell_def.value.size()});
                m_context.set_numeric_cell(pos, v);

                cout << get_display_cell_string(pos) << ": (n) " << v << endl;
                break;
            }
            case ct_boolean:
            {
                bool b = to_bool({cell_def.value.get(), cell_def.value.size()});
                m_context.set_boolean_cell(pos, b);

                cout << get_display_cell_string(pos) << ": (b) " << (b ? "true" : "false") << endl;
                break;
            }
            default:
                throw model_parser::parse_error("unknown content type");
        }
    }
}

void model_parser::parse_edit()
{
    cell_def_type cell_def = parse_cell_definition();
    if (cell_def.name.empty() && cell_def.value.empty())
        return;

    if (cell_def.matrix_value)
    {
        assert(cell_def.type == ct_formula);
        const abs_address_t& pos = cell_def.pos.first;

        m_modified_cells.insert(pos);
        unregister_formula_cell(m_context, pos);

        formula_tokens_t tokens =
            parse_formula_string(
                m_context, pos, *mp_name_resolver, {cell_def.value.get(), cell_def.value.size()});

        m_context.set_grouped_formula_cells(cell_def.pos, std::move(tokens));
        m_dirty_formula_cells.insert(cell_def.pos);
        register_formula_cell(m_context, pos);
        return;
    }

    abs_address_iterator iter(cell_def.pos, rc_direction_t::vertical);

    for (const abs_address_t& pos : iter)
    {
        m_modified_cells.insert(pos);
        unregister_formula_cell(m_context, pos);

        if (cell_def.value.empty())
        {
            // A valid name is given but with empty definition.  Just remove the
            // existing cell.
            m_context.empty_cell(pos);
            continue;
        }

        switch (cell_def.type)
        {
            case ct_formula:
            {
                formula_tokens_t tokens =
                    parse_formula_string(
                        m_context, pos, *mp_name_resolver, {cell_def.value.get(), cell_def.value.size()});

                auto ts = formula_tokens_store::create();
                ts->get() = std::move(tokens);
                m_context.set_formula_cell(pos, ts);
                m_dirty_formula_cells.insert(pos);
                register_formula_cell(m_context, pos);
                cout << get_display_cell_string(pos) << ": (f) " << cell_def.value.str() << endl;
            }
            break;
            case ct_string:
            {
                m_context.set_string_cell(pos, { cell_def.value.get(), cell_def.value.size() });
                cout << get_display_cell_string(pos) << ": (s) " << cell_def.value.str() << endl;
            }
            break;
            case ct_value:
            {
                double v = to_double({cell_def.value.get(), cell_def.value.size()});
                m_context.set_numeric_cell(pos, v);
                cout << get_display_cell_string(pos) << ": (n) " << v << endl;
            }
            break;
            default:
                throw model_parser::parse_error("unknown content type");
        }
    }
}

void model_parser::parse_result()
{
    parsed_assignment_type res = parse_assignment();

    string name_s = res.first.str();

    formula_result fres;
    fres.parse({res.second.get(), res.second.size()});
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

void model_parser::parse_result_cache()
{
    parsed_assignment_type res = parse_assignment();

    string name_s = res.first.str();

    formula_result fres;
    fres.parse({res.second.get(), res.second.size()});

    formula_name_t fnt = mp_name_resolver->resolve(name_s, abs_address_t(m_current_sheet,0,0));

    switch (fnt.type)
    {
        case formula_name_t::cell_reference:
        {
            abs_address_t pos = std::get<address_t>(fnt.value).to_abs(abs_address_t());
            formula_cell* fc = m_context.get_formula_cell(pos);
            if (!fc)
            {
                std::ostringstream os;
                os << name_s << " is not a formula cell";
                throw model_parser::parse_error(name_s);
            }

            fc->set_result_cache(fres);

            cout << get_display_cell_string(pos) << ": " << fres.str(m_context) << endl;
            break;
        }
        case formula_name_t::range_reference:
            throw model_parser::parse_error("TODO: we do not support setting result cache to range just yet.");
        default:
        {
            std::ostringstream os;
            os << "invalid cell name: " << name_s;
            throw model_parser::parse_error(os.str());
        }
    }
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
        entry.name = m_context.add_string({value.get(), value.size()});
    else if (name == "range")
    {
        if (!mp_name_resolver)
            return;

        abs_address_t pos(m_current_sheet,0,0);
        formula_name_t ret = mp_name_resolver->resolve({value.get(), value.size()}, pos);
        if (ret.type != formula_name_t::range_reference)
            throw parse_error("range of a table is expected to be given as a range reference.");

        entry.range = std::get<range_t>(ret.value).to_abs(pos);
    }
    else if (name == "columns")
        parse_table_columns(value);
    else if (name == "totals-row-count")
        entry.totals_row_count = to_double({value.get(), value.size()});
}

void model_parser::push_table()
{
    cout << detail::get_formula_result_output_separator() << endl;

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
                {s.get(), s.size()}, abs_address_t(m_current_sheet,0,0));

        if (name.type != formula_name_t::name_type::cell_reference)
        {
            ostringstream os;
            os << "'" << s << "' is not a valid named expression origin.";
            throw parse_error(os.str());
        }

        mp_named_expression->origin = std::get<address_t>(name.value).to_abs(abs_address_t(m_current_sheet,0,0));
    }
    else if (res.first == "scope")
    {
        // Resolve it as a sheet name and store the sheet index if found.
        const mem_str_buf& s = res.second;
        mp_named_expression->scope = m_context.get_sheet_index({s.get(), s.size()});
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

    formula_tokens_t tokens = parse_formula_string(
        m_context, mp_named_expression->origin, *mp_name_resolver,
        mp_named_expression->expression);

    std::string exp_s = print_formula_tokens(
        m_context, mp_named_expression->origin, *mp_name_resolver, tokens);

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
        m_context.set_named_expression(mp_named_expression->name, std::move(tokens));
    }
    else
    {
        m_context.set_named_expression(
            mp_named_expression->scope, mp_named_expression->name, std::move(tokens));
    }

    mp_named_expression.reset();
}

void model_parser::print_dependency()
{
    std::cout << detail::get_formula_result_output_separator() << std::endl;
    std::cout << m_context.get_cell_tracker().to_string() << std::endl;
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
                col_name = m_context.add_string({buf.get(), buf.size()});

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
        col_name = m_context.add_string({buf.get(), buf.size()});

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
    enum class section_type
    {
        name,
        braced_name,
        after_braced_name,
        braced_value,
        value
    };

    section_type section = section_type::name;

    cell_def_type ret;
    ret.type = model_parser::ct_unknown;

    char skip_next = 0;

    mem_str_buf buf;

    const char* line_head = mp_char;

    for (; mp_char != mp_end  && *mp_char != '\n'; ++mp_char)
    {
        if (skip_next)
        {
            if (*mp_char != skip_next)
            {
                std::ostringstream os;
                os << "'" << skip_next << "' was expected, but '" << *mp_char << "' was found.";
                throw model_parser::parse_error(os.str());
            }

            skip_next = 0;
            continue;
        }

        switch (section)
        {
            case section_type::name:
            {
                if (mp_char == line_head && *mp_char == '{')
                {
                    section = section_type::braced_name;
                    continue;
                }

                if (is_separator(*mp_char))
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

                    section = section_type::value;
                    continue;
                }

                break;
            }
            case section_type::braced_name:
            {
                if (*mp_char == '}')
                {
                    ret.name = buf;
                    buf.clear();
                    section = section_type::after_braced_name;
                    continue;
                }

                break;
            }
            case section_type::after_braced_name:
            {
                switch (*mp_char)
                {
                    case '{':
                        section = section_type::braced_value;
                        ret.type = model_parser::ct_formula;
                        skip_next = '=';
                        break;
                    case '=':
                        ret.type = model_parser::ct_formula;
                        section = section_type::value;
                        break;
                    case ':':
                        ret.type = model_parser::ct_value;
                        section = section_type::value;
                        break;
                    case '@':
                        ret.type = model_parser::ct_string;
                        section = section_type::value;
                        break;
                    default:
                    {
                        std::ostringstream os;
                        os << "Unexpected character after braced name: '" << *mp_char << "'";
                        throw model_parser::parse_error(os.str());
                    }
                }

                continue; // skip this character.
            }
            case section_type::braced_value:
            case section_type::value:
            default:
                ;
        }

        if (buf.empty())
            buf.set_start(mp_char);
        else
            buf.inc();
    }

    ret.value = buf;

    if (ret.type == model_parser::ct_value && !ret.value.empty())
    {
        // Check if this is a potential boolean value.
        if (ret.value[0] == 't' || ret.value[0] == 'f')
            ret.type = model_parser::ct_boolean;
    }

    if (section == section_type::braced_value)
    {
        // Make sure that the braced value ends with '}'.
        char last = ret.value.back();
        if (last != '}')
        {
            std::ostringstream os;
            os << "'}' was expected at the end of a braced value, but '" << last << "' was found.";
            model_parser::parse_error(os.str());
        }
        ret.value.dec();
        ret.matrix_value = true;
    }

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
        {ret.name.get(), ret.name.size()}, abs_address_t(m_current_sheet,0,0));

    switch (fnt.type)
    {
        case formula_name_t::cell_reference:
        {
            ret.pos.first = std::get<address_t>(fnt.value).to_abs(abs_address_t(0,0,0));
            ret.pos.last = ret.pos.first;
            break;
        }
        case formula_name_t::range_reference:
        {
            ret.pos = std::get<range_t>(fnt.value).to_abs(abs_address_t(0,0,0));
            break;
        }
        default:
        {
            ostringstream os;
            os << "invalid cell name: " << ret.name.str();
            throw model_parser::parse_error(os.str());
        }
    }

    return ret;
}

void model_parser::check()
{
    cout << detail::get_formula_result_output_separator() << endl
         << "checking results" << endl
         << detail::get_formula_result_output_separator() << endl;

    results_type::const_iterator itr = m_formula_results.begin(), itr_end = m_formula_results.end();
    for (; itr != itr_end; ++itr)
    {
        const string& name = itr->first;
        if (name.empty())
            throw check_error("empty cell name");

        const formula_result& res = itr->second;
        cout << name << ": " << res.str(m_context) << endl;

        formula_name_t name_type = mp_name_resolver->resolve(name, abs_address_t());
        if (name_type.type != formula_name_t::cell_reference)
        {
            ostringstream os;
            os << "unrecognized cell address: " << name;
            throw std::runtime_error(os.str());
        }

        abs_address_t addr = std::get<address_t>(name_type.value).to_abs(abs_address_t());
        cell_access ca = m_context.get_cell_access(addr);

        switch (ca.get_type())
        {
            case celltype_t::formula:
            {
                formula_result res_cell = ca.get_formula_result();

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
                double actual_val = ca.get_numeric_value();
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
                bool actual = ca.get_boolean_value();
                bool expected = res.get_boolean();
                if (actual != expected)
                {
                    ostringstream os;
                    os << std::boolalpha;
                    os << "unexpected boolean result: (expected: " << expected << "; actual: " << actual << ")";
                    throw check_error(os.str());
                }
                break;
            }
            case celltype_t::string:
            {
                std::string_view actual = ca.get_string_value();
                const std::string& s_expected = res.get_string();

                if (actual != s_expected)
                {
                    std::ostringstream os;
                    os << "unexpected string result: (expected: '" << s_expected << "'; actual: '" << actual << "')";
                    throw check_error(os.str());
                }

                break;
            }
            case celltype_t::empty:
            {
                std::ostringstream os;
                os << "cell " << name << " is empty.";
                throw check_error(os.str());
            }
            case celltype_t::unknown:
            {
                std::ostringstream os;
                os << "cell type is unknown for cell " << name;
                throw check_error(os.str());
            }
        }
    }
}

std::string model_parser::get_display_cell_string(const abs_address_t& pos) const
{
    address_t pos_display(pos);
    pos_display.set_absolute(false);
    return mp_name_resolver->get_name(pos_display, abs_address_t(), m_print_sheet_name);
}

std::string model_parser::get_display_range_string(const abs_range_t& pos) const
{
    range_t pos_display(pos);
    pos_display.first.set_absolute(false);
    pos_display.last.set_absolute(false);
    return mp_name_resolver->get_name(pos_display, abs_address_t(), m_print_sheet_name);
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
