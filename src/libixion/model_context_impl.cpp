/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "model_context_impl.hpp"

#include "ixion/address.hpp"
#include "ixion/cell.hpp"
#include "ixion/formula_result.hpp"
#include "ixion/matrix.hpp"
#include "ixion/interface/session_handler.hpp"
#include "ixion/model_iterator.hpp"

#include "calc_status.hpp"
#include "model_types.hpp"
#include "utils.hpp"
#include "debug.hpp"

#include <sstream>
#include <iostream>
#include <cstring>

using std::cout;
using std::endl;

namespace ixion { namespace detail {

string_id_t safe_string_pool::append_string_unsafe(std::string_view s)
{
    assert(!s.empty());

    string_id_t str_id = m_strings.size();
    m_strings.push_back(std::make_unique<std::string>(s));
    s = *m_strings.back();
    m_string_map.insert(string_map_type::value_type(s, str_id));
    return str_id;
}

string_id_t safe_string_pool::append_string(std::string_view s)
{
    if (s.empty())
        // Never add an empty or invalid string.
        return empty_string_id;

    std::unique_lock<std::mutex> lock(m_mtx);
    return append_string_unsafe(s);
}

string_id_t safe_string_pool::add_string(std::string_view s)
{
    if (s.empty())
        // Never add an empty or invalid string.
        return empty_string_id;

    std::unique_lock<std::mutex> lock(m_mtx);
    string_map_type::iterator itr = m_string_map.find(s);
    if (itr != m_string_map.end())
        return itr->second;

    return append_string_unsafe(s);
}

const std::string* safe_string_pool::get_string(string_id_t identifier) const
{
    if (identifier == empty_string_id)
        return &m_empty_string;

    if (identifier >= m_strings.size())
        return nullptr;

    return m_strings[identifier].get();
}

size_t safe_string_pool::size() const
{
    return m_strings.size();
}

void safe_string_pool::dump_strings() const
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
            std::string_view key = it->first;
            cout << "* key: '" << key << "' (" << (void*)key.data() << "; " << key.size() << "), value: " << it->second << endl;
        }
    }
}

string_id_t safe_string_pool::get_identifier_from_string(std::string_view s) const
{
    string_map_type::const_iterator it = m_string_map.find(s);
    return it == m_string_map.end() ? empty_string_id : it->second;
}

namespace {

model_context::session_handler_factory dummy_session_handler_factory;

rc_size_t to_group_size(const abs_range_t& group_range)
{
    rc_size_t group_size;
    group_size.row    = group_range.last.row - group_range.first.row + 1;
    group_size.column = group_range.last.column - group_range.first.column + 1;
    return group_size;
}

void set_grouped_formula_cells_to_workbook(
    workbook& wb, const abs_address_t& top_left, const rc_size_t& group_size,
    const calc_status_ptr_t& cs, const formula_tokens_store_ptr_t& ts)
{
    worksheet& sheet = wb.at(top_left.sheet);

    for (col_t col_offset = 0; col_offset < group_size.column; ++col_offset)
    {
        col_t col = top_left.column + col_offset;
        column_store_t& col_store = sheet.at(col);
        column_store_t::iterator& pos_hint = sheet.get_pos_hint(col);

        for (row_t row_offset = 0; row_offset < group_size.row; ++row_offset)
        {
            row_t row = top_left.row + row_offset;
            pos_hint = col_store.set(pos_hint, row, new formula_cell(row_offset, col_offset, cs, ts));
        }
    }
}

/**
 * The name of a named expression can only contain letters, numbers or an
 * underscore character.
 */
void check_named_exp_name_or_throw(const char* p, size_t n)
{
    const char* p_end = p + n;

    if (p == p_end)
        throw model_context_error(
            "empty name is not allowed", model_context_error::invalid_named_expression);

    if ('0' <= *p && *p <= '9')
        throw model_context_error(
            "name cannot start with a numeric character", model_context_error::invalid_named_expression);

    if (*p == '.')
        throw model_context_error(
            "name cannot start with a dot", model_context_error::invalid_named_expression);

    for (; p != p_end; ++p)
    {
        char c = *p;
        if ('a' <= c && c <= 'z')
            continue;

        if ('A' <= c && c <= 'Z')
            continue;

        if ('0' <= c && c <= '9')
            continue;

        if (c == '_' || c == '.')
            continue;

        std::ostringstream os;
        os << "name contains invalid character '" << c << "'";
        throw model_context_error(os.str(), model_context_error::invalid_named_expression);
    }
}

} // anonymous namespace

