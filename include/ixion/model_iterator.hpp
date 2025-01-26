/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_MODEL_ITERATOR_HPP
#define INCLUDED_IXION_MODEL_ITERATOR_HPP

#include "types.hpp"

#include <memory>
#include <iosfwd>
#include <variant>

namespace ixion {

namespace detail { class model_context_impl; }
class formula_cell;
struct abs_rc_range_t;

class IXION_DLLPUBLIC model_iterator
{
public:
    class impl;

private:
    friend class detail::model_context_impl;
    std::unique_ptr<model_iterator::impl> mp_impl;

    model_iterator(const detail::model_context_impl& cxt, sheet_t sheet, const abs_rc_range_t& range, rc_direction_t dir);
public:

    struct IXION_DLLPUBLIC cell
    {
        using value_type = std::variant<bool, double, string_id_t, const formula_cell*>;

        row_t row;
        col_t col;
        cell_t type;
        value_type value;

        cell();
        cell(row_t _row, col_t _col);
        cell(row_t _row, col_t _col, bool _b);
        cell(row_t _row, col_t _col, string_id_t _s);
        cell(row_t _row, col_t _col, double _v);
        cell(row_t _row, col_t _col, const formula_cell* _f);

        bool operator== (const cell& other) const;
        bool operator!= (const cell& other) const;
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

IXION_DLLPUBLIC std::ostream& operator<< (std::ostream& os, const model_iterator::cell& c);

} // namespace ixion

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
