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

struct model_iterator::impl
{
    collection_type collection;
    model_iterator::cell current_cell;

    impl(const model_context& cxt, sheet_t sheet, model_iterator_direction_t dir)
    {
        if (dir != model_iterator_direction_t::horizontal)
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
    }
};

model_iterator::model_iterator(const model_context& cxt, sheet_t sheet, model_iterator_direction_t dir) :
    mp_impl(ixion::make_unique<impl>(cxt, sheet, dir))
{
}

model_iterator::model_iterator(model_iterator&& other) :
    mp_impl(std::move(other.mp_impl))
{
}

model_iterator::~model_iterator()
{
}

bool model_iterator::has() const
{
    return false;
}

void model_iterator::next()
{
}

const model_iterator::cell& model_iterator::get() const
{
    return mp_impl->current_cell;
}

}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
