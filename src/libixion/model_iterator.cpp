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

using collection_type = mdds::mtv::collection<column_store_t>;

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

struct model_iterator::impl
{
    collection_type collection;
    mutable model_iterator::cell current_cell;
    collection_type::const_iterator current_pos;
    collection_type::const_iterator end;

    impl() {}

    impl(const model_context& cxt, sheet_t sheet, rc_direction_t dir)
    {
        if (dir != rc_direction_t::horizontal)
        {
            std::ostringstream os;
            os << "Only horizontal iterator is implemented for now.";
            throw model_context_error(os.str(), model_context_error::not_implemented);
        }

        const column_stores_t* cols = cxt.get_columns(sheet);
        if (cols)
        {
            collection_type c = mdds::mtv::collection<column_store_t>(cols->begin(), cols->end());
            collection.swap(c);
        }

        current_pos = collection.begin();
        end = collection.end();
    }

    void update_current() const
    {
        current_cell.col = current_pos->index;
        current_cell.row = current_pos->position;

        switch (current_pos->type)
        {
            case element_type_boolean:
                current_cell.type = celltype_t::boolean;
                current_cell.value.boolean = current_pos->get<boolean_element_block>();
                break;
            case element_type_numeric:
                current_cell.type = celltype_t::numeric;
                current_cell.value.numeric = current_pos->get<numeric_element_block>();
                break;
            case element_type_string:
                current_cell.type = celltype_t::string;
                current_cell.value.string = current_pos->get<string_element_block>();
                break;
            case element_type_formula:
                current_cell.type = celltype_t::formula;
                current_cell.value.formula = current_pos->get<formula_element_block>();
                break;
            case element_type_empty:
                current_cell.type = celltype_t::empty;
            default:
                ;
        }
    }
};

model_iterator::model_iterator() : mp_impl(ixion::make_unique<impl>()) {}
model_iterator::model_iterator(const model_context& cxt, sheet_t sheet, rc_direction_t dir) :
    mp_impl(ixion::make_unique<impl>(cxt, sheet, dir)) {}

model_iterator::model_iterator(model_iterator&& other) : mp_impl(std::move(other.mp_impl)) {}

model_iterator::~model_iterator() {}

model_iterator& model_iterator::operator= (model_iterator&& other)
{
    mp_impl = std::move(other.mp_impl);
    other.mp_impl = ixion::make_unique<impl>();
    return *this;
}

bool model_iterator::has() const
{
    return mp_impl->current_pos != mp_impl->end;
}

void model_iterator::next()
{
    ++mp_impl->current_pos;
}

const model_iterator::cell& model_iterator::get() const
{
    mp_impl->update_current();
    return mp_impl->current_cell;
}

} // namespace ixion

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
