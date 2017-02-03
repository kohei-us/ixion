/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/model_context.hpp"
#include "ixion/formula_name_resolver.hpp"
#include "ixion/matrix.hpp"
#include "ixion/config.hpp"
#include "ixion/interface/session_handler.hpp"
#include "ixion/interface/table_handler.hpp"
#include "ixion/cell_listener_tracker.hpp"
#include "ixion/formula_result.hpp"
#include "ixion/formula.hpp"

#include "workbook.hpp"
#include "model_types.hpp"

#include <memory>
#include <sstream>
#include <unordered_map>
#include <map>
#include <vector>
#include <iostream>

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

    if (cxt.get_celltype(test) != celltype_t::formula)
        // The neighboring cell is not a formula cell.
        return false;

    formula_cell* test_cell = cxt.get_formula_cell(test);
    if (!test_cell)
        // The neighboring cell is not a formula cell.
        throw general_error("formula cell doesn't exist but it should!");

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

model_context::session_handler_factory dummy_session_handler_factory;

} // anonymous namespace

class model_context_impl
{
    typedef std::vector<std::string> strings_type;
    typedef std::vector<std::unique_ptr<std::string>> string_pool_type;
    typedef std::unordered_map<mem_str_buf, string_id_t, mem_str_buf::hash> string_map_type;
    typedef std::deque<formula_tokens_t*> formula_tokens_store_type;

    typedef model_context::shared_tokens shared_tokens;
    typedef model_context::shared_tokens_type shared_tokens_type;

public:
    model_context_impl() = delete;
    model_context_impl(const model_context_impl&) = delete;
    model_context_impl& operator= (model_context_impl) = delete;

    model_context_impl(model_context& parent) :
        m_parent(parent),
        mp_config(new config),
        mp_cell_listener_tracker(new cell_listener_tracker(parent)),
        mp_table_handler(nullptr),
        mp_session_factory(&dummy_session_handler_factory)
    {
    }

    ~model_context_impl()
    {
        delete mp_config;
        delete mp_cell_listener_tracker;

        for_each(m_tokens.begin(), m_tokens.end(), delete_element<formula_tokens_t>());
        for_each(m_shared_tokens.begin(), m_shared_tokens.end(), delete_shared_tokens());
    }

    const config& get_config() const
    {
        return *mp_config;
    }

    cell_listener_tracker& get_cell_listener_tracker()
    {
        return *mp_cell_listener_tracker;
    }

    std::unique_ptr<iface::session_handler> create_session_handler()
    {
        return mp_session_factory->create();
    }

    void set_session_handler_factory(model_context::session_handler_factory* factory)
    {
        mp_session_factory = factory;
    }

    iface::table_handler* get_table_handler()
    {
        return mp_table_handler;
    }

    const iface::table_handler* get_table_handler() const
    {
        return mp_table_handler;
    }

    void set_table_handler(iface::table_handler* handler)
    {
        mp_table_handler = handler;
    }

    void erase_cell(const abs_address_t& addr);
    void set_numeric_cell(const abs_address_t& addr, double val);
    void set_boolean_cell(const abs_address_t& addr, bool val);
    void set_string_cell(const abs_address_t& addr, const char* p, size_t n);
    void set_string_cell(const abs_address_t& addr, string_id_t identifier);
    void set_formula_cell(const abs_address_t& addr, const char* p, size_t n, const formula_name_resolver& resolver);
    void set_formula_cell(const abs_address_t& addr, size_t identifier, bool shared);

    abs_range_t get_data_range(sheet_t sheet) const;

    bool is_empty(const abs_address_t& addr) const;
    celltype_t get_celltype(const abs_address_t& addr) const;
    double get_numeric_value(const abs_address_t& addr) const;
    double get_numeric_value_nowait(const abs_address_t& addr) const;
    string_id_t get_string_identifier(const abs_address_t& addr) const;
    string_id_t get_string_identifier_nowait(const abs_address_t& addr) const;
    string_id_t get_string_identifier(const char* p, size_t n) const;
    const formula_cell* get_formula_cell(const abs_address_t& addr) const;
    formula_cell* get_formula_cell(const abs_address_t& addr);

    void set_named_expression(const char* p, size_t n, std::unique_ptr<formula_tokens_t>&& expr);
    void set_named_expression(sheet_t sheet, const char* p, size_t n, std::unique_ptr<formula_tokens_t>&& expr);

    const formula_tokens_t* get_named_expression(const string& name) const;
    const formula_tokens_t* get_named_expression(sheet_t sheet, const string& name) const;

    sheet_t get_sheet_index(const char* p, size_t n) const;
    std::string get_sheet_name(sheet_t sheet) const;
    sheet_size_t get_sheet_size(sheet_t sheet) const;
    sheet_t append_sheet(const char* p, size_t n, row_t row_size, col_t col_size);

