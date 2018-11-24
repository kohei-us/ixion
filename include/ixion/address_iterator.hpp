/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_ADDRESS_ITERATOR_HPP
#define INCLUDED_IXION_ADDRESS_ITERATOR_HPP

#include "ixion/env.hpp"
#include "ixion/types.hpp"

#include <memory>

namespace ixion {

struct abs_range_t;
struct abs_address_t;

class IXION_DLLPUBLIC abs_address_iterator
{
    struct impl;
    std::unique_ptr<impl> mp_impl;

public:
    class IXION_DLLPUBLIC const_iterator
    {
        friend class abs_address_iterator;

        struct impl_node;
        std::unique_ptr<impl_node> mp_impl;

        const_iterator(const abs_range_t& range, rc_direction_t dir, bool end);
    public:
        using value_type = abs_address_t;

        const_iterator();
        const_iterator(const const_iterator& r);
        const_iterator(const_iterator&& r);
        ~const_iterator();

        const_iterator& operator++();
        const_iterator operator++(int);
        const_iterator& operator--();
        const_iterator operator--(int);

        const value_type& operator*() const;
        const value_type* operator->() const;

        bool operator== (const const_iterator& r) const;
        bool operator!= (const const_iterator& r) const;
    };

    abs_address_iterator(const abs_range_t& range, rc_direction_t dir);
    ~abs_address_iterator();

    const_iterator begin() const;
    const_iterator end() const;
    const_iterator cbegin() const;
    const_iterator cend() const;
};

}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
