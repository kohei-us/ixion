/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "mem_str_buf.hpp"

#include <cstring>
#include <cassert>
#include <algorithm>

using namespace std;

namespace ixion {

size_t mem_str_buf::hash::operator() (const mem_str_buf& s) const
{
    // copied from the hash function for pstring from orcus.
    size_t hash_val = s.size();
    size_t loop_size = std::min<size_t>(hash_val, 20); // prevent too much looping.
    const char* p = s.get();
    for (size_t i = 0; i < loop_size; ++i, ++p)
    {
        hash_val += static_cast<size_t>(*p);
        hash_val *= 2;
    }

    return hash_val;
}

mem_str_buf::mem_str_buf() : mp_buf(NULL), m_size(0) {}
mem_str_buf::mem_str_buf(const char* p) : mp_buf(p), m_size(strlen(p)) {}
mem_str_buf::mem_str_buf(const char* p, size_t n) : mp_buf(p), m_size(n) {}
mem_str_buf::mem_str_buf(const mem_str_buf& r) : mp_buf(r.mp_buf), m_size(r.m_size) {}

void mem_str_buf::append(const char* p)
{
    if (m_size)
        inc();
    else
        set_start(p);
}

void mem_str_buf::set_start(const char* p)
{
    mp_buf = p;
    m_size = 1;
}

void mem_str_buf::inc()
{
    assert(mp_buf);
    ++m_size;
}

void mem_str_buf::dec()
{
    assert(mp_buf);
    --m_size;
}

void mem_str_buf::pop_front()
{
    ++mp_buf;
    --m_size;
}

bool mem_str_buf::empty() const
{
    return m_size == 0;
}

size_t mem_str_buf::size() const
{
    return m_size;
}

const char* mem_str_buf::get() const
{
    return mp_buf;
}

void mem_str_buf::clear()
{
    mp_buf = NULL;
    m_size = 0;
}

void mem_str_buf::swap(mem_str_buf& r)
{
    ::std::swap(mp_buf, r.mp_buf);
    ::std::swap(m_size, r.m_size);
}

bool mem_str_buf::equals(const char* s) const
{
    if (strlen(s) != m_size)
        return false;

    return ::std::strncmp(mp_buf, s, m_size) == 0;
}

string mem_str_buf::str() const
{
    return string(mp_buf, m_size);
}

mem_str_buf& mem_str_buf::operator= (const mem_str_buf& r)
{
    mp_buf = r.mp_buf;
    m_size = r.m_size;
    return *this;
}

char mem_str_buf::operator[] (size_t pos) const
{
    return mp_buf[pos];
}

bool mem_str_buf::operator== (const mem_str_buf& r) const
{
    if (m_size != r.m_size)
        return false;

    for (size_t i = 0; i < m_size; ++i)
        if (mp_buf[i] != r.mp_buf[i])
            return false;

    return true;
}

char mem_str_buf::back() const
{
    if (!m_size)
        return '\0';

    return mp_buf[m_size-1];
}

bool operator< (const mem_str_buf& left, const mem_str_buf& right)
{
    // TODO: optimize this.
    return left.str() < right.str();
}

bool operator> (const mem_str_buf& left, const mem_str_buf& right)
{
    // TODO: optimize this.
    return left.str() > right.str();
}

std::ostream& operator<< (std::ostream& os, const mem_str_buf& str)
{
    os << str.str();
    return os;
}

}
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