    string_id_t append_string(const char* p, size_t n);
    string_id_t add_string(const char* p, size_t n);
    const std::string* get_string(string_id_t identifier) const;
    size_t get_string_count() const;
    void dump_strings() const;

    const column_store_t* get_column(sheet_t sheet, col_t col) const;
    const column_stores_t* get_columns(sheet_t sheet) const;

    const formula_tokens_t* get_formula_tokens(sheet_t sheet, size_t identifier) const;
    size_t add_formula_tokens(sheet_t sheet, formula_tokens_t* p);
    void remove_formula_tokens(sheet_t sheet, size_t identifier);

    void set_shared_formula(
        const abs_address_t& addr, size_t si,
        const char* p_formula, size_t n_formula,
        const formula_name_resolver& resolver);

    void set_shared_formula(
        const abs_address_t& addr, size_t si,
        const char* p_formula, size_t n_formula, const char* p_range, size_t n_range,
        const formula_name_resolver& resolver);

    void set_shared_formula(
        const abs_address_t& addr, size_t si,
        const char* p_formula, size_t n_formula, const abs_range_t& range,
        const formula_name_resolver& resolver);

    void set_shared_formula(const abs_address_t& addr, size_t si);

    const formula_tokens_t* get_shared_formula_tokens(sheet_t sheet, size_t identifier) const;
    size_t set_formula_tokens_shared(sheet_t sheet, size_t identifier);
    abs_range_t get_shared_formula_range(sheet_t sheet, size_t identifier) const;
    void set_shared_formula_range(sheet_t sheet, size_t identifier, const abs_range_t& range);

    double count_range(const abs_range_t& range, const values_t& values_type) const;

    dirty_formula_cells_t get_all_formula_cells() const;

    bool empty() const;

    const worksheet* fetch_sheet(sheet_t sheet_index) const;

private:
    model_context& m_parent;

    workbook m_sheets;

    config* mp_config;
    cell_listener_tracker* mp_cell_listener_tracker;
    iface::table_handler* mp_table_handler;
    detail::named_expressions_t m_named_expressions;

    model_context::session_handler_factory* mp_session_factory;

    formula_tokens_store_type m_tokens;
    model_context::shared_tokens_type m_shared_tokens;
    strings_type m_sheet_names; ///< index to sheet name map.
    string_pool_type m_strings;
    string_map_type m_string_map;
    string m_empty_string;
};

void model_context_impl::set_named_expression(const char* p, size_t n, std::unique_ptr<formula_tokens_t>&& expr)
{
    string name(p, n);
    m_named_expressions.insert(
        detail::named_expressions_t::value_type(std::move(name), std::move(expr)));
}

void model_context_impl::set_named_expression(
    sheet_t sheet, const char* p, size_t n, std::unique_ptr<formula_tokens_t>&& expr)
{
    detail::named_expressions_t& ns = m_sheets.at(sheet).get_named_expressions();
    string name(p, n);
    ns.insert(
        detail::named_expressions_t::value_type(std::move(name), std::move(expr)));
}

const formula_tokens_t* model_context_impl::get_named_expression(const string& name) const
{
    detail::named_expressions_t::const_iterator itr = m_named_expressions.find(name);
    return itr == m_named_expressions.end() ? nullptr : itr->second.get();
}

const formula_tokens_t* model_context_impl::get_named_expression(sheet_t sheet, const string& name) const
{
    const worksheet* ws = fetch_sheet(sheet);

    if (ws)
    {
        const detail::named_expressions_t& ns = ws->get_named_expressions();
        auto it = ns.find(name);
        if (it != ns.end())
            return it->second.get();
    }

    // Search the global scope if not found in the sheet local scope.
    return get_named_expression(name);
}

sheet_t model_context_impl::get_sheet_index(const char* p, size_t n) const
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

std::string model_context_impl::get_sheet_name(sheet_t sheet) const
{
    if (m_sheet_names.size() <= static_cast<size_t>(sheet))
        return std::string();

    return m_sheet_names[sheet];
}

sheet_size_t model_context_impl::get_sheet_size(sheet_t sheet) const
{
    return m_sheets.at(sheet).get_sheet_size();
}

sheet_t model_context_impl::append_sheet(
    const char* p, size_t n, row_t row_size, col_t col_size)
{
    // Check if the new sheet name already exists.
    string new_name(p, n);
    strings_type::const_iterator it =
        std::find(m_sheet_names.begin(), m_sheet_names.end(), new_name);
    if (it != m_sheet_names.end())
    {
        // This sheet name is already taken.
        ostringstream os;
        os << "Sheet name '" << new_name << "' already exists.";
        throw model_context_error(os.str(), model_context_error::sheet_name_conflict);
    }

    // index of the new sheet.
    sheet_t sheet_index = m_sheets.size();

    m_sheet_names.emplace_back(p, n);
    m_sheets.push_back(row_size, col_size);
    return sheet_index;
}