model_context_impl::model_context_impl(model_context& parent, const rc_size_t& sheet_size) :
    m_parent(parent),
    m_sheet_size(sheet_size),
    m_tracker(),
    mp_table_handler(nullptr),
    mp_session_factory(&dummy_session_handler_factory),
    m_formula_res_wait_policy(formula_result_wait_policy_t::throw_exception)
{
}

model_context_impl::~model_context_impl() {}

void model_context_impl::notify(formula_event_t event)
{
    switch (event)
    {
        case formula_event_t::calculation_begins:
            m_formula_res_wait_policy = formula_result_wait_policy_t::block_until_done;
            break;
        case formula_event_t::calculation_ends:
            m_formula_res_wait_policy = formula_result_wait_policy_t::throw_exception;
            break;
    }
}

void model_context_impl::set_named_expression(
    const char* p, size_t n, const abs_address_t& origin, formula_tokens_t&& expr)
{
    check_named_exp_name_or_throw(p, n);

    std::string name(p, n);
    IXION_TRACE("named expression: name='" << name << "'");
    m_named_expressions.insert(
        detail::named_expressions_t::value_type(
            std::move(name),
            named_expression_t(origin, std::move(expr))
        )
    );
}

void model_context_impl::set_named_expression(
    sheet_t sheet, const char* p, size_t n, const abs_address_t& origin, formula_tokens_t&& expr)
{
    check_named_exp_name_or_throw(p, n);

    detail::named_expressions_t& ns = m_sheets.at(sheet).get_named_expressions();
    std::string name(p, n);
    IXION_TRACE("named expression: name='" << name << "'");
    ns.insert(
        detail::named_expressions_t::value_type(
            std::move(name),
            named_expression_t(origin, std::move(expr))
        )
    );
}

const named_expression_t* model_context_impl::get_named_expression(std::string_view name) const
{
    named_expressions_t::const_iterator itr = m_named_expressions.find(std::string(name));
    return itr == m_named_expressions.end() ? nullptr : &itr->second;
}

