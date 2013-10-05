/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __IXION_MODEL_PARSER_HPP__
#define __IXION_MODEL_PARSER_HPP__

#include "ixion/cell.hpp"
#include "ixion/exceptions.hpp"
#include "ixion/model_context.hpp"
#include "ixion/hash_container/map.hpp"

#include <string>
#include <exception>
#include <vector>

#include <boost/noncopyable.hpp>

namespace ixion {

class formula_result;

class model_parser : public ::boost::noncopyable
{
public:
    typedef _ixion_unordered_map_type< ::std::string, formula_result> results_type;

    class parse_error : public ::std::exception
    {
    public:
        explicit parse_error(const ::std::string& msg);
        ~parse_error() throw();
        virtual const char* what() const throw();
    private:
        ::std::string m_msg;
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

    model_parser(const ::std::string& filepath, size_t thread_count);
    ~model_parser();

    void parse();

private:
    model_parser(); // disabled

    void parse_init(const char*& p);
    void parse_result(const char*& p);

    void check();

private:
    model_context m_context;
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