string_id_t model_context_impl::append_string(const char* p, size_t n)
{
    if (!p || !n)
        // Never add an empty or invalid string.
        return empty_string_id;

    string_id_t str_id = m_strings.size();
    m_strings.push_back(make_unique<std::string>(p, n));
    p = m_strings.back()->data();
    mem_str_buf key(p, n);
    m_string_map.insert(string_map_type::value_type(key, str_id));
    return str_id;
}

string_id_t model_context_impl::add_string(const char* p, size_t n)
{
    string_map_type::iterator itr = m_string_map.find(mem_str_buf(p, n));
    if (itr != m_string_map.end())
        return itr->second;

    return append_string(p, n);
}

const std::string* model_context_impl::get_string(string_id_t identifier) const
{
    if (identifier == empty_string_id)
        return &m_empty_string;

    if (identifier >= m_strings.size())
        return nullptr;

    return m_strings[identifier].get();
}

size_t model_context_impl::get_string_count() const
{
    return m_strings.size();
}

void model_context_impl::dump_strings() const
{
    {
        cout << "string count: " << m_strings.size() << endl;
        auto it = m_strings.begin(), ite = m_strings.end();
        for (string_id_t sid = 0; it != ite; ++it, ++sid)
        {
            const std::string& s = **it;
            cout << "* " << sid << ": '" << s << "' (" << (void*)s.data() << ")" << endl;
        }
    }

    {
        cout << "string map count: " << m_string_map.size() << endl;
        auto it = m_string_map.begin(), ite = m_string_map.end();
        for (; it != ite; ++it)
        {
            mem_str_buf key = it->first;
            cout << "* key: '" << key << "' (" << (void*)key.get() << "; " << key.size() << "), value: " << it->second << endl;
        }
    }
}

const column_store_t* model_context_impl::get_column(sheet_t sheet, col_t col) const
{
    if (static_cast<size_t>(sheet) >= m_sheets.size())
        return nullptr;

    const worksheet& sh = m_sheets[sheet];

    if (static_cast<size_t>(col) >= sh.size())
        return nullptr;

    return &sh[col];
}

const column_stores_t* model_context_impl::get_columns(sheet_t sheet) const
{
    if (static_cast<size_t>(sheet) >= m_sheets.size())
        return nullptr;

    const worksheet& sh = m_sheets[sheet];
    return &sh.get_columns();
}

const formula_tokens_t* model_context_impl::get_formula_tokens(sheet_t sheet, size_t identifier) const
{
    if (m_tokens.size() <= identifier)
        return NULL;
    return m_tokens[identifier];
}

size_t model_context_impl::add_formula_tokens(sheet_t sheet, formula_tokens_t* p)
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

void model_context_impl::remove_formula_tokens(sheet_t sheet, size_t identifier)
{
    if (m_tokens.size() >= identifier)
        return;

    delete m_tokens[identifier];
    m_tokens[identifier] = NULL;
}

void model_context_impl::set_shared_formula(
        const abs_address_t& addr, size_t si,
        const char* p_formula, size_t n_formula, const char* p_range, size_t n_range,
        const formula_name_resolver& resolver)
{
    formula_name_t name_type = resolver.resolve(p_range, n_range, abs_address_t());
    abs_range_t range;
    switch (name_type.type)
    {
        case ixion::formula_name_t::cell_reference:
            range.first.sheet = name_type.address.sheet;
            range.first.row = name_type.address.row;
            range.first.column = name_type.address.col;
            range.last = range.first;
        break;
        case ixion::formula_name_t::range_reference:
            range.first.sheet = name_type.range.first.sheet;
            range.first.row = name_type.range.first.row;
            range.first.column = name_type.range.first.col;
            range.last.sheet = name_type.range.last.sheet;
            range.last.row = name_type.range.last.row;
            range.last.column = name_type.range.last.col;
        break;
        default:
        {
            std::ostringstream os;
            os << "failed to resolve shared formula range. ";
            os << "(" << string(p_range, n_range) << ")";
            throw general_error(os.str());
        }
    }

    set_shared_formula(addr, si, p_formula, n_formula, range, resolver);
}

