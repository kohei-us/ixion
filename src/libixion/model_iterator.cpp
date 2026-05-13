/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#elif defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4996)
#endif

#include "ixion/model_iterator.hpp"

namespace ixion {

model_iterator::model_iterator() = default;

model_iterator::model_iterator(const detail::model_context_impl& cxt, sheet_t sheet, const abs_rc_range_t& range, rc_direction_t dir) :
    m_range(cxt, sheet, range, dir), m_pos(m_range.begin()) {}

model_iterator::model_iterator(model_iterator&& other) = default;

model_iterator::~model_iterator() = default;

model_iterator& model_iterator::operator= (model_iterator&& other) = default;

bool model_iterator::has() const
{
    return m_pos != model_cell_range::const_iterator{};
}

void model_iterator::next()
{
    ++m_pos;
}

const model_iterator::cell& model_iterator::get() const
{
    return *m_pos;
}

} // namespace ixion

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
