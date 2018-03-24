/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/address_iterator.hpp"
#include "ixion/address.hpp"
#include "ixion/global.hpp"

#include <cassert>

namespace ixion {

struct abs_address_iterator::impl
{
    const abs_range_t m_range;

    impl(const abs_range_t& range) : m_range(range) {}
};

struct abs_address_iterator::const_iterator::impl_node
{
    const abs_range_t* mp_range;
    abs_address_t m_pos;
    bool m_end_pos; //< flag that indicates whether the node is at the position past the last valid address.

    impl_node() : mp_range(nullptr), m_pos(abs_address_t::invalid), m_end_pos(false) {}

    impl_node(const abs_range_t& range, bool end) :
        mp_range(&range),
        m_pos(end ? range.last : range.first),
        m_end_pos(end) {}

    impl_node(const impl_node& r) :
        mp_range(r.mp_range),
        m_pos(r.m_pos),
        m_end_pos(r.m_end_pos) {}

    bool equals(const impl_node& r) const
    {
        return mp_range == r.mp_range && m_pos == r.m_pos && m_end_pos == r.m_end_pos;
    }

    void inc()
    {
        if (m_end_pos)
            throw std::out_of_range("attempting to increment past the end position.");

        if (m_pos.column < mp_range->last.column)
        {
            ++m_pos.column;
            return;
        }

        if (m_pos.row < mp_range->last.row)
        {
            ++m_pos.row;
            m_pos.column = mp_range->first.column;
            return;
        }

        if (m_pos.sheet < mp_range->last.sheet)
        {
            ++m_pos.sheet;
            m_pos.row = mp_range->first.row;
            m_pos.column = mp_range->first.column;
            return;
        }

        assert(m_pos == mp_range->last);
        m_end_pos = true;
    }

    void dec()
    {
        if (m_end_pos)
        {
            m_end_pos = false;
            assert(m_pos == mp_range->last);
            return;
        }

        if (mp_range->first.column < m_pos.column)
        {
            --m_pos.column;
            return;
        }

        assert(m_pos.column == mp_range->first.column);

        if (mp_range->first.row < m_pos.row)
        {
            --m_pos.row;
            m_pos.column = mp_range->last.column;
            return;
        }

        assert(m_pos.row == mp_range->first.row);

        if (mp_range->first.sheet < m_pos.sheet)
        {
            --m_pos.sheet;
            m_pos.row = mp_range->last.row;
            m_pos.column = mp_range->last.column;
            return;
        }

        assert(m_pos == mp_range->first);
        throw std::out_of_range("Attempting to decrement beyond the first position.");
    }
};

abs_address_iterator::const_iterator::const_iterator() :
    mp_impl(ixion::make_unique<impl_node>()) {}

abs_address_iterator::const_iterator::const_iterator(
    const abs_range_t& range, bool end) :
    mp_impl(ixion::make_unique<impl_node>(range, end)) {}

abs_address_iterator::const_iterator::const_iterator(const const_iterator& r) :
    mp_impl(ixion::make_unique<impl_node>(*r.mp_impl)) {}

abs_address_iterator::const_iterator::const_iterator(const_iterator&& r) :
    mp_impl(std::move(r.mp_impl)) {}

abs_address_iterator::const_iterator::~const_iterator() {}

abs_address_iterator::const_iterator& abs_address_iterator::const_iterator::operator++()
{
    mp_impl->inc();
    return *this;
}

abs_address_iterator::const_iterator abs_address_iterator::const_iterator::operator++(int)
{
    auto saved = *this;
    mp_impl->inc();
    return saved;
}

abs_address_iterator::const_iterator& abs_address_iterator::const_iterator::operator--()
{
    mp_impl->dec();
    return *this;
}

abs_address_iterator::const_iterator abs_address_iterator::const_iterator::operator--(int)
{
    auto saved = *this;
    mp_impl->dec();
    return saved;
}

const abs_address_iterator::const_iterator::value_type& abs_address_iterator::const_iterator::operator*() const
{
    return mp_impl->m_pos;
}

const abs_address_iterator::const_iterator::value_type* abs_address_iterator::const_iterator::operator->() const
{
    return &mp_impl->m_pos;
}

bool abs_address_iterator::const_iterator::operator== (const const_iterator& r) const
{
    return mp_impl->equals(*r.mp_impl);
}

bool abs_address_iterator::const_iterator::operator!= (const const_iterator& r) const
{
    return !operator==(r);
}

abs_address_iterator::abs_address_iterator(const abs_range_t& range) :
    mp_impl(ixion::make_unique<impl>(range)) {}

abs_address_iterator::~abs_address_iterator() {}

abs_address_iterator::const_iterator abs_address_iterator::begin() const
{
    return const_iterator(mp_impl->m_range, false);
}

abs_address_iterator::const_iterator abs_address_iterator::end() const
{
    return const_iterator(mp_impl->m_range, true);
}

abs_address_iterator::const_iterator abs_address_iterator::cbegin() const
{
    return const_iterator(mp_impl->m_range, false);
}

abs_address_iterator::const_iterator abs_address_iterator::cend() const
{
    return const_iterator(mp_impl->m_range, true);
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