void model_context_impl::set_shared_formula(
        const abs_address_t& addr, size_t si,
        const char* p_formula, size_t n_formula, const abs_range_t& range,
        const formula_name_resolver& resolver)
{
    // Tokenize the formula string and store it.
    unique_ptr<formula_tokens_t> tokens(new formula_tokens_t);
    parse_formula_string(m_parent, addr, resolver, p_formula, n_formula, *tokens);

    if (si >= m_shared_tokens.size())
        m_shared_tokens.resize(si+1);

    m_shared_tokens[si].tokens = tokens.release();
    m_shared_tokens[si].range = range;
}

void model_context_impl::set_shared_formula(
        const abs_address_t& addr, size_t si,
        const char* p_formula, size_t n_formula,
        const formula_name_resolver& resolver)
{
    abs_range_t range;
    range.first = addr;
    range.last = range.first;
    set_shared_formula(addr, si, p_formula, n_formula, range, resolver);
}

const formula_tokens_t* model_context_impl::get_shared_formula_tokens(sheet_t sheet, size_t identifier) const
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

size_t model_context_impl::set_formula_tokens_shared(sheet_t sheet, size_t identifier)
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

abs_range_t model_context_impl::get_shared_formula_range(sheet_t sheet, size_t identifier) const
{
    assert(identifier < m_shared_tokens.size());
    return m_shared_tokens.at(identifier).range;
}

void model_context_impl::set_shared_formula_range(sheet_t sheet, size_t identifier, const abs_range_t& range)
{
    m_shared_tokens.at(identifier).range = range;
}

namespace {

double count_formula_block(
    const column_store_t::const_iterator& itb, size_t offset, size_t len, const values_t& vt)
{
    double ret = 0.0;

    // Inspect each formula cell individually.
    formula_cell** pp = &formula_element_block::at(*itb->data, offset);
    formula_cell** pp_end = pp + len;
    for (; pp != pp_end; ++pp)
    {
        const formula_cell& fc = **pp;
        const formula_result& res = fc.get_result_cache();

        switch (res.get_type())
        {
            case formula_result::result_type::value:
                if (vt.is_numeric())
                    ++ret;
            break;
            case formula_result::result_type::string:
                if (vt.is_string())
                    ++ret;
            case formula_result::result_type::error:
                // TODO : how do we handle error formula cells?
            break;
        }
    }

    return ret;
}

}

double model_context_impl::count_range(const abs_range_t& range, const values_t& values_type) const
{
    if (m_sheets.empty())
        return 0.0;

    double ret = 0.0;
    sheet_t last_sheet = range.last.sheet;
    if (static_cast<size_t>(last_sheet) >= m_sheets.size())
        last_sheet = m_sheets.size() - 1;

    for (sheet_t sheet = range.first.sheet; sheet <= last_sheet; ++sheet)
    {
        const worksheet& ws = m_sheets.at(sheet);
        for (col_t col = range.first.column; col <= range.last.column; ++col)
        {
            const column_store_t& cs = ws.at(col);
            row_t cur_row = range.first.row;
            column_store_t::const_position_type pos = cs.position(cur_row);
            column_store_t::const_iterator itb = pos.first; // block iterator
            column_store_t::const_iterator itb_end = cs.end();
            size_t offset = pos.second;
            if (itb == itb_end)
                continue;

            bool cont = true;
            while (cont)
            {
                // remaining length of current block.
                size_t len = itb->size - offset;
                row_t last_row = cur_row + len - 1;

                if (last_row >= range.last.row)
                {
                    last_row = range.last.row;
                    len = last_row - cur_row + 1;
                    cont = false;
                }

                bool match = false;

                switch (itb->type)
                {
                    case element_type_numeric:
                        match = values_type.is_numeric();
                    break;
                    case element_type_string:
                        match = values_type.is_string();
                    break;
                    case element_type_empty:
                        match = values_type.is_empty();
                    break;
                    case element_type_formula:
                        ret += count_formula_block(itb, offset, len, values_type);
                    break;
                }

                if (match)
                    ret += len;

                // Move to the next block.
                cur_row = last_row + 1;
                ++itb;
                offset = 0;
                if (itb == itb_end)
                    cont = false;
            }
        }
    }

    return ret;
}

dirty_formula_cells_t model_context_impl::get_all_formula_cells() const
{
    dirty_formula_cells_t cells;

    for (size_t sid = 0; sid < m_sheets.size(); ++sid)
    {
        const worksheet& sh = m_sheets[sid];
        for (size_t cid = 0; cid < sh.size(); ++cid)
        {
            const column_store_t& col = sh[cid];
            column_store_t::const_iterator it = col.begin();
            column_store_t::const_iterator ite = col.end();
            for (; it != ite; ++it)
            {
                if (it->type != ixion::element_type_formula)
                    continue;

                row_t row_id = it->position;
                abs_address_t pos(sid, row_id, cid);
                for (size_t i = 0; i < it->size; ++i, ++pos.row)
                    cells.insert(pos);
            }
        }
    }

    return cells;
}

