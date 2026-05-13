/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <ixion/model_cell_range.hpp>
#include <ixion/global.hpp>
#include <ixion/exceptions.hpp>
#include "model_context_impl.hpp"

#include <mdds/multi_type_vector/collection.hpp>
#include <cassert>
#include <ostream>
#include <sstream>

namespace ixion {

model_cell_range::cell::cell() : row(0), col(0), type(cell_t::empty), value(false) {}

model_cell_range::cell::cell(row_t _row, col_t _col) :
    row(_row), col(_col), type(cell_t::empty), value(false) {}

model_cell_range::cell::cell(row_t _row, col_t _col, bool _b) :
    row(_row), col(_col), type(cell_t::boolean), value(_b) {}

model_cell_range::cell::cell(row_t _row, col_t _col, std::string_view _s) :
    row(_row), col(_col), type(cell_t::string), value(_s) {}

model_cell_range::cell::cell(row_t _row, col_t _col, double _v) :
    row(_row), col(_col), type(cell_t::numeric), value(_v) {}

model_cell_range::cell::cell(row_t _row, col_t _col, const formula_cell* _f) :
    row(_row), col(_col), type(cell_t::formula), value(_f) {}

bool model_cell_range::cell::operator== (const cell& other) const
{
    if (type != other.type || row != other.row || col != other.col)
        return false;

    return value == other.value;
}

class model_cell_range::core
{
public:
    virtual bool has() const = 0;
    virtual void next() = 0;
    virtual const model_cell_range::cell& get() const = 0;
    virtual ~core() {}
};

namespace {

class iterator_core_horizontal : public model_cell_range::core
{
    using collection_type = mdds::mtv::collection<column_store_t>;

    const detail::model_context_impl& m_cxt;
    collection_type m_collection;
    mutable model_cell_range::cell m_current_cell;
    mutable bool m_update_current_cell;
    collection_type::const_iterator m_current_pos;
    collection_type::const_iterator m_end;

    void update_current() const
    {
        m_current_cell.col = m_current_pos->index;
        m_current_cell.row = m_current_pos->position;

        switch (m_current_pos->type)
        {
            case element_type_boolean:
                m_current_cell.type = cell_t::boolean;
                m_current_cell.value = m_current_pos->get<boolean_element_block>();
                break;
            case element_type_numeric:
                m_current_cell.type = cell_t::numeric;
                m_current_cell.value = m_current_pos->get<numeric_element_block>();
                break;
            case element_type_string:
            {
                m_current_cell.type = cell_t::string;
                string_id_t sid{m_current_pos->get<string_element_block>()};
                const std::string* s = m_cxt.get_string(sid);
                m_current_cell.value = s ? std::string_view{*s} : std::string_view{};
                break;
            }
            case element_type_inline_string:
                m_current_cell.type = cell_t::string;
                m_current_cell.value = std::string_view{m_current_pos->get<inline_string_element_block>()};
                break;
            case element_type_formula:
                m_current_cell.type = cell_t::formula;
                m_current_cell.value = m_current_pos->get<formula_element_block>();
                break;
            case element_type_empty:
                m_current_cell.type = cell_t::empty;
                m_current_cell.value = false;
            default:
                ;
        }

        m_update_current_cell = false;
    }
public:
    iterator_core_horizontal(const detail::model_context_impl& cxt, sheet_t sheet, const abs_rc_range_t& range) :
        m_cxt(cxt),
        m_update_current_cell(true)
    {
        const column_stores_t* cols = cxt.get_columns(sheet);
        if (cols && !cols->empty())
        {
            collection_type c = mdds::mtv::collection<column_store_t>(cols->begin(), cols->end());

            if (range.valid())
            {
                if (!range.all_columns())
                {
                    col_t c1 = range.first.column == column_unset ? 0 : range.first.column;
                    col_t c2 = range.last.column == column_unset ? (cols->size() - 1) : range.last.column;
                    assert(c1 >= 0);
                    assert(c1 <= c2);
                    std::size_t start = c1;
                    std::size_t size = c2 - c1 + 1;
                    c.set_collection_range(start, size);
                }

                if (!range.all_rows())
                {
                    const column_store_t& col = (*cols)[0];
                    row_t r1 = range.first.row == row_unset ? 0 : range.first.row;
                    row_t r2 = range.last.row == row_unset ? (col.size() - 1) : range.last.row;
                    assert(r1 >= 0);
                    assert(r1 <= r2);
                    std::size_t start = r1;
                    std::size_t size = r2 - r1 + 1;
                    c.set_element_range(start, size);
                }
            }

            m_collection.swap(c);
        }

        m_current_pos = m_collection.begin();
        m_end = m_collection.end();
    }

