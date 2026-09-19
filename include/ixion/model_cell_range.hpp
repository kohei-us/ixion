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

    /**
     * End sentinel to allow const_iterator to remain move-only while still
     * satisfying std::sentinel_for etc.
     */
    struct sentinel {};

    /**
     * One cell yielded by the iteration: its position within the sheet, its
     * type and its value.  Empty cells are yielded too, with the empty type
     * and no meaningful value.
     */
    struct IXION_DLLPUBLIC cell
    {
        /**
         * Type of the stored value.  A formula cell is represented by a
         * pointer to its formula_cell instance, which remains valid as long
         * as the cell stays in the model.
         */
        using value_type = std::variant<bool, double, std::string_view, const formula_cell*>;

        /** 0-based row position of the cell. */
        row_t row;
        /** 0-based column position of the cell. */
        col_t col;
        /** Type of the cell. */
        cell_t type;
        /** Value of the cell. */
        value_type value;

        /** Construct an empty cell at the top-left position. */
        cell();
        /** Construct an empty cell. */
        cell(row_t _row, col_t _col);
        /** Construct a boolean cell. */
        cell(row_t _row, col_t _col, bool _b);
        /** Construct a string cell from a null-terminated string. */
        cell(row_t _row, col_t _col, const char* _s);
        /** Construct a string cell. */
        cell(row_t _row, col_t _col, std::string_view _s);
        /** Construct a numeric cell. */
        cell(row_t _row, col_t _col, double _v);
        /** Construct a formula cell. */
        cell(row_t _row, col_t _col, const formula_cell* _f);

        /** Compare the position, type and value of two cells. */
        bool operator== (const cell& other) const;
    };

    /**
     * Move-only input iterator over the cells of a model_cell_range.  The
     * end of the range is marked by a sentinel rather than another
     * iterator.
     */
    class IXION_DLLPUBLIC const_iterator
    {
        friend class model_cell_range;

        std::unique_ptr<core> mp_core; //< null means end sentinel.

        explicit const_iterator(std::unique_ptr<core> c);

    public:
        /** Iterator category, which is input iterator. */
        using iterator_category = std::input_iterator_tag;
        /** Type of the values the iterator yields. */
        using value_type        = cell;
        /** Type of the difference between two iterators. */
        using difference_type   = std::ptrdiff_t;
        /** Reference type of the yielded values. */
        using reference         = const cell&;
        /** Pointer type of the yielded values. */
        using pointer           = const cell*;

        /** Construct an iterator at the end position. */
        const_iterator();
        /** Move constructor. */
        const_iterator(const_iterator&& other);
        /** Move assignment. */
        const_iterator& operator= (const_iterator&& other);
        ~const_iterator();

        /** Advance to the next cell. */
        const_iterator& operator++();
        /** Advance to the next cell.  Nothing is returned, as the iterator is move-only. */
        void operator++(int);

        /** Get the current cell. */
        reference operator*() const;
        /** Get a pointer to the current cell. */
        pointer   operator->() const;

        /** Compare the positions of two iterators. */
        bool operator== (const const_iterator& r) const;

        /** Check whether the iterator has reached the end of the range. */
        bool operator== (sentinel) const;
    };

    /** Construct an empty range. */
    model_cell_range();
    model_cell_range(const model_cell_range&) = delete;
    /** Move constructor. */
    model_cell_range(model_cell_range&& other);
    ~model_cell_range();

    model_cell_range& operator= (const model_cell_range&) = delete;
    /** Move assignment. */
    model_cell_range& operator= (model_cell_range&& other);

    /** Get an iterator to the first cell of the range. */
    const_iterator begin() const;
    /** Get the sentinel marking the end of the range. */
    sentinel end() const;
    /** @copydoc begin() */
    const_iterator cbegin() const;
    /** @copydoc end() */
    sentinel cend() const;
};

/** Print the position, type and value of a cell, for debugging. */
IXION_DLLPUBLIC std::ostream& operator<< (std::ostream& os, const model_cell_range::cell& c);

} // namespace ixion

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