bool model_context_impl::empty() const
{
    return m_sheets.empty();
}

const worksheet* model_context_impl::fetch_sheet(sheet_t sheet_index) const
{
    if (sheet_index < 0 || m_sheets.size() <= size_t(sheet_index))
        return nullptr;

    return &m_sheets[sheet_index];
}

void model_context_impl::erase_cell(const abs_address_t& addr)
{
    worksheet& sheet = m_sheets.at(addr.sheet);
    column_store_t& col_store = sheet.at(addr.column);
    column_store_t::iterator& pos_hint = sheet.get_pos_hint(addr.column);

    mdds::mtv::element_t celltype = col_store.get_type(addr.row);
    if (celltype == element_type_formula)
    {
        const formula_cell* fcell = col_store.get<formula_cell*>(addr.row);
        assert(fcell);
        remove_formula_tokens(addr.sheet, fcell->get_identifier());
    }

    // Just update the hint. This call is not used during import.
    pos_hint = col_store.set_empty(addr.row, addr.row);
}

void model_context_impl::set_numeric_cell(const abs_address_t& addr, double val)
{
    worksheet& sheet = m_sheets.at(addr.sheet);
    column_store_t& col_store = sheet.at(addr.column);
    column_store_t::iterator& pos_hint = sheet.get_pos_hint(addr.column);
    pos_hint = col_store.set(pos_hint, addr.row, val);
}

void model_context_impl::set_boolean_cell(const abs_address_t& addr, bool val)
{
    worksheet& sheet = m_sheets.at(addr.sheet);
    column_store_t& col_store = sheet.at(addr.column);
    column_store_t::iterator& pos_hint = sheet.get_pos_hint(addr.column);
    pos_hint = col_store.set(pos_hint, addr.row, val);
}

void model_context_impl::set_string_cell(const abs_address_t& addr, const char* p, size_t n)
{
    worksheet& sheet = m_sheets.at(addr.sheet);
    string_id_t str_id = add_string(p, n);
    column_store_t& col_store = sheet.at(addr.column);
    column_store_t::iterator& pos_hint = sheet.get_pos_hint(addr.column);
    pos_hint = col_store.set(pos_hint, addr.row, str_id);
}

void model_context_impl::set_string_cell(const abs_address_t& addr, string_id_t identifier)
{
    worksheet& sheet = m_sheets.at(addr.sheet);
    column_store_t& col_store = sheet.at(addr.column);
    column_store_t::iterator& pos_hint = sheet.get_pos_hint(addr.column);
    pos_hint = col_store.set(pos_hint, addr.row, identifier);
}

void model_context_impl::set_formula_cell(
    const abs_address_t& addr, const char* p, size_t n, const formula_name_resolver& resolver)
{
    unique_ptr<formula_tokens_t> tokens(new formula_tokens_t);
    parse_formula_string(m_parent, addr, resolver, p, n, *tokens);
    unique_ptr<formula_cell> fcell(new formula_cell);
    if (!set_shared_formula_tokens_to_cell(m_parent, addr, *fcell, *tokens))
    {
        size_t tkid = add_formula_tokens(0, tokens.release());
        fcell->set_identifier(tkid);
    }

    worksheet& sheet = m_sheets.at(addr.sheet);
    column_store_t& col_store = sheet.at(addr.column);
    column_store_t::iterator& pos_hint = sheet.get_pos_hint(addr.column);
    pos_hint = col_store.set(pos_hint, addr.row, fcell.release());
}

void model_context_impl::set_formula_cell(
    const abs_address_t& addr, size_t identifier, bool shared)
{
    unique_ptr<formula_cell> fcell(new formula_cell(identifier));
    fcell->set_shared(shared);

    worksheet& sheet = m_sheets.at(addr.sheet);
    column_store_t& col_store = sheet.at(addr.column);
    column_store_t::iterator& pos_hint = sheet.get_pos_hint(addr.column);
    pos_hint = col_store.set(pos_hint, addr.row, fcell.release());
}