    virtual bool has() const override
    {
        return m_current_pos != m_end;
    }

    virtual void next() override
    {
        ++m_current_pos;
        m_update_current_cell = true;
    }

    virtual const model_cell_range::cell& get() const override
    {
        if (m_update_current_cell)
            update_current();
        return m_current_cell;
    }
};

class iterator_core_vertical : public model_cell_range::core
{
    const detail::model_context_impl& m_cxt;
    const column_stores_t* m_cols;
    mutable model_cell_range::cell m_current_cell;
    mutable bool m_update_current_cell;

    column_stores_t::const_iterator m_it_cols;
    column_stores_t::const_iterator m_it_cols_begin;
    column_stores_t::const_iterator m_it_cols_end;

    column_store_t::const_position_type m_current_pos;
    column_store_t::const_position_type m_end_pos;

    row_t m_row_first;
    row_t m_row_last;

    void update_current() const
    {
        column_store_t::const_iterator blk_pos = m_current_pos.first;

        switch (blk_pos->type)
        {
            case element_type_empty:
                m_current_cell.type = cell_t::empty;
                m_current_cell.value = false;
                break;
            case element_type_boolean:
                m_current_cell.type = cell_t::boolean;
                m_current_cell.value = column_store_t::get<boolean_element_block>(m_current_pos);
                break;
            case element_type_numeric:
                m_current_cell.type = cell_t::numeric;
                m_current_cell.value = column_store_t::get<numeric_element_block>(m_current_pos);
                break;
            case element_type_string:
            {
                m_current_cell.type = cell_t::string;
                string_id_t sid{column_store_t::get<string_element_block>(m_current_pos)};
                const std::string* s = m_cxt.get_string(sid);
                m_current_cell.value = s ? std::string_view{*s} : std::string_view{};
                break;
            }
            case element_type_inline_string:
                m_current_cell.type = cell_t::string;
                m_current_cell.value = std::string_view{column_store_t::get<inline_string_element_block>(m_current_pos)};
                break;
            case element_type_formula:
                m_current_cell.type = cell_t::formula;
                m_current_cell.value = column_store_t::get<formula_element_block>(m_current_pos);
                break;
            default:
                throw std::logic_error("unhandled element type.");
        }

        m_current_cell.row = column_store_t::logical_position(m_current_pos);
        m_current_cell.col = std::distance(m_it_cols_begin, m_it_cols);
        m_update_current_cell = false;
    }

public:
    iterator_core_vertical(const detail::model_context_impl& cxt, sheet_t sheet, const abs_rc_range_t& range) :
        m_cxt(cxt),
        m_update_current_cell(true),
        m_row_first(0),
        m_row_last(row_unset)
    {
        m_cols = cxt.get_columns(sheet);
        if (!m_cols)
            return;

        m_it_cols_begin = m_cols->begin();
        m_it_cols = m_it_cols_begin;
        m_it_cols_end = m_cols->end();
        if (m_it_cols_begin == m_it_cols_end)
            return;

        m_row_last = (*m_cols)[0].size() - 1;

        if (range.valid())
        {
            col_t last_col = m_cols->size() - 1;

            if (range.last.column != column_unset && range.last.column < last_col)
            {
                // Shrink the tail end.
                col_t diff = range.last.column - last_col;
                assert(diff < 0);
                std::advance(m_it_cols_end, diff);

                last_col += diff;
            }

            if (range.first.column != column_unset)
            {
                if (range.first.column <= last_col)
                    std::advance(m_it_cols, range.first.column);
                else
                {
                    // First column is past the last column.  Nothing to parse.
                    m_it_cols_begin = m_it_cols_end;
                    return;
                }
            }

            if (range.last.row != row_unset && range.last.row < m_row_last)
            {
                // Shrink the tail end.
                m_row_last = range.last.row;
            }

            if (range.first.row != row_unset)
            {
                if (range.first.row <= m_row_last)
                    m_row_first = range.first.row;
                else
                {
                    // First row is past the last row.  Set it to an empty
                    // range and bail out.
                    m_it_cols_begin = m_it_cols_end;
                    return;
                }
            }
        }

        const column_store_t& col = *m_it_cols;
        m_current_pos = col.position(m_row_first);
        m_end_pos = col.position(m_row_last+1);
    }

    bool has() const override
    {
        if (!m_cols)
            return false;

        return m_it_cols != m_it_cols_end;
    }

