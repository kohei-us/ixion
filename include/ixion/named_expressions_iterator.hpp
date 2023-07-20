/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_NAMED_EXPRESSIONS_ITERATOR_HPP
#define INCLUDED_IXION_NAMED_EXPRESSIONS_ITERATOR_HPP

#include "types.hpp"
#include "formula_tokens_fwd.hpp"

#include <memory>
#include <iosfwd>
#include <string>

namespace ixion {

class model_context;
struct abs_address_t;

class IXION_DLLPUBLIC named_expressions_iterator
{
    friend class model_context;

    struct impl;
    std::unique_ptr<impl> mp_impl;

    named_expressions_iterator(const model_context& cxt, sheet_t scope);

public:
    named_expressions_iterator();
    named_expressions_iterator(const named_expressions_iterator& other);
    named_expressions_iterator(named_expressions_iterator&& other);
    ~named_expressions_iterator();

    struct named_expression
    {
        const std::string* name;
        const named_expression_t* expression;
    };

    size_t size() const;
    bool has() const;
    void next();

    named_expression get() const;

    named_expressions_iterator& operator= (const named_expressions_iterator& other);
};

}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