const named_expression_t* model_context_impl::get_named_expression(sheet_t sheet, std::string_view name) const
{
    const worksheet* ws = fetch_sheet(sheet);

    if (ws)
    {
        const named_expressions_t& ns = ws->get_named_expressions();
        auto it = ns.find(std::string(name));
        if (it != ns.end())
            return &it->second;
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

rc_size_t model_context_impl::get_sheet_size() const
{
    return m_sheet_size;
}

size_t model_context_impl::get_sheet_count() const
{
    return m_sheets.size();
}

sheet_t model_context_impl::append_sheet(std::string&& name)
{
    IXION_TRACE("name='" << name << "'");

    // Check if the new sheet name already exists.
    strings_type::const_iterator it =
        std::find(m_sheet_names.begin(), m_sheet_names.end(), name);
    if (it != m_sheet_names.end())
    {
        // This sheet name is already taken.
        std::ostringstream os;
        os << "Sheet name '" << name << "' already exists.";
        throw model_context_error(os.str(), model_context_error::sheet_name_conflict);
    }

    // index of the new sheet.
    sheet_t sheet_index = m_sheets.size();

    m_sheet_names.push_back(std::move(name));
    m_sheets.push_back(m_sheet_size.row, m_sheet_size.column);
    return sheet_index;
}

void model_context_impl::set_cell_values(sheet_t sheet, std::initializer_list<model_context::input_row>&& rows)
{
    abs_address_t pos;
    pos.sheet = sheet;
    pos.row = 0;
    pos.column = 0;

    // TODO : This function is not optimized for speed as it is mainly for
    // convenience.  Decide if we need to optimize this later.

    for (const model_context::input_row& row : rows)
    {
        pos.column = 0;

        for (const model_context::input_cell& c : row.cells())
        {
            switch (c.type)
            {
                case celltype_t::numeric:
                    set_numeric_cell(pos, c.value.numeric);
                    break;
                case celltype_t::string:
                    set_string_cell(pos, c.value.string, std::strlen(c.value.string));
                    break;
                case celltype_t::boolean:
                    set_boolean_cell(pos, c.value.boolean);
                    break;
                default:
                    ;
            }

            ++pos.column;
        }

        ++pos.row;
    }
}

string_id_t model_context_impl::append_string(std::string_view s)
{
    return m_str_pool.append_string(s);
}

string_id_t model_context_impl::add_string(std::string_view s)
{
    return m_str_pool.add_string(s);
}

const std::string* model_context_impl::get_string(string_id_t identifier) const
{
    return m_str_pool.get_string(identifier);
}

size_t model_context_impl::get_string_count() const
{
    return m_str_pool.size();
}

void model_context_impl::dump_strings() const
{
    m_str_pool.dump_strings();
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

namespace {

double count_formula_block(
    formula_result_wait_policy_t wait_policy, const column_store_t::const_iterator& itb, size_t offset, size_t len, const values_t& vt)
{
    double ret = 0.0;

    // Inspect each formula cell individually.
    formula_cell** pp = &formula_element_block::at(*itb->data, offset);
    formula_cell** pp_end = pp + len;
    for (; pp != pp_end; ++pp)
    {
        const formula_cell& fc = **pp;
        formula_result res = fc.get_result_cache(wait_policy);

        switch (res.get_type())
        {
            case formula_result::result_type::value:
                if (vt.is_numeric())
                    ++ret;
                break;
            case formula_result::result_type::string:
                if (vt.is_string())
                    ++ret;
                break;
            case formula_result::result_type::error:
                // TODO : how do we handle error formula cells?
                break;
            case formula_result::result_type::matrix:
                // TODO : ditto
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
                    case element_type_boolean:
                        match = values_type.is_boolean();
                        break;
                    case element_type_string:
                        match = values_type.is_string();
                        break;
                    case element_type_empty:
                        match = values_type.is_empty();
                        break;
                    case element_type_formula:
                        ret += count_formula_block(m_formula_res_wait_policy, itb, offset, len, values_type);
                        break;
                    default:
                    {
                        std::ostringstream os;
                        os << __FUNCTION__ << ": unhandled block type (" << itb->type << ")";
                        throw general_error(os.str());
                    }
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

abs_address_set_t model_context_impl::get_all_formula_cells() const
{
    abs_address_set_t cells;

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
                if (it->type != element_type_formula)
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

column_store_t::const_position_type model_context_impl::get_cell_position(const abs_address_t& addr) const
{
    const column_store_t& col_store = m_sheets.at(addr.sheet).at(addr.column);
    return col_store.position(addr.row);
}

const detail::named_expressions_t& model_context_impl::get_named_expressions() const
{
    return m_named_expressions;
}

const detail::named_expressions_t& model_context_impl::get_named_expressions(sheet_t sheet) const
{
    const worksheet& sh = m_sheets.at(sheet);
    return sh.get_named_expressions();
}

model_iterator model_context_impl::get_model_iterator(
    sheet_t sheet, rc_direction_t dir, const abs_rc_range_t& range) const
{
    return model_iterator(*this, sheet, range, dir);
}

void model_context_impl::set_sheet_size(const rc_size_t& sheet_size)
{
    if (!m_sheets.empty())
        throw model_context_error(
            "You cannot change the sheet size if you already have at least one existing sheet.",
            model_context_error::sheet_size_locked);

    m_sheet_size = sheet_size;
}

std::unique_ptr<iface::session_handler> model_context_impl::create_session_handler()
{
    return mp_session_factory->create();
}

void model_context_impl::empty_cell(const abs_address_t& addr)
{
    worksheet& sheet = m_sheets.at(addr.sheet);
    column_store_t& col_store = sheet.at(addr.column);
    column_store_t::iterator& pos_hint = sheet.get_pos_hint(addr.column);
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
    string_id_t str_id = add_string({p, n});
    column_store_t& col_store = sheet.at(addr.column);
    column_store_t::iterator& pos_hint = sheet.get_pos_hint(addr.column);
    pos_hint = col_store.set(pos_hint, addr.row, str_id);
}

void model_context_impl::fill_down_cells(const abs_address_t& src, size_t n_dst)
{
    if (!n_dst)
        // Destination cell length is 0.  Nothing to copy to.
        return;

    worksheet& sheet = m_sheets.at(src.sheet);
    column_store_t& col_store = sheet.at(src.column);
    column_store_t::iterator& pos_hint = sheet.get_pos_hint(src.column);

    column_store_t::const_position_type pos = col_store.position(pos_hint, src.row);
    auto it = pos.first; // block iterator

    switch (it->type)
    {
        case element_type_numeric:
        {
            double v = col_store.get<numeric_element_block>(pos);
            std::vector<double> vs(n_dst, v);
            pos_hint = col_store.set(pos_hint, src.row+1, vs.begin(), vs.end());
            break;
        }
        case element_type_boolean:
        {
            bool b = col_store.get<boolean_element_block>(pos);
            std::deque<bool> vs(n_dst, b);
            pos_hint = col_store.set(pos_hint, src.row+1, vs.begin(), vs.end());
            break;
        }
        case element_type_string:
        {
            string_id_t sid = col_store.get<string_element_block>(pos);
            std::vector<string_id_t> vs(n_dst, sid);
            pos_hint = col_store.set(pos_hint, src.row+1, vs.begin(), vs.end());
            break;
        }
        case element_type_empty:
        {
            size_t start_pos = src.row + 1;
            size_t end_pos = start_pos + n_dst - 1;
            pos_hint = col_store.set_empty(pos_hint, start_pos, end_pos);
            break;
        }
        case element_type_formula:
            // TODO : support this.
            throw not_implemented_error("filling down of a formula cell is not yet supported.");
        default:
        {
            std::ostringstream os;
            os << __FUNCTION__ << ": unhandled block type (" << it->type << ")";
            throw general_error(os.str());
        }
    }
}

void model_context_impl::set_string_cell(const abs_address_t& addr, string_id_t identifier)
{
    worksheet& sheet = m_sheets.at(addr.sheet);
    column_store_t& col_store = sheet.at(addr.column);
    column_store_t::iterator& pos_hint = sheet.get_pos_hint(addr.column);
    pos_hint = col_store.set(pos_hint, addr.row, identifier);
}

formula_cell* model_context_impl::set_formula_cell(
    const abs_address_t& addr, const formula_tokens_store_ptr_t& tokens)
{
    std::unique_ptr<formula_cell> fcell = std::make_unique<formula_cell>(tokens);

    worksheet& sheet = m_sheets.at(addr.sheet);
    column_store_t& col_store = sheet.at(addr.column);
    column_store_t::iterator& pos_hint = sheet.get_pos_hint(addr.column);
    formula_cell* p = fcell.release();
    pos_hint = col_store.set(pos_hint, addr.row, p);
    return p;
}

formula_cell* model_context_impl::set_formula_cell(
    const abs_address_t& addr, const formula_tokens_store_ptr_t& tokens, formula_result result)
{
    std::unique_ptr<formula_cell> fcell = std::make_unique<formula_cell>(tokens);

    worksheet& sheet = m_sheets.at(addr.sheet);
    column_store_t& col_store = sheet.at(addr.column);
    column_store_t::iterator& pos_hint = sheet.get_pos_hint(addr.column);
    formula_cell* p = fcell.release();
    p->set_result_cache(std::move(result));
    pos_hint = col_store.set(pos_hint, addr.row, p);
    return p;
}

void model_context_impl::set_grouped_formula_cells(
    const abs_range_t& group_range, formula_tokens_t tokens)
{
    formula_tokens_store_ptr_t ts = formula_tokens_store::create();
    ts->get() = std::move(tokens);

    rc_size_t group_size = to_group_size(group_range);
    calc_status_ptr_t cs(new calc_status(group_size));
    set_grouped_formula_cells_to_workbook(m_sheets, group_range.first, group_size, cs, ts);
}

void model_context_impl::set_grouped_formula_cells(
    const abs_range_t& group_range, formula_tokens_t tokens, formula_result result)
{
    formula_tokens_store_ptr_t ts = formula_tokens_store::create();
    ts->get() = std::move(tokens);

    rc_size_t group_size = to_group_size(group_range);

    if (result.get_type() != formula_result::result_type::matrix)
        throw std::invalid_argument("cached result for grouped formula cells must be of matrix type.");

    if (row_t(result.get_matrix().row_size()) != group_size.row || col_t(result.get_matrix().col_size()) != group_size.column)
        throw std::invalid_argument("dimension of the cached result differs from the size of the group.");

    calc_status_ptr_t cs(new calc_status(group_size));
    cs->result = std::make_unique<formula_result>(std::move(result));
    set_grouped_formula_cells_to_workbook(m_sheets, group_range.first, group_size, cs, ts);
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
            if (it->type == element_type_empty)
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

                assert(it->type != element_type_empty);
                if (range.first.row > offset)
                    range.first.row = offset;
            }
            else
                // Set the first row to 0, and lock it.
                range.first.row = 0;
        }

        if (range.last.row < (row_size-1))
        {
            // Last non-empty row.

            column_store_t::const_reverse_iterator it = col.rbegin(), it_end = col.rend();
            assert(it != it_end);
            if (it->type == element_type_empty)
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

                assert(it->type != element_type_empty);
                row_t last_data_row = static_cast<row_t>(col.size() - size_last_block - 1);
                if (range.last.row < last_data_row)
                    range.last.row = last_data_row;
            }
            else
                // Last block is not empty.
                range.last.row = row_size - 1;
        }

        // Check if the column contains at least one non-empty cell.
        if (col.block_size() > 1 || !col.is_empty(0))
            range.last.column = i;
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

    return detail::to_celltype(gmcell_type);
}

double model_context_impl::get_numeric_value(const abs_address_t& addr) const
{
    const column_store_t& col_store = m_sheets.at(addr.sheet).at(addr.column);
    auto pos = col_store.position(addr.row);

    switch (pos.first->type)
    {
        case element_type_numeric:
            return numeric_element_block::at(*pos.first->data, pos.second);
        case element_type_boolean:
        {
            auto it = boolean_element_block::cbegin(*pos.first->data);
            std::advance(it, pos.second);
            return *it ? 1.0 : 0.0;
        }
        case element_type_formula:
        {
            const formula_cell* p = formula_element_block::at(*pos.first->data, pos.second);
            return p->get_value(m_formula_res_wait_policy);
        }
        default:
            ;
    }
    return 0.0;
}

bool model_context_impl::get_boolean_value(const abs_address_t& addr) const
{
    const column_store_t& col_store = m_sheets.at(addr.sheet).at(addr.column);
    auto pos = col_store.position(addr.row);

    switch (pos.first->type)
    {
        case element_type_numeric:
            return numeric_element_block::at(*pos.first->data, pos.second) != 0.0 ? true : false;
        case element_type_boolean:
        {
            auto it = boolean_element_block::cbegin(*pos.first->data);
            std::advance(it, pos.second);
            return *it;
        }
        case element_type_formula:
        {
            const formula_cell* p = formula_element_block::at(*pos.first->data, pos.second);
            return p->get_value(m_formula_res_wait_policy) == 0.0 ? false : true;
        }
        default:
            ;
    }
    return false;
}

string_id_t model_context_impl::get_string_identifier(const abs_address_t& addr) const
{
    const column_store_t& col_store = m_sheets.at(addr.sheet).at(addr.column);
    auto pos = col_store.position(addr.row);

    switch (pos.first->type)
    {
        case element_type_string:
            return string_element_block::at(*pos.first->data, pos.second);
        default:
            ;
    }
    return empty_string_id;
}

const std::string* model_context_impl::get_string_value(const abs_address_t& addr) const
{
    const column_store_t& col_store = m_sheets.at(addr.sheet).at(addr.column);
    auto pos = col_store.position(addr.row);

    switch (pos.first->type)
    {
        case element_type_string:
        {
            string_id_t sid = string_element_block::at(*pos.first->data, pos.second);
            return m_str_pool.get_string(sid);
        }
        case element_type_formula:
        {
            const formula_cell* p = formula_element_block::at(*pos.first->data, pos.second);
            return p->get_string(m_formula_res_wait_policy);
        }
        case element_type_empty:
            return &empty_string;
        default:
            ;
    }

    return nullptr;
}

string_id_t model_context_impl::get_identifier_from_string(const char* p, size_t n) const
{
    return m_str_pool.get_identifier_from_string({p, n});
}

const formula_cell* model_context_impl::get_formula_cell(const abs_address_t& addr) const
{
    const column_store_t& col_store = m_sheets.at(addr.sheet).at(addr.column);
    auto pos = col_store.position(addr.row);

    if (pos.first->type != element_type_formula)
        return nullptr;

    return formula_element_block::at(*pos.first->data, pos.second);
}

formula_cell* model_context_impl::get_formula_cell(const abs_address_t& addr)
{
    column_store_t& col_store = m_sheets.at(addr.sheet).at(addr.column);
    auto pos = col_store.position(addr.row);

    if (pos.first->type != element_type_formula)
        return nullptr;

    return formula_element_block::at(*pos.first->data, pos.second);
}

formula_result model_context_impl::get_formula_result(const abs_address_t& addr) const
{
    const formula_cell* fc = get_formula_cell(addr);
    if (!fc)
        throw general_error("not a formula cell.");

    return fc->get_result_cache(m_formula_res_wait_policy);
}

}}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
