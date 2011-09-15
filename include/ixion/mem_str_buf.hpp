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

#ifndef __IXION_MEM_STR_BUF_HPP__
#define __IXION_MEM_STR_BUF_HPP__

#include <string>

namespace ixion {

/**
 * String buffer that only stores the first char position in memory and the
 * size of the string.
 */
class mem_str_buf
{
public:
    mem_str_buf();
    mem_str_buf(const char* p, size_t n);

    void append(const char* p);
    void set_start(const char* p);
    void inc();
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

private:
    const char* mp_buf;
    size_t m_size;
};

bool operator<  (const mem_str_buf& left, const mem_str_buf& right);
bool operator>  (const mem_str_buf& left, const mem_str_buf& right);

}

#endif