abs_range_t model_context_impl::get_data_range(sheet_t sheet) const
{
    const worksheet& cols = m_sheets.at(sheet);
    size_t col_size = cols.size();
    if (!col_size)
        return abs_range_t(abs_range_t::invalid);

    row_t row_size = cols[0].size();
    if (!row_size)
        return abs_range_t(abs_range_t::invalid);

    abs_range_t range;
    range.first.column = 0;
    range.first.row = row_size-1;
    range.first.sheet = sheet;
    range.last.column = -1; // if this stays -1 all columns are empty.
    range.last.row = 0;
    range.last.sheet = sheet;

    for (size_t i = 0; i < col_size; ++i)
    {
        const column_store_t& col = cols[i];
        if (col.empty())
        {
            if (range.last.column < 0)
                ++range.first.column;
            continue;
        }

        if (range.first.row > 0)
        {
            // First non-empty row.

            column_store_t::const_iterator it = col.begin(), it_end = col.end();
            assert(it != it_end);
            if (it->type == mdds::mtv::element_type_empty)
            {
                // First block is empty.
                row_t offset = it->size;
                ++it;
                if (it == it_end)
                {
                    // The whole column is empty.
                    if (range.last.column < 0)
                        ++range.first.column;
                    continue;
                }

                assert(it->type != mdds::mtv::element_type_empty);
                if (range.first.row > offset)
                    range.first.row = offset;
            }
            else
                // Set the first row to 0, and lock it.
                range.first.row = 0;

            range.last.column = i;
        }

        if (range.last.row < (row_size-1))
        {
            // Last non-empty row.

            column_store_t::const_reverse_iterator it = col.rbegin(), it_end = col.rend();
            assert(it != it_end);
            if (it->type == mdds::mtv::element_type_empty)
            {
                // Last block is empty.
                size_t size_last_block = it->size;
                ++it;
                if (it == it_end)
                {
                    // The whole column is empty.
                    if (range.last.column < 0)
                        ++range.first.column;
                    continue;
                }

                assert(it->type != mdds::mtv::element_type_empty);
                row_t last_data_row = static_cast<row_t>(col.size() - size_last_block - 1);
                if (range.last.row < last_data_row)
                    range.last.row = last_data_row;
            }
            else
                // Last block is not empty.
                range.last.row = row_size - 1;

            range.last.column = i;
        }

    }

    if (range.last.column < 0)
        // No data column found.  The whole sheet is empty.
        return abs_range_t(abs_range_t::invalid);

    return range;
}

bool model_context_impl::is_empty(const abs_address_t& addr) const
{
    return m_sheets.at(addr.sheet).at(addr.column).is_empty(addr.row);
}

celltype_t model_context_impl::get_celltype(const abs_address_t& addr) const
{
    mdds::mtv::element_t gmcell_type =
        m_sheets.at(addr.sheet).at(addr.column).get_type(addr.row);
    switch (gmcell_type)
    {
        case mdds::mtv::element_type_empty:
            return celltype_t::empty;
        case mdds::mtv::element_type_numeric:
            return celltype_t::numeric;
        case mdds::mtv::element_type_ulong:
            return celltype_t::string;
        case element_type_formula:
            return celltype_t::formula;
        default:
            throw general_error("unknown cell type");
    }

    return celltype_t::unknown;
}

double model_context_impl::get_numeric_value(const abs_address_t& addr) const
{
    const column_store_t& col_store = m_sheets.at(addr.sheet).at(addr.column);
    switch (col_store.get_type(addr.row))
    {
        case mdds::mtv::element_type_numeric:
            return col_store.get<double>(addr.row);
        case element_type_formula:
        {
            const formula_cell* p = col_store.get<formula_cell*>(addr.row);
            return p->get_value();
        }
        break;
        default:
            ;
    }
    return 0.0;
}

double model_context_impl::get_numeric_value_nowait(const abs_address_t& addr) const
{
    const column_store_t& col_store = m_sheets.at(addr.sheet).at(addr.column);
    switch (col_store.get_type(addr.row))
    {
        case element_type_numeric:
            return col_store.get<double>(addr.row);
        case element_type_formula:
        {
            const formula_cell* p = col_store.get<formula_cell*>(addr.row);
            return p->get_value_nowait();
        }
        break;
        default:
            ;
    }
    return 0.0;
}

string_id_t model_context_impl::get_string_identifier(const abs_address_t& addr) const
{
    const column_store_t& col_store = m_sheets.at(addr.sheet).at(addr.column);
    switch (col_store.get_type(addr.row))
    {
        case ixion::element_type_string:
            return col_store.get<string_id_t>(addr.row);
        default:
            ;
    }
    return empty_string_id;
}

string_id_t model_context_impl::get_string_identifier_nowait(const abs_address_t& addr) const
{
    const column_store_t& col_store = m_sheets.at(addr.sheet).at(addr.column);
    switch (col_store.get_type(addr.row))
    {
        case ixion::element_type_string:
            return col_store.get<string_id_t>(addr.row);
        case ixion::element_type_formula:
        {
            const formula_cell* p = col_store.get<formula_cell*>(addr.row);
            const formula_result* res_cache = p->get_result_cache_nowait();
            if (!res_cache)
                break;

            switch (res_cache->get_type())
            {
                case formula_result::result_type::string:
                    return res_cache->get_string();
                case formula_result::result_type::error:
                    // TODO : perhaps we should return the error string here.
                default:
                    ;
            }
            break;
        }
        default:
            ;
    }
    return empty_string_id;
}

