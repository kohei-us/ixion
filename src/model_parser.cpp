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
#include "ixion/address_range.hpp"
#include "ixion/dirty_cell_tracker.hpp"
#include "ixion/cell_access.hpp"
#include "ixion/config.hpp"
#include "ixion/cell.hpp"
#include "ixion/model_cell_range.hpp"

#include <sstream>
#include <iostream>
#include <vector>
#include <functional>
#include <cstring>
#include <cassert>
#include <memory>

#include <mdds/sorted_string_map.hpp>

namespace ixion {

namespace {

long to_long(std::string_view value)
{
    char* pe = nullptr;
    long ret = std::strtol(value.data(), &pe, 10);

    if (value.data() == pe)
    {
        std::ostringstream os;
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

/**
 * Tokenize a command line.  The first token is the command name, and the
 * rest are its arguments.  Consecutive spaces are treated as a single
 * token separator.
 */
std::vector<std::string_view> parse_command_to_tokens(const char*& p, const char* p_end)
{
    ++p; // skip '%'.

    if (p != p_end && *p == '%')
    {
        // This line is a comment.  Skip the rest of the line.
        std::string_view ret{p, 1u}; // return it as command named '%'
        while (p != p_end && *p != '\n') ++p;
        return { ret };
    }

    std::vector<std::string_view> tokens;
    const char* p_head = nullptr;

    for (; p != p_end && *p != '\n'; ++p)
    {
        if (*p == ' ')
        {
            if (p_head)
            {
                tokens.emplace_back(p_head, p - p_head);
                p_head = nullptr;
            }
            continue;
        }

        if (!p_head)
            p_head = p;
    }

    if (p_head)
        tokens.emplace_back(p_head, p - p_head);

    return tokens;
}

class string_printer
{
    char m_sep;

public:
    explicit string_printer(char sep) : m_sep(sep) {}

    template <typename Range>
    void print(const Range& range, std::ostream& os = std::cout) const
    {
        auto it = std::begin(range);
        auto end = std::end(range);
        if (it == end)
            return;

        os << *it;
        for (++it; it != end; ++it)
            os << m_sep << *it;
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
    copy_sheet,
    exit,
    push,
    mode,
    print,
};

typedef mdds::sorted_string_map<type> map_type;

// Keys must be sorted.
constexpr map_type::entry_type entries[] =
{
    { "%",          type::comment    },
    { "calc",       type::calc       },
    { "check",      type::check      },
    { "copy-sheet", type::copy_sheet },
    { "exit",       type::exit       },
    { "mode",       type::mode       },
    { "print",      type::print      },
    { "push",       type::push       },
    { "recalc",     type::recalc     },
};

const map_type& get()
{
    static map_type mt(entries, type::unknown);
    return mt;
}

} // namespace commands

namespace modes {

enum class type
{
    unknown,
    edit,
    init,
    named_expression,
    result,
    result_cache,
    session,
    table,
};

typedef mdds::sorted_string_map<type> map_type;

// Keys must be sorted.
constexpr map_type::entry_type entries[] =
{
    { "edit",             type::edit             },
    { "init",             type::init             },
    { "named-expression", type::named_expression },
    { "result",           type::result           },
    { "result-cache",     type::result_cache     },
    { "session",          type::session          },
    { "table",            type::table            },
};

const map_type& get()
{
    static map_type mt(entries, type::unknown);
    return mt;
}

} // namespace modes

} // anonymous namespace

model_parser::parse_error::parse_error(const std::string& msg) : general_error()
{
    std::ostringstream os;
    os << "parse error: " << msg;
    set_message(os.str());
}

// ============================================================================

model_parser::check_error::check_error(const std::string& msg) :
    general_error(msg) {}

// ============================================================================

model_parser::model_parser(const std::string& filepath, std::size_t thread_count) :
    m_context({1048576, 1024}),
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

    mp_head = m_strm.data();
    mp_end = mp_head + m_strm.size();
}

model_parser::~model_parser() = default;

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
            std::cout << detail::get_formula_result_output_separator() << std::endl;
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
    std::vector<std::string_view> tokens = parse_command_to_tokens(mp_char, mp_end);
    if (tokens.empty())
        throw parse_error("empty command line");