    void next() override
    {
        m_update_current_cell = true;
        m_current_pos = column_store_t::next_position(m_current_pos);

        const column_store_t* col = &*m_it_cols;
        if (m_current_pos != m_end_pos)
            // It hasn't reached the end of the current column yet.
            return;

        ++m_it_cols; // Move to the next column.
        if (m_it_cols == m_it_cols_end)
            return;

        // Reset the position to the first cell in the new column.
        col = &*m_it_cols;
        m_current_pos = col->position(m_row_first);
        m_end_pos = col->position(m_row_last+1);
    }

    const model_cell_range::cell& get() const override
    {
        if (m_update_current_cell)
            update_current();
        return m_current_cell;
    }
};

} // anonymous namespace

class model_cell_range::impl
{
public:
    const detail::model_context_impl* m_cxt;
    sheet_t m_sheet;
    abs_rc_range_t m_range;
    rc_direction_t m_dir;

    impl() :
        m_cxt(nullptr), m_sheet(0), m_range(), m_dir(rc_direction_t::horizontal) {}

    impl(const detail::model_context_impl& cxt, sheet_t sheet,
         const abs_rc_range_t& range, rc_direction_t dir) :
        m_cxt(&cxt), m_sheet(sheet), m_range(range), m_dir(dir) {}

    std::unique_ptr<core> make_core() const
    {
        if (!m_cxt)
            return nullptr;

        switch (m_dir)
        {
            case rc_direction_t::horizontal:
                return std::make_unique<iterator_core_horizontal>(*m_cxt, m_sheet, m_range);
            case rc_direction_t::vertical:
                return std::make_unique<iterator_core_vertical>(*m_cxt, m_sheet, m_range);
        }

        return nullptr;
    }
};

model_cell_range::const_iterator::const_iterator() = default;

model_cell_range::const_iterator::const_iterator(std::unique_ptr<core> c) :
    mp_core(std::move(c)) {}

model_cell_range::const_iterator::const_iterator(const_iterator&& other) = default;

model_cell_range::const_iterator&
model_cell_range::const_iterator::operator= (const_iterator&& other) = default;

model_cell_range::const_iterator::~const_iterator() = default;

model_cell_range::const_iterator& model_cell_range::const_iterator::operator++()
{
    assert(mp_core);
    mp_core->next();
    return *this;
}

void model_cell_range::const_iterator::operator++(int)
{
    ++*this;
}

model_cell_range::const_iterator::reference
model_cell_range::const_iterator::operator*() const
{
    assert(mp_core);
    return mp_core->get();
}

model_cell_range::const_iterator::pointer
model_cell_range::const_iterator::operator->() const
{
    assert(mp_core);
    return &mp_core->get();
}

bool model_cell_range::const_iterator::operator== (const const_iterator& r) const
{
    const bool l_end = !mp_core || !mp_core->has();
    const bool r_end = !r.mp_core || !r.mp_core->has();
    if (l_end && r_end)
        return true;
    if (l_end != r_end)
        return false;
    return mp_core.get() == r.mp_core.get();
}

bool model_cell_range::const_iterator::operator!= (const const_iterator& r) const
{
    return !(*this == r);
}

model_cell_range::model_cell_range() : mp_impl(std::make_unique<impl>()) {}

model_cell_range::model_cell_range(const detail::model_context_impl& cxt, sheet_t sheet,
                                   const abs_rc_range_t& range, rc_direction_t dir) :
    mp_impl(std::make_unique<impl>(cxt, sheet, range, dir)) {}

model_cell_range::model_cell_range(model_cell_range&& other) = default;

model_cell_range& model_cell_range::operator= (model_cell_range&& other) = default;

model_cell_range::~model_cell_range() = default;

model_cell_range::const_iterator model_cell_range::begin() const
{
    return const_iterator{mp_impl->make_core()};
}

model_cell_range::const_iterator model_cell_range::end() const
{
    return const_iterator{};
}

model_cell_range::const_iterator model_cell_range::cbegin() const
{
    return begin();
}

model_cell_range::const_iterator model_cell_range::cend() const
{
    return end();
}

std::ostream& operator<< (std::ostream& os, const model_cell_range::cell& c)
{
    os << "(row=" << c.row << "; col=" << c.col << "; type=" << short(c.type);

    switch (c.type)
    {
        case cell_t::boolean:
            os << "; boolean=" << std::get<bool>(c.value);
            break;
        case cell_t::formula:
            os << "; formula=" << std::get<const formula_cell*>(c.value);
            break;
        case cell_t::numeric:
            os << "; numeric=" << std::get<double>(c.value);
            break;
        case cell_t::string:
            os << "; string=\"" << std::get<std::string_view>(c.value) << '"';
            break;
        case cell_t::empty:
            os << "; empty";
            break;
        case cell_t::unknown:
        default:
            ;
    }

    os << ')';
    return os;
}

} // namespace ixion

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
