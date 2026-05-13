/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once
#include "types.hpp"

#include <cstddef>
#include <iosfwd>
#include <iterator>
#include <memory>
#include <string_view>
#include <variant>

namespace ixion {

namespace detail { class model_context_impl; }
class formula_cell;
struct abs_rc_range_t;

/**
 * STL-compliant range over the cells of a sheet (or sub-range).  Returned by
 * @ref model_context::iterate_cells.  Supports range-`for` and STL/range
 * algorithms via its nested @ref const_iterator.
 *
 * <i>The caller has to ensure that the model content does not change for the
 * duration of the iteration.</i>
 */
class IXION_DLLPUBLIC model_cell_range
{
    friend class detail::model_context_impl;
    friend class model_iterator; //< deprecated shim that wraps this type.

    class impl;
    std::unique_ptr<impl> mp_impl;

    model_cell_range(const detail::model_context_impl& cxt, sheet_t sheet,
                     const abs_rc_range_t& range, rc_direction_t dir);

public:
    class core;

    struct IXION_DLLPUBLIC cell
    {
        using value_type = std::variant<bool, double, std::string_view, const formula_cell*>;

        row_t row;
        col_t col;
        cell_t type;
        value_type value;

        cell();
        cell(row_t _row, col_t _col);
        cell(row_t _row, col_t _col, bool _b);
        cell(row_t _row, col_t _col, std::string_view _s);
        cell(row_t _row, col_t _col, double _v);
        cell(row_t _row, col_t _col, const formula_cell* _f);

        bool operator== (const cell& other) const;
    };

    class IXION_DLLPUBLIC const_iterator
    {
        friend class model_cell_range;

        std::unique_ptr<core> mp_core; //< null means end sentinel.

        explicit const_iterator(std::unique_ptr<core> c);

    public:
        using iterator_category = std::input_iterator_tag;
        using value_type        = cell;
        using difference_type   = std::ptrdiff_t;
        using reference         = const cell&;
        using pointer           = const cell*;

        const_iterator();
        const_iterator(const_iterator&& other);
        const_iterator& operator= (const_iterator&& other);
        ~const_iterator();

        const_iterator& operator++();
        void operator++(int);

        reference operator*() const;
        pointer   operator->() const;

        bool operator== (const const_iterator& r) const;
        bool operator!= (const const_iterator& r) const;
    };

    model_cell_range();
    model_cell_range(const model_cell_range&) = delete;
    model_cell_range(model_cell_range&& other);
    ~model_cell_range();

    model_cell_range& operator= (const model_cell_range&) = delete;
    model_cell_range& operator= (model_cell_range&& other);

    const_iterator begin() const;
    const_iterator end() const;
    const_iterator cbegin() const;
    const_iterator cend() const;
};

IXION_DLLPUBLIC std::ostream& operator<< (std::ostream& os, const model_cell_range::cell& c);

} // namespace ixion

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
