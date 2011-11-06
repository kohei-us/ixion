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

#include "ixion/model_context.hpp"
#include "ixion/formula_name_resolver.hpp"
#include "ixion/matrix.hpp"
#include "ixion/cells_in_range.hpp"
#include "ixion/config.hpp"
#include "ixion/session_handler.hpp"
#include "ixion/cell_listener_tracker.hpp"

#include <memory>

#define DEBUG_MODEL_CONTEXT 0

using namespace std;

namespace ixion {

namespace {

struct delete_shared_tokens : public std::unary_function<model_context::shared_tokens, void>
{
    void operator() (const model_context::shared_tokens& v)
    {
        delete v.tokens;
    }
};

class find_tokens_by_pointer : public std::unary_function<model_context::shared_tokens, bool>
{
    const formula_tokens_t* m_ptr;
public:
    find_tokens_by_pointer(const formula_tokens_t* p) : m_ptr(p) {}
    bool operator() (const model_context::shared_tokens& v) const
    {
        return v.tokens == m_ptr;
    }
};

}

model_context::shared_tokens::shared_tokens() : tokens(NULL) {}
model_context::shared_tokens::shared_tokens(formula_tokens_t* _tokens) : tokens(_tokens) {}
model_context::shared_tokens::shared_tokens(const shared_tokens& r) : tokens(r.tokens), range(r.range) {}

bool model_context::shared_tokens::operator== (const shared_tokens& r) const
{
    return tokens == r.tokens && range == r.range;
}

model_context::model_context() :
    mp_config(new config),
    mp_name_resolver(new formula_name_resolver_a1),
    mp_session_handler(new session_handler(*this))
{}

model_context::~model_context()
{
    delete mp_config;
    delete mp_name_resolver;
    delete mp_session_handler;

    for_each(m_tokens.begin(), m_tokens.end(), delete_element<formula_tokens_t>());
    for_each(m_shared_tokens.begin(), m_shared_tokens.end(), delete_shared_tokens());

    cell_listener_tracker::reset();
}

const config& model_context::get_config() const
{
    return *mp_config;
}

const formula_name_resolver& model_context::get_name_resolver() const
{
    return *mp_name_resolver;
}

void model_context::set_cell(const abs_address_t& addr, base_cell* cell)
{
    erase_cell(addr);
    abs_address_t addr2(addr);
    m_cells.insert(addr2, cell);
}

void model_context::erase_cell(const abs_address_t& addr)
{
    cell_store_type::iterator itr = m_cells.find(addr);
    if (itr == m_cells.end())
        return;

    const base_cell& cell = *itr->second;
    if (cell.get_celltype() == celltype_formula)
        remove_formula_tokens(addr.sheet, cell.get_identifier());

    m_cells.erase(itr);
}

const base_cell* model_context::get_cell(const abs_address_t& addr) const
{
    cell_store_type::const_iterator itr = m_cells.find(addr);
    return itr == m_cells.end() ? NULL : itr->second;
}

base_cell* model_context::get_cell(const abs_address_t& addr)
{
    cell_store_type::iterator itr = m_cells.find(addr);
    return itr == m_cells.end() ? NULL : itr->second;
}

string model_context::get_cell_name(const base_cell* p) const
{
    cell_store_type::const_iterator itr = m_cells.begin(), itr_end = m_cells.end();
    for (; itr != itr_end; ++itr)
    {
        if (itr->second == p)
            return get_name_resolver().get_name(itr->first, abs_address_t(), false);
    }

    // Cell not found.  Return an empty string.
    return string();
}

abs_address_t model_context::get_cell_position(const base_cell* p) const
{
    cell_store_type::const_iterator itr = m_cells.begin(), itr_end = m_cells.end();
    for (; itr != itr_end; ++itr)
    {
        if (itr->second == p)
            return itr->first;
    }

    throw general_error("cell instance not found");
}

iface::cells_in_range* model_context::get_cells_in_range(const abs_range_t& range)
{
    return new cells_in_range(*this, range);
}

iface::const_cells_in_range* model_context::get_cells_in_range(const abs_range_t& range) const
{
    return new const_cells_in_range(*this, range);
}

matrix model_context::get_range_value(const abs_range_t& range) const
{
    if (range.first.sheet != range.last.sheet)
        throw general_error("multi-sheet range is not allowed.");

    row_t rows = range.last.row - range.first.row + 1;
    col_t cols = range.last.column - range.first.column + 1;

    matrix ret(rows, cols);
    for (row_t i = 0; i < rows; ++i)
    {
        for (col_t j = 0; j < cols; ++j)
        {
            row_t row = i + range.first.row;
            col_t col = j + range.first.column;
            const base_cell* p = get_cell(abs_address_t(range.first.sheet, row, col));
#if DEBUG_MODEL_CONTEXT
            __IXION_DEBUG_OUT__ << "cell address (sheet=" << range.first.sheet << "; row=" << row << "; col=" << col << ") = " << p << endl;
#endif
            if (!p)
                // empty cell.
                continue;

            // TODO: we need to handle string types when that becomes available.
            ret.set(i, j, p->get_value());
        }
    }
    return ret;
}

iface::session_handler* model_context::get_session_handler() const
{
    return mp_session_handler;
}

const formula_tokens_t* model_context::get_formula_tokens(sheet_t sheet, size_t identifier) const
{
    if (m_tokens.size() <= identifier)
        return NULL;
    return m_tokens[identifier];
}

size_t model_context::add_formula_tokens(sheet_t sheet, formula_tokens_t* p)
{
    // First, search for a NULL spot.
    formula_tokens_store_type::iterator itr = std::find(
        m_tokens.begin(), m_tokens.end(), static_cast<formula_tokens_t*>(NULL));

    if (itr != m_tokens.end())
    {
        // NULL spot found.
        size_t pos = std::distance(m_tokens.begin(), itr);
        m_tokens[pos] = p;
        return pos;
    }

    size_t identifier = m_tokens.size();
    m_tokens.push_back(p);
    return identifier;
}

void model_context::remove_formula_tokens(sheet_t sheet, size_t identifier)
{
    if (m_tokens.size() >= identifier)
        return;

    delete m_tokens[identifier];
    m_tokens[identifier] = NULL;
}

const formula_tokens_t* model_context::get_shared_formula_tokens(sheet_t sheet, size_t identifier) const
{
#if DEBUG_MODEL_CONTEXT
    __IXION_DEBUG_OUT__ << "identifier: " << identifier << "  shared token count: " << m_shared_tokens.size() << endl;
#endif
    if (m_shared_tokens.size() <= identifier)
        return NULL;

#if DEBUG_MODEL_CONTEXT
    __IXION_DEBUG_OUT__ << "tokens: " << m_shared_tokens[identifier].tokens << endl;
#endif
    return m_shared_tokens[identifier].tokens;
}

size_t model_context::set_formula_tokens_shared(sheet_t sheet, size_t identifier)
{
    assert(identifier < m_tokens.size());
    formula_tokens_t* tokens = m_tokens.at(identifier);
    assert(tokens);
    m_tokens[identifier] = NULL;

    // First, search for a NULL spot.
    shared_tokens_type::iterator itr = std::find_if(
        m_shared_tokens.begin(), m_shared_tokens.end(),
        find_tokens_by_pointer(static_cast<const formula_tokens_t*>(NULL)));

    if (itr != m_shared_tokens.end())
    {
        // NULL spot found.
        size_t pos = std::distance(m_shared_tokens.begin(), itr);
        m_shared_tokens[pos].tokens = tokens;
        return pos;
    }

    size_t shared_identifier = m_shared_tokens.size();
    m_shared_tokens.push_back(shared_tokens(tokens));
    return shared_identifier;
}

abs_range_t model_context::get_shared_formula_range(sheet_t sheet, size_t identifier) const
{
    assert(identifier < m_shared_tokens.size());
    return m_shared_tokens.at(identifier).range;
}

void model_context::set_shared_formula_range(sheet_t sheet, size_t identifier, const abs_range_t& range)
{
    m_shared_tokens.at(identifier).range = range;
}

size_t model_context::add_string(const char* p, size_t n)
{
    string_map_type::iterator itr = m_string_map.find(mem_str_buf(p, n));
    if (itr != m_string_map.end())
        return itr->second;

    size_t str_id = m_strings.size();
    std::auto_ptr<string> ps(new string(p, n));
    p = &(*ps)[0];
    mem_str_buf key(p, n);
    m_strings.push_back(ps);
    m_string_map.insert(string_map_type::value_type(key, str_id));
    return str_id;
}

const std::string* model_context::get_string(size_t identifier) const
{
    if (identifier >= m_strings.size())
        return NULL;

    return &m_strings[identifier];
}

sheet_t model_context::get_sheet_index(const char* p, size_t n) const
{
    strings_type::const_iterator itr_beg = m_sheet_names.begin(), itr_end = m_sheet_names.end();
    for (strings_type::const_iterator itr = itr_beg; itr != itr_end; ++itr)
    {
        const std::string& s = *itr;
        if (s.empty())
            continue;

        mem_str_buf s1(&s[0], s.size()), s2(p, n);
        if (s1 == s2)
            return static_cast<sheet_t>(std::distance(itr_beg, itr));
    }
    return invalid_sheet;
}

std::string model_context::get_sheet_name(sheet_t sheet) const
{
    if (m_sheet_names.size() <= static_cast<size_t>(sheet))
        return std::string();

    return m_sheet_names[sheet];
}

void model_context::set_named_expression(const char* p, size_t n, formula_cell* cell)
{
    string name(p, n);
    m_named_expressions.insert(name, cell);
}

formula_cell* model_context::get_named_expression(const string& name)
{
    named_expressions_type::iterator itr = m_named_expressions.find(name);
    return itr == m_named_expressions.end() ? NULL : itr->second;
}

const formula_cell* model_context::get_named_expression(const string& name) const
{
    named_expressions_type::const_iterator itr = m_named_expressions.find(name);
    return itr == m_named_expressions.end() ? NULL : itr->second;
}

const string* model_context::get_named_expression_name(const formula_cell* expr) const
{
    named_expressions_type::const_iterator itr = m_named_expressions.begin(), itr_end = m_named_expressions.end();
    for (; itr != itr_end; ++itr)
    {
        if (itr->second == expr)
            return &itr->first;
    }
    return NULL;
}

void model_context::append_sheet_name(const char* p, size_t n)
{
    m_sheet_names.push_back(new string(p, n));
}

}
