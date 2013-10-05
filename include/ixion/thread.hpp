/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

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
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
