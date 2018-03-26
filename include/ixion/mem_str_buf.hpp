/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __IXION_MEM_STR_BUF_HPP__
#define __IXION_MEM_STR_BUF_HPP__

#include "ixion/env.hpp"

#include <string>
#include <ostream>

namespace ixion {

/**
 * String buffer that only stores the first char position in memory and the
 * size of the string.
 */
class IXION_DLLPUBLIC mem_str_buf
{
public:
    struct IXION_DLLPUBLIC hash
    {
        size_t operator() (const mem_str_buf& s) const;
    };

    mem_str_buf();
    mem_str_buf(const char* p);
    mem_str_buf(const char* p, size_t n);
    mem_str_buf(const mem_str_buf& r);

    void append(const char* p);
    void set_start(const char* p);
    void inc();
    void dec();
    void pop_front();
    bool empty() const;
    size_t size() const;
    const char* get() const;
    void clear();
    void swap(mem_str_buf& r);
    bool equals(const char* s) const;
    ::std::string str() const;
    mem_str_buf& operator= (const mem_str_buf& r);
    char operator[] (size_t pos) const;
    bool operator== (const mem_str_buf& r) const;
    bool operator!= (const mem_str_buf& r) const { return !operator==(r); }
    char back() const;

private:
    const char* mp_buf;
    size_t m_size;
};

IXION_DLLPUBLIC bool operator<  (const mem_str_buf& left, const mem_str_buf& right);
IXION_DLLPUBLIC bool operator>  (const mem_str_buf& left, const mem_str_buf& right);

IXION_DLLPUBLIC std::ostream& operator<< (std::ostream& os, const mem_str_buf& str);

}

#endif
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
