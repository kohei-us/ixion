/*************************************************************************
 *
 * Copyright (c) 2011 Kohei Yoshida
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

#ifndef __IXION_MODEL_CONTEXT_HPP__
#define __IXION_MODEL_CONTEXT_HPP__

#include "ixion/cell.hpp"
#include "ixion/mem_str_buf.hpp"
#include "ixion/interface/model_context.hpp"

#include <string>
#include <deque>
#include <memory>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/unordered_map.hpp>

namespace ixion {

class cells_in_range;
class cells_in_range_impl;
class const_cells_in_range;
class session_handler;
struct abs_address_t;
struct config;
class matrix;

/**
 * This class stores all data relevant to current session.  You can think of
 * this like a document model for each formula calculation run.
 *
 * I will eventually create an interface class for this which is to be
 * sub-classed by the consumer application to provide access to the
 * application-specific context.
 */
class model_context : public interface::model_context
{
    friend class cells_in_range_impl;

    typedef boost::ptr_map<std::string, formula_cell> named_expressions_type;
    typedef boost::ptr_map<abs_address_t, base_cell> cell_store_type;
    typedef std::deque<formula_tokens_t*> formula_tokens_store_type;
    typedef boost::ptr_vector<std::string> strings_type;
    typedef boost::unordered_map<mem_str_buf, size_t, mem_str_buf::hash> string_map_type;
    typedef std::deque<abs_range_t> formula_token_range_type;

public:
    struct shared_tokens
    {
        formula_tokens_t* tokens;
        abs_range_t range;

        shared_tokens();
        shared_tokens(formula_tokens_t* tokens);
        shared_tokens(const shared_tokens& r);

        bool operator== (const shared_tokens& r) const;
    };
    typedef std::vector<shared_tokens> shared_tokens_type;

    model_context();
    virtual ~model_context();

    virtual const config& get_config() const;
    virtual const formula_name_resolver& get_name_resolver() const;
    virtual const base_cell* get_cell(const abs_address_t& addr) const;
    virtual base_cell* get_cell(const abs_address_t& addr);
    virtual interface::cells_in_range* get_cells_in_range(const abs_range_t& range);
    virtual interface::const_cells_in_range* get_cells_in_range(const abs_range_t& range) const;
    virtual ::std::string get_cell_name(const base_cell* p) const;
    virtual abs_address_t get_cell_position(const base_cell* p) const;
    virtual const formula_cell* get_named_expression(const ::std::string& name) const;
    virtual const ::std::string* get_named_expression_name(const formula_cell* expr) const;
    virtual matrix get_range_value(const abs_range_t& range) const;
    virtual interface::session_handler* get_session_handler() const;
    virtual const formula_tokens_t* get_formula_tokens(sheet_t sheet, size_t identifier) const;
    virtual size_t add_formula_tokens(sheet_t sheet, formula_tokens_t* p);
    virtual void remove_formula_tokens(sheet_t sheet, size_t identifier);
    virtual size_t set_formula_tokens_shared(sheet_t sheet, size_t identifier);
    virtual abs_range_t get_shared_formula_range(sheet_t sheet, size_t identifier) const;
    virtual void set_shared_formula_range(sheet_t sheet, size_t identifier, const abs_range_t& range);
    virtual size_t add_string(const char* p, size_t n);
    virtual const std::string* get_string(size_t identifier) const;

    void set_cell(const abs_address_t& addr, base_cell* cell);
    void erase_cell(const abs_address_t& addr);

    void set_named_expression(const char* p, size_t n, formula_cell* cell);
    formula_cell* get_named_expression(const ::std::string& name);

private:
    config* mp_config;
    formula_name_resolver* mp_name_resolver;
    session_handler* mp_session_handler;
    named_expressions_type m_named_expressions;
    cell_store_type m_cells; // TODO: This storage needs to be optimized.
    formula_tokens_store_type m_tokens;
    shared_tokens_type m_shared_tokens;
    formula_token_range_type m_shared_token_ranges;
    strings_type m_strings;
    string_map_type m_string_map;
};

}

#endif
