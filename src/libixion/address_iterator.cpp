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
#include <functional>

namespace ixion {

namespace {

bool inc_sheet(const abs_range_t& range, abs_address_t& pos)
{
    if (pos.sheet < range.last.sheet)
    {
        ++pos.sheet;
        pos.row = range.first.row;
        pos.column = range.first.column;
        return true;
    }

    return false;
}

bool dec_sheet(const abs_range_t& range, abs_address_t& pos)
{
    if (range.first.sheet < pos.sheet)
    {
        --pos.sheet;
        pos.row = range.last.row;
        pos.column = range.last.column;
        return true;
    }

    return false;
}

void inc_vertical(const abs_range_t& range, abs_address_t& pos, bool& end_pos)
{
    if (end_pos)
        throw std::out_of_range("attempting to increment past the end position.");

    if (pos.row < range.last.row)
    {
        ++pos.row;
        return;
    }

    if (pos.column < range.last.column)
    {
        ++pos.column;
        pos.row = range.first.row;
        return;
    }

    if (inc_sheet(range, pos))
        return;

    assert(pos == range.last);
    end_pos = true;
}

void dec_vertical(const abs_range_t& range, abs_address_t& pos, bool& end_pos)
{
    if (end_pos)
    {
        end_pos = false;
        assert(pos == range.last);
        return;
    }

    if (range.first.row < pos.row)
    {
        --pos.row;
        return;
    }

    assert(pos.row == range.first.row);

    if (range.first.column < pos.column)
    {
        --pos.column;
        pos.row = range.last.row;
        return;
    }

    assert(pos.column == range.first.column);

    if (dec_sheet(range, pos))
        return;

    assert(pos == range.first);
    throw std::out_of_range("Attempting to decrement beyond the first position.");
}

void inc_horizontal(const abs_range_t& range, abs_address_t& pos, bool& end_pos)
{
    if (end_pos)
        throw std::out_of_range("attempting to increment past the end position.");

    if (pos.column < range.last.column)
    {
        ++pos.column;
        return;
    }

    if (pos.row < range.last.row)
    {
        ++pos.row;
        pos.column = range.first.column;
        return;
    }

    if (inc_sheet(range, pos))
        return;

    assert(pos == range.last);
    end_pos = true;
}

void dec_horizontal(const abs_range_t& range, abs_address_t& pos, bool& end_pos)
{
    if (end_pos)
    {
        end_pos = false;
        assert(pos == range.last);
        return;
    }

    if (range.first.column < pos.column)
    {
        --pos.column;
        return;
    }

    assert(pos.column == range.first.column);

    if (range.first.row < pos.row)
    {
        --pos.row;
        pos.column = range.last.column;
        return;
    }

    assert(pos.row == range.first.row);

    if (dec_sheet(range, pos))
        return;

    assert(pos == range.first);
    throw std::out_of_range("Attempting to decrement beyond the first position.");
}

using update_func_type = std::function<void(const abs_range_t&,abs_address_t&,bool&)>;

} // anonymous namespace

struct abs_address_iterator::impl
{
    const abs_range_t m_range;
    rc_direction_t m_dir;

    impl(const abs_range_t& range, rc_direction_t dir) :
        m_range(range), m_dir(dir) {}
};

struct abs_address_iterator::const_iterator::impl_node
{
    const abs_range_t* mp_range;
    abs_address_t m_pos;
    bool m_end_pos; //< flag that indicates whether the node is at the position past the last valid address.

    update_func_type m_func_inc;
    update_func_type m_func_dec;

    impl_node() : mp_range(nullptr), m_pos(abs_address_t::invalid), m_end_pos(false) {}

    impl_node(const abs_range_t& range, rc_direction_t dir, bool end) :
        mp_range(&range),
        m_pos(end ? range.last : range.first),
        m_end_pos(end)
    {
        switch (dir)
        {
            case rc_direction_t::horizontal:
                m_func_inc = inc_horizontal;
                m_func_dec = dec_horizontal;
                break;
            case rc_direction_t::vertical:
                m_func_inc = inc_vertical;
                m_func_dec = dec_vertical;
                break;
            default:
                throw std::logic_error("unhandled direction value.");
        }
    }

    impl_node(const impl_node& r) :
        mp_range(r.mp_range),
        m_pos(r.m_pos),
        m_end_pos(r.m_end_pos),
        m_func_inc(r.m_func_inc),
        m_func_dec(r.m_func_dec) {}

    bool equals(const impl_node& r) const
    {
        return mp_range == r.mp_range && m_pos == r.m_pos && m_end_pos == r.m_end_pos;
    }

    void inc()
    {
        m_func_inc(*mp_range, m_pos, m_end_pos);
    }

    void dec()
    {
        m_func_dec(*mp_range, m_pos, m_end_pos);
    }
};

abs_address_iterator::const_iterator::const_iterator() :
    mp_impl(ixion::make_unique<impl_node>()) {}

abs_address_iterator::const_iterator::const_iterator(
    const abs_range_t& range, rc_direction_t dir, bool end) :
    mp_impl(ixion::make_unique<impl_node>(range, dir, end)) {}

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

abs_address_iterator::abs_address_iterator(const abs_range_t& range, rc_direction_t dir) :
    mp_impl(ixion::make_unique<impl>(range, dir)) {}

abs_address_iterator::~abs_address_iterator() {}

abs_address_iterator::const_iterator abs_address_iterator::begin() const
{
    return cbegin();
}

abs_address_iterator::const_iterator abs_address_iterator::end() const
{
    return cend();
}

abs_address_iterator::const_iterator abs_address_iterator::cbegin() const
{
    return const_iterator(mp_impl->m_range, mp_impl->m_dir, false);
}

abs_address_iterator::const_iterator abs_address_iterator::cend() const
{
    return const_iterator(mp_impl->m_range, mp_impl->m_dir, true);
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
