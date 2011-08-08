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

#ifndef __IXION_THREAD_HPP__
#define __IXION_THREAD_HPP__

#include <mutex>
#include <type_traits>
#include <utility>

namespace ixion {

/** 
 * Original implementation from http://www.stdthread.co.uk/syncvalue .
 * Altered naming conventions to match this code base.  You can read more
 * about this template here: http://www.drdobbs.com/cpp/225200269 .
 */
template<typename T>
class synchronized_value
{
    T data;
    ::std::mutex m;
public:
    struct updater
    {
    private:
        friend class synchronized_value;
        
        ::std::unique_lock<std::mutex> lk;
        T& data;
        
        explicit updater(synchronized_value& outer) :
            lk(outer.m),data(outer.data) {}
    public:
        updater(updater&& other):
            lk(::std::move(other.lk)),data(other.data) {}

        T* operator->()
        {
            return &data;
        }
        
        T& operator*()
        {
            return data;
        }
    };

    updater operator->()
    {
        return updater(*this);
    }
    
    updater update()
    {
        return updater(*this);
    }

private:
    class deref_value
    {
    private:
        friend class synchronized_value;
        
        std::unique_lock<std::mutex> lk;
        T& data;
        
        explicit deref_value(synchronized_value& outer) :
            lk(outer.m),data(outer.data)
        {}

        deref_value(deref_value&& other):
            lk(std::move(other.lk)),data(other.data)
        {}
        
    public:
        operator T()
        {
            return data;
        }
        
        deref_value& operator=(T const& new_val)
        {
            data=new_val;
            return *this;
        }
    };

public:
    deref_value operator*()
    {
        return deref_value(*this);
    }
    
};

}

#endif