string_id_t model_context_impl::get_string_identifier(const char* p, size_t n) const
{
    string_map_type::const_iterator it = m_string_map.find(mem_str_buf(p, n));
    return it == m_string_map.end() ? empty_string_id : it->second;
}

const formula_cell* model_context_impl::get_formula_cell(const abs_address_t& addr) const
{
    const column_store_t& col_store = m_sheets.at(addr.sheet).at(addr.column);
    if (col_store.get_type(addr.row) != element_type_formula)
        return NULL;

    return col_store.get<formula_cell*>(addr.row);
}

formula_cell* model_context_impl::get_formula_cell(const abs_address_t& addr)
{
    column_store_t& col_store = m_sheets.at(addr.sheet).at(addr.column);
    if (col_store.get_type(addr.row) != element_type_formula)
        return NULL;

    return col_store.get<formula_cell*>(addr.row);
}

std::unique_ptr<iface::session_handler> model_context::session_handler_factory::create()
{
    return std::unique_ptr<iface::session_handler>();
}

model_context::shared_tokens::shared_tokens() : tokens(NULL) {}
model_context::shared_tokens::shared_tokens(formula_tokens_t* _tokens) : tokens(_tokens) {}
model_context::shared_tokens::shared_tokens(const shared_tokens& r) : tokens(r.tokens), range(r.range) {}

bool model_context::shared_tokens::operator== (const shared_tokens& r) const
{
    return tokens == r.tokens && range == r.range;
}

model_context::model_context() :
    mp_impl(new model_context_impl(*this)) {}

model_context::~model_context()
{
    delete mp_impl;
}

const config& model_context::get_config() const
{
    return mp_impl->get_config();
}

cell_listener_tracker& model_context::get_cell_listener_tracker()
{
    return mp_impl->get_cell_listener_tracker();
}

void model_context::erase_cell(const abs_address_t& addr)
{
    mp_impl->erase_cell(addr);
}

void model_context::set_numeric_cell(const abs_address_t& addr, double val)
{
    mp_impl->set_numeric_cell(addr, val);
}

void model_context::set_boolean_cell(const abs_address_t& addr, bool val)
{
    mp_impl->set_boolean_cell(addr, val);
}

void model_context::set_string_cell(const abs_address_t& addr, const char* p, size_t n)
{
    mp_impl->set_string_cell(addr, p, n);
}

void model_context::set_string_cell(const abs_address_t& addr, string_id_t identifier)
{
    mp_impl->set_string_cell(addr, identifier);
}

void model_context::set_formula_cell(
    const abs_address_t& addr, const char* p, size_t n, const formula_name_resolver& resolver)
{
    mp_impl->set_formula_cell(addr, p, n, resolver);
}

void model_context::set_formula_cell(
    const abs_address_t& addr, size_t identifier, bool shared)
{
    mp_impl->set_formula_cell(addr, identifier, shared);
}

abs_range_t model_context::get_data_range(sheet_t sheet) const
{
    return mp_impl->get_data_range(sheet);
}

bool model_context::is_empty(const abs_address_t& addr) const
{
    return mp_impl->is_empty(addr);
}

celltype_t model_context::get_celltype(const abs_address_t& addr) const
{
    return mp_impl->get_celltype(addr);
}

double model_context::get_numeric_value(const abs_address_t& addr) const
{
    return mp_impl->get_numeric_value(addr);
}

double model_context::get_numeric_value_nowait(const abs_address_t& addr) const
{
    return mp_impl->get_numeric_value_nowait(addr);
}

string_id_t model_context::get_string_identifier(const abs_address_t& addr) const
{
    return mp_impl->get_string_identifier(addr);
}

string_id_t model_context::get_string_identifier_nowait(const abs_address_t& addr) const
{
    return mp_impl->get_string_identifier_nowait(addr);
}

string_id_t model_context::get_string_identifier(const char* p, size_t n) const
{
    return mp_impl->get_string_identifier(p, n);
}

const formula_cell* model_context::get_formula_cell(const abs_address_t& addr) const
{
    return mp_impl->get_formula_cell(addr);
}

formula_cell* model_context::get_formula_cell(const abs_address_t& addr)
{
    return mp_impl->get_formula_cell(addr);
}

double model_context::count_range(const abs_range_t& range, const values_t& values_type) const
{
    return mp_impl->count_range(range, values_type);
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
            double val = get_numeric_value(abs_address_t(range.first.sheet, row, col));

            // TODO: we need to handle string types when that becomes available.
            ret.set(i, j, val);
        }
    }
    return ret;
}

