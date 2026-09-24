/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "types.hpp"
#include "formula_tokens_fwd.hpp"

#include <cstddef>
#include <iterator>
#include <memory>
#include <string>

namespace ixion {

namespace detail { class model_context_impl; }

/**
 * STL-compliant range over the named expressions of one scope, either the
 * global scope or one sheet.  Returned by
 * @ref model_context::iterate_named_expressions.  The expressions are
 * visited in the order of their names.  Supports range-`for` and STL/range
 * algorithms via its nested @ref const_iterator.
 *
 * <i>The caller has to ensure that the named expressions of the scope do not
 * change for the duration of the iteration.</i>
 */
class IXION_DLLPUBLIC named_expressions_range
{
    friend class detail::model_context_impl;

    class impl;
    std::unique_ptr<impl> mp_impl;

    named_expressions_range(const detail::model_context_impl& cxt, sheet_t scope);

public:
    /**
     * One named expression yielded by the iteration.  Both references refer
     * to storage inside the model and remain valid as long as the named
     * expression stays in the model.
     */
    struct entry
    {
        /** Name of the expression. */
        const std::string& name;
        /** The expression itself. */
        const named_expression_t& expression;
    };

    /**
     * Forward iterator over the named expressions of a
     * named_expressions_range.  The entry it yields lives inside the
     * iterator, so the reference stays valid as long as the iterator does.
     */
    class IXION_DLLPUBLIC const_iterator
    {
        friend class named_expressions_range;

        class impl;
        std::unique_ptr<impl> mp_impl;

        explicit const_iterator(std::unique_ptr<impl> p);

    public:
        /** Iterator category, which is forward iterator. */
        using iterator_category = std::forward_iterator_tag;
        /** Type of the values the iterator yields. */
        using value_type        = entry;
        /** Type of the difference between two iterators. */
        using difference_type   = std::ptrdiff_t;
        /** Reference type of the yielded values. */
        using reference         = const entry&;
        /** Pointer type of the yielded values. */
        using pointer           = const entry*;

        /**
         * Construct an iterator at the end position of an empty range.  It
         * equals any other default-constructed iterator.
         */
        const_iterator();
        /** Copy constructor. */
        const_iterator(const const_iterator& other);
        /** Move constructor. */
        const_iterator(const_iterator&& other);
        ~const_iterator();

        /** Copy assignment. */
        const_iterator& operator= (const const_iterator& other);
        /** Move assignment. */
        const_iterator& operator= (const_iterator&& other);

        /** Advance to the next named expression. */
        const_iterator& operator++();
        /** Advance to the next named expression and return the previous position. */
        const_iterator operator++(int);

        /** Get the current named expression. */
        reference operator*() const;
        /** Get a pointer to the current named expression. */
        pointer operator->() const;

        /** Compare the positions of two iterators. */
        bool operator== (const const_iterator& other) const;
    };

    /** Construct a range over no named expressions. */
    named_expressions_range();
    /** Copy constructor. */
    named_expressions_range(const named_expressions_range& other);
    /** Move constructor. */
    named_expressions_range(named_expressions_range&& other);
    ~named_expressions_range();

    /** Copy assignment. */
    named_expressions_range& operator= (const named_expressions_range& other);
    /** Move assignment. */
    named_expressions_range& operator= (named_expressions_range&& other);

    /** Get an iterator to the first named expression of the range. */
    const_iterator begin() const;
    /** Get an iterator past the last named expression of the range. */
    const_iterator end() const;
    /** @copydoc begin() */
    const_iterator cbegin() const;
    /** @copydoc end() */
    const_iterator cend() const;

    /** Get the number of named expressions in the range. */
    std::size_t size() const;
    /** Check whether the range has no named expressions. */
    bool empty() const;
};

} // namespace ixion

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
