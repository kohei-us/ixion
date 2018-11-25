/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/model_iterator.hpp"
#include "ixion/global.hpp"
#include "ixion/exceptions.hpp"
#include "ixion/model_context.hpp"

#include <mdds/multi_type_vector/collection.hpp>
#include <sstream>

namespace ixion {

model_iterator::cell::cell() : row(0), col(0), type(celltype_t::empty) {}

bool model_iterator::cell::operator== (const cell& other) const
{
    if (type != other.type || row != other.row || col != other.col)
        return false;

    switch (type)
    {
        case celltype_t::empty:
            return true;
        case celltype_t::numeric:
            return value.numeric == other.value.numeric;
        case celltype_t::boolean:
            return value.boolean == other.value.boolean;
        case celltype_t::string:
            return value.string == other.value.string;
        case celltype_t::formula:
            // Compare by the formula cell memory address.
            return value.formula == other.value.formula;
        default:
            ;
    }

    return false;
}

class model_iterator::impl
{
public:
    virtual bool has() const = 0;
    virtual void next() = 0;
    virtual const model_iterator::cell& get() const = 0;
};

namespace {

class iterator_core_empty : public model_iterator::impl
{
    model_iterator::cell m_cell;
public:
    bool has() const override { return false; }

    void next() override {}

    const model_iterator::cell& get() const override
    {
        return m_cell;
    }
};


class iterator_core_horizontal : public model_iterator::impl
{
    using collection_type = mdds::mtv::collection<column_store_t>;

    collection_type m_collection;
    mutable model_iterator::cell m_current_cell;
    collection_type::const_iterator m_current_pos;
    collection_type::const_iterator m_end;

    void update_current() const
    {
        m_current_cell.col = m_current_pos->index;
        m_current_cell.row = m_current_pos->position;

        switch (m_current_pos->type)
        {
            case element_type_boolean:
                m_current_cell.type = celltype_t::boolean;
                m_current_cell.value.boolean = m_current_pos->get<boolean_element_block>();
                break;
            case element_type_numeric:
                m_current_cell.type = celltype_t::numeric;
                m_current_cell.value.numeric = m_current_pos->get<numeric_element_block>();
                break;
            case element_type_string:
                m_current_cell.type = celltype_t::string;
                m_current_cell.value.string = m_current_pos->get<string_element_block>();
                break;
            case element_type_formula:
                m_current_cell.type = celltype_t::formula;
                m_current_cell.value.formula = m_current_pos->get<formula_element_block>();
                break;
            case element_type_empty:
                m_current_cell.type = celltype_t::empty;
            default:
                ;
        }
    }
public:
    iterator_core_horizontal(const model_context& cxt, sheet_t sheet)
    {
        const column_stores_t* cols = cxt.get_columns(sheet);
        if (cols)
        {
            collection_type c = mdds::mtv::collection<column_store_t>(cols->begin(), cols->end());
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
    }

    virtual const model_iterator::cell& get() const override
    {
        update_current();
        return m_current_cell;
    }
};

class iterator_core_vertical : public model_iterator::impl
{
    model_iterator::cell m_current_cell;

public:
    iterator_core_vertical(const model_context& cxt, sheet_t sheet)
    {
    }

    bool has() const override
    {
        return false;
    }

    void next() override
    {
    }

    const model_iterator::cell& get() const override
    {
        return m_current_cell;
    }
};

} // anonymous namespace

model_iterator::model_iterator() : mp_impl(ixion::make_unique<iterator_core_empty>()) {}
model_iterator::model_iterator(const model_context& cxt, sheet_t sheet, rc_direction_t dir)
{
    switch (dir)
    {
        case rc_direction_t::horizontal:
            mp_impl = ixion::make_unique<iterator_core_horizontal>(cxt, sheet);
            break;
        case rc_direction_t::vertical:
            mp_impl = ixion::make_unique<iterator_core_vertical>(cxt, sheet);
            break;
    }
}

model_iterator::model_iterator(model_iterator&& other) : mp_impl(std::move(other.mp_impl)) {}

model_iterator::~model_iterator() {}

model_iterator& model_iterator::operator= (model_iterator&& other)
{
    mp_impl = std::move(other.mp_impl);
    other.mp_impl = ixion::make_unique<iterator_core_empty>();
    return *this;
}

bool model_iterator::has() const
{
    return mp_impl->has();
}

void model_iterator::next()
{
    mp_impl->next();
}

const model_iterator::cell& model_iterator::get() const
{
    return mp_impl->get();
}

} // namespace ixion

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
