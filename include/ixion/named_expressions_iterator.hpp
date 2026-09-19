/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once
#include "types.hpp"
#include "formula_tokens_fwd.hpp"

#include <memory>
#include <iosfwd>
#include <string>

namespace ixion {

class model_context;
struct abs_address_t;

/**
 * Iterator over the named expressions of one scope, either the global scope
 * or one sheet.  Obtained from
 * model_context::get_named_expressions_iterator().  It is a cursor: check
 * has(), read get(), then call next(), until has() returns false.  The
 * expressions are visited in the order of their names.
 */
class IXION_DLLPUBLIC named_expressions_iterator
{
    friend class model_context;

    struct impl;
    std::unique_ptr<impl> mp_impl;

    named_expressions_iterator(const model_context& cxt, sheet_t scope);

public:
    /** Construct an iterator over no named expressions. */
    named_expressions_iterator();
    /** Copy constructor. */
    named_expressions_iterator(const named_expressions_iterator& other);
    /** Move constructor. */
    named_expressions_iterator(named_expressions_iterator&& other);
    ~named_expressions_iterator();

    /**
     * One named expression, as returned by get().  Both pointers refer to
     * storage inside the model and remain valid as long as the named
     * expression stays in the model.
     */
    struct named_expression
    {
        /** Name of the expression. */
        const std::string* name;
        /** The expression itself. */
        const named_expression_t* expression;
    };

    /** Get the number of named expressions in the scope. */
    size_t size() const;
    /** Check whether the iterator is at a named expression rather than past the end. */
    bool has() const;
    /** Advance to the next named expression. */
    void next();

    /** Get the named expression at the current position. */
    named_expression get() const;

    /** Copy assignment. */
    named_expressions_iterator& operator= (const named_expressions_iterator& other);
};

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
