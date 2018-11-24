/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_MODEL_ITERATOR_HPP
#define INCLUDED_IXION_MODEL_ITERATOR_HPP

#include "ixion/types.hpp"
#include <memory>

namespace ixion {

class model_context;
class formula_cell;

class IXION_DLLPUBLIC model_iterator
{
    friend class model_context;

    struct impl;
    std::unique_ptr<impl> mp_impl;

    model_iterator(const model_context& cxt, sheet_t sheet, rc_direction_t dir);
public:

    struct cell
    {
        row_t row;
        col_t col;
        celltype_t type;

        union
        {
            bool boolean;
            double numeric;
            string_id_t string;
            const formula_cell* formula;

        } value;
    };

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

}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
