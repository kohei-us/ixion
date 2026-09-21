/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once
#include "types.hpp"

#include <memory>

namespace ixion {

struct abs_range_t;
struct abs_address_t;

/**
 * STL-compliant range that yields every @ref abs_address_t inside an
 * @ref abs_range_t one address at a time.
 *
 * The iteration order is determined by the @c ixion::rc_direction_t value passed
 * to the constructor:
 * - @c horizontal walks row-major within each sheet (column varies fastest).
 * - @c vertical walks column-major within each sheet (row varies fastest).
 *
 * In either direction, iteration visits all cells before advancing to the
 * next sheet.
 *
 * The range purely walks the address space specified by the @ref abs_range_t
 * bounds; it does not reference any sheet contents.  Use
 * @ref model_context::iterate_cells to iterate over cell values.
 */
class IXION_DLLPUBLIC abs_address_range
{
    struct impl;
    std::unique_ptr<impl> mp_impl;

public:
    /**
     * Bidirectional iterator over the addresses of an abs_address_range.
     */
    class IXION_DLLPUBLIC const_iterator
    {
        friend class abs_address_range;

        struct impl_node;
        std::unique_ptr<impl_node> mp_impl;

        const_iterator(const abs_range_t& range, rc_direction_t dir, bool end);
    public:
        /** Type of the values the iterator yields. */
        using value_type = abs_address_t;

        /** Construct an iterator that refers to no range. */
        const_iterator();
        /** Copy constructor. */
        const_iterator(const const_iterator& r);
        /** Move constructor. */
        const_iterator(const_iterator&& r);
        ~const_iterator();

        /** Advance to the next address. */
        const_iterator& operator++();
        /** Advance to the next address, returning the previous position. */
        const_iterator operator++(int);
        /** Move back to the previous address. */
        const_iterator& operator--();
        /** Move back to the previous address, returning the previous position. */
        const_iterator operator--(int);

        /** Get the current address. */
        const value_type& operator*() const;
        /** Get a pointer to the current address. */
        const value_type* operator->() const;

        /** Compare the positions of two iterators. */
        bool operator== (const const_iterator& r) const;
    };

    /**
     * Constructor.
     *
     * @param range Range whose addresses to iterate over.
     * @param dir Direction of the iteration.
     */
    abs_address_range(const abs_range_t& range, rc_direction_t dir);
    ~abs_address_range();

    /** Get an iterator to the first address of the range. */
    const_iterator begin() const;
    /** Get an iterator past the last address of the range. */
    const_iterator end() const;
    /** @copydoc begin() */
    const_iterator cbegin() const;
    /** @copydoc end() */
    const_iterator cend() const;
};

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
