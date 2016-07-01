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

    /**
     * Right-hand-side cell content type.
     */
    enum cell_type
    {
        ct_unknown = 0,
        ct_formula,
        ct_value,
        ct_string
    };

    model_parser() = delete;
    model_parser(const model_parser&) = delete;
    model_parser& operator= (model_parser) = delete;

    model_parser(const ::std::string& filepath, size_t thread_count);
    ~model_parser();

    void parse();

private:

    void parse_init(const char*& p);
    void parse_result(const char*& p);
    void parse_table(const char*& p);
    void push_table();

    void parse_table_columns(const mem_str_buf& str);

    void check();

private:

    model_context m_context;
    table_handler m_table_handler;
    std::unique_ptr<table_handler::entry> mp_table_entry;
    std::unique_ptr<formula_name_resolver> mp_name_resolver;
    std::string m_filepath;
    size_t m_thread_count;
    dirty_formula_cells_t m_dirty_cells;
    modified_cells_t m_dirty_cell_addrs;
    results_type m_formula_results;

    bool m_print_separator:1;
};

}

#endif
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
