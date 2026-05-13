/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "model_cell_range.hpp"

namespace ixion {

namespace detail { class model_context_impl; }
struct abs_rc_range_t;

class IXION_DLLPUBLIC
[[deprecated("use ixion::model_cell_range")]]
model_iterator
{
    friend class detail::model_context_impl;

    model_cell_range m_range;
    model_cell_range::const_iterator m_pos;

    model_iterator(const detail::model_context_impl& cxt, sheet_t sheet, const abs_rc_range_t& range, rc_direction_t dir);
public:
    using cell = model_cell_range::cell;

    model_iterator();
    model_iterator(const model_iterator&) = delete;
    model_iterator(model_iterator&& other);
    ~model_iterator();

    model_iterator& operator= (const model_iterator&) = delete;
    model_iterator& operator= (model_iterator&& other);

    bool has() const;

    void next();

    const cell& get() const;
};

} // namespace ixion

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