std::unique_ptr<iface::session_handler> model_context::create_session_handler()
{
    return mp_impl->create_session_handler();
}

iface::table_handler* model_context::get_table_handler()
{
    return mp_impl->get_table_handler();
}

const iface::table_handler* model_context::get_table_handler() const
{
    return mp_impl->get_table_handler();
}

const formula_tokens_t* model_context::get_formula_tokens(sheet_t sheet, size_t identifier) const
{
    return mp_impl->get_formula_tokens(sheet, identifier);
}

size_t model_context::add_formula_tokens(sheet_t sheet, formula_tokens_t* p)
{
    return mp_impl->add_formula_tokens(sheet, p);
}

void model_context::remove_formula_tokens(sheet_t sheet, size_t identifier)
{
    mp_impl->remove_formula_tokens(sheet, identifier);
}

void model_context::set_shared_formula(
        const abs_address_t& addr, size_t si,
        const char* p_formula, size_t n_formula, const char* p_range, size_t n_range,
        const formula_name_resolver& resolver)
{
    mp_impl->set_shared_formula(addr, si, p_formula, n_formula, p_range, n_range, resolver);
}

void model_context::set_shared_formula(
        const abs_address_t& addr, size_t si,
        const char* p_formula, size_t n_formula,
        const formula_name_resolver& resolver)
{
    mp_impl->set_shared_formula(addr, si, p_formula, n_formula, resolver);
}

const formula_tokens_t* model_context::get_shared_formula_tokens(sheet_t sheet, size_t identifier) const
{
    return mp_impl->get_shared_formula_tokens(sheet, identifier);
}

size_t model_context::set_formula_tokens_shared(sheet_t sheet, size_t identifier)
{
    return mp_impl->set_formula_tokens_shared(sheet, identifier);
}

abs_range_t model_context::get_shared_formula_range(sheet_t sheet, size_t identifier) const
{
    return mp_impl->get_shared_formula_range(sheet, identifier);
}

void model_context::set_shared_formula_range(sheet_t sheet, size_t identifier, const abs_range_t& range)
{
    return mp_impl->set_shared_formula_range(sheet, identifier, range);
}

string_id_t model_context::append_string(const char* p, size_t n)
{
    return mp_impl->append_string(p, n);
}

string_id_t model_context::add_string(const char* p, size_t n)
{
    return mp_impl->add_string(p, n);
}

const std::string* model_context::get_string(string_id_t identifier) const
{
    return mp_impl->get_string(identifier);
}

sheet_t model_context::get_sheet_index(const char* p, size_t n) const
{
    return mp_impl->get_sheet_index(p, n);
}

std::string model_context::get_sheet_name(sheet_t sheet) const
{
    return mp_impl->get_sheet_name(sheet);
}

sheet_size_t model_context::get_sheet_size(sheet_t sheet) const
{
    return mp_impl->get_sheet_size(sheet);
}

void model_context::set_named_expression(const char* p, size_t n, std::unique_ptr<formula_tokens_t>&& expr)
{
    mp_impl->set_named_expression(p, n, std::move(expr));
}

void model_context::set_named_expression(
    sheet_t sheet, const char* p, size_t n, std::unique_ptr<formula_tokens_t>&& expr)
{
    mp_impl->set_named_expression(sheet, p, n, std::move(expr));
}

const formula_tokens_t* model_context::get_named_expression(const string& name) const
{
    return mp_impl->get_named_expression(name);
}

const formula_tokens_t* model_context::get_named_expression(sheet_t sheet, const string& name) const
{
    return mp_impl->get_named_expression(sheet, name);
}

sheet_t model_context::append_sheet(const char* p, size_t n, row_t row_size, col_t col_size)
{
    return mp_impl->append_sheet(p, n, row_size, col_size);
}

void model_context::set_session_handler_factory(session_handler_factory* factory)
{
    mp_impl->set_session_handler_factory(factory);
}

void model_context::set_table_handler(iface::table_handler* handler)
{
    mp_impl->set_table_handler(handler);
}

size_t model_context::get_string_count() const
{
    return mp_impl->get_string_count();
}

void model_context::dump_strings() const
{
    mp_impl->dump_strings();
}

const column_store_t* model_context::get_column(sheet_t sheet, col_t col) const
{
    return mp_impl->get_column(sheet, col);
}

const column_stores_t* model_context::get_columns(sheet_t sheet) const
{
    return mp_impl->get_columns(sheet);
}

dirty_formula_cells_t model_context::get_all_formula_cells() const
{
    return mp_impl->get_all_formula_cells();
}

bool model_context::empty() const
{
    return mp_impl->empty();
}

}
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
