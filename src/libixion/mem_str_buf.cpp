/*************************************************************************
 *
 * Copyright (c) 2010, 2011 Kohei Yoshida
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 ************************************************************************/

#include "ixion/mem_str_buf.hpp"

#include <cstring>
#include <cassert>

using namespace std;

namespace ixion {

mem_str_buf::mem_str_buf() : mp_buf(NULL), m_size(0) {}

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

}
