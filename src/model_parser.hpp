/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_MODEL_PARSER_HPP
#define INCLUDED_IXION_MODEL_PARSER_HPP

#include "ixion/exceptions.hpp"
#include "ixion/model_context.hpp"
#include "ixion/formula_result.hpp"

#include "session_handler.hpp"
#include "table_handler.hpp"

#include <string>
#include <exception>
#include <vector>
#include <unordered_map>

namespace ixion {

class model_parser
{
    enum parse_mode_type
    {
        parse_mode_unknown = 0,
        parse_mode_init,
        parse_mode_result,
        parse_mode_edit,
        parse_mode_table,
        parse_mode_session,
        parse_mode_named_expression,
        parse_mode_exit
    };

    using parsed_assignment_type = std::pair<mem_str_buf, mem_str_buf>;

    struct named_expression_type
    {
        std::string name;
        std::string expression;
        abs_address_t origin;
        sheet_t scope = global_scope;
    };

    /**
     * Right-hand-side cell content type.
     */
    enum cell_type
    {
        ct_unknown = 0,
        ct_formula,
        ct_value,
        ct_string,
        ct_boolean
    };

    struct cell_def_type
    {
        mem_str_buf name;
        mem_str_buf value;
        cell_type type;
        abs_range_t pos;

        bool matrix_value = false;
    };

public:
    typedef std::unordered_map< ::std::string, formula_result> results_type;

    class parse_error : public general_error
    {
    public:
        explicit parse_error(const std::string& msg);
    };

    class check_error : public general_error
    {
    public:
        check_error(const ::std::string& msg);
    };

    model_parser() = delete;
    model_parser(const model_parser&) = delete;
    model_parser& operator= (model_parser) = delete;

    model_parser(const ::std::string& filepath, size_t thread_count);
    ~model_parser();

    void parse();

private:
    void init_model();

    void parse_command();

    void parse_session();
    void parse_init();
    void parse_edit();
    void parse_result();

    void parse_table();
    void parse_table_columns(const mem_str_buf& str);
    void push_table();

    void parse_named_expression();
    void push_named_expression();

    /**
     * Parse a simple left=right assignment line.
     */
    parsed_assignment_type parse_assignment();

    cell_def_type parse_cell_definition();

    void check();

    std::string get_display_cell_string(const abs_address_t& pos) const;
    std::string get_display_range_string(const abs_range_t& pos) const;

private:

    model_context m_context;
    table_handler m_table_handler;
    session_handler::factory m_session_handler_factory;
    std::unique_ptr<table_handler::entry> mp_table_entry;
    std::unique_ptr<formula_name_resolver> mp_name_resolver;
    std::unique_ptr<named_expression_type> mp_named_expression;
    std::string m_filepath;
    std::string m_strm;
    size_t m_thread_count;
    cell_address_set_t m_dirty_cells;
    modified_cells_t m_dirty_cell_addrs;
    results_type m_formula_results;

    const char* mp_head;
    const char* mp_end;
    const char* mp_char;

    row_t m_row_limit;
    col_t m_col_limit;
    sheet_t m_current_sheet;

    parse_mode_type m_parse_mode;
    bool m_print_separator:1;
    bool m_print_sheet_name:1;
};

}

#endif
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