    commands::type cmd = commands::get().find(tokens[0]);

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
        case commands::type::copy_sheet:
        {
            if (tokens.size() != 3)
                throw parse_error("copy-sheet command expects the source and new sheet names as arguments");

            copy_sheet(tokens[1], tokens[2]);
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
        case commands::type::mode:
        {
            if (tokens.size() != 2)
                throw parse_error("mode command expects the mode name as its argument");

            set_mode(tokens[1]);
            break;
        }
        case commands::type::print:
        {
            if (tokens.size() == 2 && tokens[1] == "dependency")
            {
                print_section_title("print dependency");
                print_dependency();
            }
            else if (tokens.size() >= 3 && tokens[1] == "sheet")
            {
                print_section_title("print sheet");

                sheet_t sheet = m_context.get_sheet_index(tokens[2]);
                if (sheet == invalid_sheet)
                {
                    std::ostringstream os;
                    os << "no sheet named '" << tokens[2] << "'";
                    throw parse_error(os.str());
                }

                sheet_dump_mode_t mode = sheet_dump_mode_t::simple;
                auto resolver_type = formula_name_resolver_t::excel_a1;

                for (auto it = tokens.begin() + 3; it != tokens.end(); ++it)
                {
                    if (*it == "simple")
                        mode = sheet_dump_mode_t::simple;
                    else if (*it == "verbose")
                        mode = sheet_dump_mode_t::verbose;
                    else if (*it == "a1")
                        resolver_type = formula_name_resolver_t::excel_a1;
                    else if (*it == "r1c1")
                        resolver_type = formula_name_resolver_t::excel_r1c1;
                    else
                    {
                        std::ostringstream os;
                        os << "unknown print sheet option: " << *it;
                        throw parse_error(os.str());
                    }
                }

                auto resolver = formula_name_resolver::get(resolver_type, &m_context);
                std::ostringstream buf;
                m_context.dump_sheet(buf, sheet, mode, resolver.get());
                std::string grid = buf.str();
                if (grid.empty())
                    std::cout << "(empty sheet)" << std::endl;
                else
                    std::cout << grid << std::endl;
            }
            else
            {
                std::ostringstream os;
                os << "unknown print target";
                if (tokens.size() > 1)
                    os << ": " << tokens[1];
                throw parse_error(os.str());
            }
            break;
        }
        case commands::type::unknown:
        {
            std::ostringstream os;
            os << "unknown command: " << tokens[0] << std::endl;
            throw parse_error(os.str());
        }
        default:
            ;
    }
}

void model_parser::set_mode(std::string_view name)
{
    switch (modes::get().find(name))
    {
        case modes::type::init:
        {
            print_section_title("initializing");

            m_parse_mode = parse_mode_init;
            m_print_separator = true;
            break;
        }
        case modes::type::result:
        {
            // Clear any previous result values.
            m_formula_results.clear();
            m_parse_mode = parse_mode_result;
            break;
        }
        case modes::type::result_cache:
        {
            print_section_title("caching formula results");

            m_parse_mode = parse_mode_result_cache;
            m_print_separator = true;
            break;
        }
        case modes::type::edit:
        {
            print_section_title("editing");

            m_parse_mode = parse_mode_edit;
            m_dirty_formula_cells.clear();
            m_modified_cells.clear();
            m_print_separator = true;
            break;
        }
        case modes::type::table:
        {
            m_parse_mode = parse_mode_table;
            mp_table_entry.reset(new table_t);
            break;
        }
        case modes::type::session:
        {
            print_section_title("session");

            m_print_separator = true;
            m_parse_mode = parse_mode_session;
            break;
        }
        case modes::type::named_expression:
        {
            m_print_separator = true;
            m_parse_mode = parse_mode_named_expression;
            mp_named_expression = std::make_unique<named_expression_type>();
            break;
        }
        default:
        {
            std::ostringstream os;
            os << "unknown mode: " << name;
            throw parse_error(os.str());
        }
    }
}

void model_parser::parse_session()
{
    std::string_view cmd, value;
    std::string_view* buf = &cmd;

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
            *buf = std::string_view{mp_char, 1u};
        else
            *buf = std::string_view{buf->data(), buf->size() + 1u};
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
        m_context.append_sheet(std::string{value});
        std::cout << "sheet: (name: " << value << ")" << std::endl;
    }
    else if (cmd == "current-sheet")
    {
        m_current_sheet = m_context.get_sheet_index(value);

        if (m_current_sheet == invalid_sheet)
        {
            std::ostringstream os;
            os << "No sheet named '" << value << "' found.";
            throw parse_error(os.str());
        }

        std::cout << "current sheet: " << value << std::endl;
    }
    else if (cmd == "display-sheet-name")
    {
        std::cout << "display sheet name: " << value << std::endl;
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

    if (cell_def.matrix_value)
    {
        assert(cell_def.type == ct_formula);
        const abs_address_t& pos = cell_def.pos.first;

        formula_tokens_t tokens =
            parse_formula_string(m_context, pos, *mp_name_resolver, cell_def.value);

        m_context.set_grouped_formula_cells(cell_def.pos, std::move(tokens));
        m_dirty_formula_cells.insert(cell_def.pos);

        std::cout << "{" << get_display_range_string(cell_def.pos) << "}: (m) " << cell_def.value << std::endl;
        return;
    }

    abs_address_range iter(cell_def.pos, rc_direction_t::vertical);

    for (const abs_address_t& pos : iter)
    {
        m_modified_cells.insert(pos);

        switch (cell_def.type)
        {
            case ct_formula:
            {
                formula_tokens_t tokens =
                    parse_formula_string(m_context, pos, *mp_name_resolver, cell_def.value);

                auto ts = formula_tokens_store::create(std::move(tokens));
                m_context.set_formula_cell(pos, ts);
                m_dirty_formula_cells.insert(pos);

                std::cout << get_display_cell_string(pos) << ": (f) " << cell_def.value << std::endl;
                break;
            }
            case ct_string:
            {
                m_context.set_string_cell(pos, cell_def.value);

                std::cout << get_display_cell_string(pos) << ": (s) " << cell_def.value << std::endl;
                break;
            }
            case ct_value:
            {
                double v = to_double(cell_def.value);
                m_context.set_numeric_cell(pos, v);

                std::cout << get_display_cell_string(pos) << ": (n) " << v << std::endl;
                break;
            }
            case ct_boolean:
            {
                bool b = to_bool(cell_def.value);
                m_context.set_boolean_cell(pos, b);

                std::cout << get_display_cell_string(pos) << ": (b) " << (b ? "true" : "false") << std::endl;
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
            parse_formula_string(m_context, pos, *mp_name_resolver, cell_def.value);

        m_context.set_grouped_formula_cells(cell_def.pos, std::move(tokens));
        m_dirty_formula_cells.insert(cell_def.pos);
        register_formula_cell(m_context, pos);
        return;
    }

    abs_address_range iter(cell_def.pos, rc_direction_t::vertical);

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
                    parse_formula_string(m_context, pos, *mp_name_resolver, cell_def.value);

                auto ts = formula_tokens_store::create(std::move(tokens));
                m_context.set_formula_cell(pos, ts);
                m_dirty_formula_cells.insert(pos);
                register_formula_cell(m_context, pos);
                std::cout << get_display_cell_string(pos) << ": (f) " << cell_def.value << std::endl;
                break;
            }
            case ct_string:
            {
                m_context.set_string_cell(pos, cell_def.value);
                std::cout << get_display_cell_string(pos) << ": (s) " << cell_def.value << std::endl;
                break;
            }
            case ct_value:
            {
                double v = to_double(cell_def.value);
                m_context.set_numeric_cell(pos, v);
                std::cout << get_display_cell_string(pos) << ": (n) " << v << std::endl;
                break;
            }
            default:
                throw model_parser::parse_error("unknown content type");
        }
    }
}

void model_parser::parse_result()
{
    parsed_assignment_type res = parse_assignment();

    auto name_s = std::string{res.first};

    formula_result fres;
    fres.parse(res.second);
    model_parser::results_type::iterator itr = m_formula_results.find(name_s);
    if (itr == m_formula_results.end())
    {
        // This cell doesn't exist yet.
        auto r = m_formula_results.insert(model_parser::results_type::value_type(name_s, fres));
        if (!r.second)
            throw model_parser::parse_error("failed to insert a new result.");
    }
    else
        itr->second = fres;
}

void model_parser::parse_result_cache()
{
    parsed_assignment_type res = parse_assignment();

    auto name_s = std::string{res.first};

    formula_result fres;
    fres.parse(res.second);

    formula_name_t fnt = mp_name_resolver->resolve(name_s, abs_address_t(m_current_sheet,0,0));

    switch (fnt.type)
    {
        case formula_name_t::cell_reference:
        {
            abs_address_t pos = std::get<address_t>(fnt.value).to_abs(abs_address_t(m_current_sheet,0,0));
            formula_cell* fc = m_context.get_formula_cell(pos);
            if (!fc)
            {
                std::ostringstream os;
                os << name_s << " is not a formula cell";
                throw model_parser::parse_error(name_s);
            }

            fc->set_result_cache(fres);

            std::cout << get_display_cell_string(pos) << ": " << fres.str(m_context) << std::endl;
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
    const auto [name, value] = res;

    table_t& entry = *mp_table_entry;

    if (name == "name")
        entry.name = std::string{value};
    else if (name == "range")
    {
        if (!mp_name_resolver)
            return;

        abs_address_t pos(m_current_sheet,0,0);
        formula_name_t ret = mp_name_resolver->resolve(value, pos);
        if (ret.type != formula_name_t::range_reference)
            throw parse_error("range of a table is expected to be given as a range reference.");

        abs_range_t range = std::get<range_t>(ret.value).to_abs(pos);
        if (range.first.sheet != range.last.sheet)
            throw parse_error("range of a table must not span multiple sheets.");

        entry.sheet = range.first.sheet;
        entry.range = range;
    }
    else if (name == "columns")
        parse_table_columns(value);
    else if (name == "totals-row-count")
        entry.totals_row_count = to_double(value);
}

void model_parser::push_table()
{
    std::cout << detail::get_formula_result_output_separator() << std::endl;

    if (!mp_table_entry)
        return;

    table_t& entry = *mp_table_entry;

    std::cout << "name: " << entry.name << std::endl;

    if (mp_name_resolver)
    {
        abs_range_t range{entry.sheet, entry.range};
        std::cout << "range: "
            << mp_name_resolver->get_name(range, abs_address_t(m_current_sheet,0,0), false)
            << std::endl;
    }

    std::cout << "columns: ";
    string_printer{','}.print(entry.columns);
    std::cout << std::endl;

    std::cout << "totals row count: " << mp_table_entry->totals_row_count << std::endl;
    m_context.set_table(std::move(*mp_table_entry));
    mp_table_entry.reset();
}

void model_parser::copy_sheet(std::string_view src_name, std::string_view new_name)
{
    sheet_t src = m_context.get_sheet_index(src_name);
    if (src == invalid_sheet)
    {
        std::ostringstream os;
        os << "no sheet named '" << src_name << "' exists";
        throw parse_error(os.str());
    }

    print_section_title("copying sheet");
    std::cout << "source: " << src_name << std::endl;
    std::cout << "new: " << new_name << std::endl;

    auto res = m_context.append_sheet_copy(src, std::string{new_name});

    // The copied formula cells are new to the dependency tracker.
    if (abs_range_t data_range = m_context.get_data_range(res.sheet); data_range.valid())
    {
        auto cells = m_context.iterate_cells(res.sheet, rc_direction_t::vertical, data_range);
        auto it = cells.begin();

        while (it != cells.end())
        {
            if (it->type != cell_t::formula)
            {
                ++it;
                continue;
            }

            const auto* fc = std::get<const formula_cell*>(it->value);
            register_formula_cell(m_context, abs_address_t(res.sheet, it->row, it->col), fc);

            // Registering the top-most cell of a group registers the entire
            // group, so skip over the rest of the group.
            formula_group_t group = fc->get_group_properties();
            std::advance(it, group.grouped ? group.size.row : 1);
        }
    }

    m_dirty_formula_cells.insert(res.recalc_cells.begin(), res.recalc_cells.end());
}

void model_parser::parse_named_expression()
{
    assert(mp_named_expression);

    parsed_assignment_type res = parse_assignment();
    if (res.first == "name")
        mp_named_expression->name = std::string{res.second};
    else if (res.first == "expression")
        mp_named_expression->expression = std::string{res.second};
    else if (res.first == "origin")
    {
        const std::string_view s = res.second;

        formula_name_t name =
            mp_name_resolver->resolve(s, abs_address_t(m_current_sheet,0,0));

        if (name.type != formula_name_t::name_type::cell_reference)
        {
            std::ostringstream os;
            os << "'" << s << "' is not a valid named expression origin.";
            throw parse_error(os.str());
        }

        mp_named_expression->origin = std::get<address_t>(name.value).to_abs(abs_address_t(m_current_sheet,0,0));
    }
    else if (res.first == "scope")
    {
        // Resolve it as a sheet name and store the sheet index if found.
        mp_named_expression->scope = m_context.get_sheet_index(res.second);
        if (mp_named_expression->scope == invalid_sheet)
        {
            std::ostringstream os;
            os << "no sheet named '" << res.second << "' exists in the model.";
            throw parse_error(os.str());
        }
    }
    else
    {
        std::ostringstream os;
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

    std::cout << "name: " << mp_named_expression->name << std::endl;
    std::cout << "expression: " << exp_s << std::endl;
    std::cout << "origin: " << mp_named_expression->origin << std::endl;

    std::cout << "scope: ";

    if (mp_named_expression->scope == global_scope)
        std::cout << "(global)";
    else
    {
        auto sheet_name = m_context.get_sheet_name(mp_named_expression->scope);

        if (sheet_name.empty())
        {
            std::ostringstream os;
            os << "no sheet exists with a sheet index of " << mp_named_expression->scope;
            throw std::runtime_error(os.str());
        }

        std::cout << sheet_name;
    }

    std::cout << std::endl;

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

void model_parser::parse_table_columns(std::string_view str)
{
    assert(mp_table_entry);
    table_t& entry = *mp_table_entry;

    const char* p = str.data();
    const char* pend = p + str.size();
    std::string_view buf;
    for (; p != pend; ++p)
    {
        if (*p == ',')
        {
            // Flush the current column name buffer.
            entry.columns.emplace_back(buf);
            buf = std::string_view{};
        }
        else
        {
            if (buf.empty())
                buf = std::string_view{p, 1u};
            else
                buf = std::string_view{buf.data(), buf.size() + 1u};
        }
    }

    entry.columns.emplace_back(buf);
}

model_parser::parsed_assignment_type model_parser::parse_assignment()
{
    // Parse to get name and value strings.
    parsed_assignment_type res;
    std::string_view buf;

    for (; mp_char != mp_end && *mp_char != '\n'; ++mp_char)
    {
        if (*mp_char == '=')
        {
            if (buf.empty())
                throw model_parser::parse_error("left hand side is empty");

            res.first = buf;
            buf = std::string_view{};
        }
        else
        {
            if (buf.empty())
                buf = std::string_view{mp_char, 1u};
            else
                buf = std::string_view{buf.data(), buf.size() + 1u};
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

    std::string_view buf;

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
                    buf = std::string_view{};

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
                    buf = std::string_view{};
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
            buf = std::string_view{mp_char, 1u};
        else
            buf = std::string_view{buf.data(), buf.size() + 1u};
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
        ret.value = std::string_view{ret.value.data(), ret.value.size() - 1u};
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

    formula_name_t fnt = mp_name_resolver->resolve(ret.name, abs_address_t(m_current_sheet, 0, 0));

    switch (fnt.type)
    {
        case formula_name_t::cell_reference:
        {
            ret.pos.first = std::get<address_t>(fnt.value).to_abs(abs_address_t(m_current_sheet,0,0));
            ret.pos.last = ret.pos.first;
            break;
        }
        case formula_name_t::range_reference:
        {
            ret.pos = std::get<range_t>(fnt.value).to_abs(abs_address_t(m_current_sheet,0,0));
            break;
        }
        default:
        {
            std::ostringstream os;
            os << "invalid cell name: " << ret.name;
            throw model_parser::parse_error(os.str());
        }
    }

    return ret;
}

void model_parser::check()
{
    std::cout << detail::get_formula_result_output_separator() << std::endl
         << "checking results" << std::endl
         << detail::get_formula_result_output_separator() << std::endl;

    for (const auto& [name, res] : m_formula_results)
    {
        if (name.empty())
            throw check_error("empty cell name");

        std::cout << name << ": " << res.str(m_context) << std::endl;

        formula_name_t name_type = mp_name_resolver->resolve(name, abs_address_t());
        if (name_type.type != formula_name_t::cell_reference)
        {
            std::ostringstream os;
            os << "unrecognized cell address: " << name;
            throw std::runtime_error(os.str());
        }

        abs_address_t addr = std::get<address_t>(name_type.value).to_abs(abs_address_t());
        cell_access ca = m_context.get_cell_access(addr);

        switch (ca.get_type())
        {
            case cell_t::formula:
            {
                formula_result res_cell = ca.get_formula_result();

                if (res_cell != res)
                {
                    std::ostringstream os;
                    os << "unexpected result: (expected: " << res.str(m_context) << "; actual: " << res_cell.str(m_context) << ")";
                    throw check_error(os.str());
                }
                break;
            }
            case cell_t::numeric:
            {
                double actual_val = ca.get_numeric_value();
                if (actual_val != res.get_value())
                {
                    std::ostringstream os;
                    os << "unexpected numeric result: (expected: " << res.get_value() << "; actual: " << actual_val << ")";
                    throw check_error(os.str());
                }
                break;
            }
            case cell_t::boolean:
            {
                bool actual = ca.get_boolean_value();
                bool expected = res.get_boolean();
                if (actual != expected)
                {
                    std::ostringstream os;
                    os << std::boolalpha;
                    os << "unexpected boolean result: (expected: " << expected << "; actual: " << actual << ")";
                    throw check_error(os.str());
                }
                break;
            }
            case cell_t::string:
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
            case cell_t::empty:
            {
                std::ostringstream os;
                os << "cell " << name << " is empty.";
                throw check_error(os.str());
            }
            case cell_t::unknown:
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
