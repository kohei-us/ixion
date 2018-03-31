/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_CALC_STATUS_HPP
#define INCLUDED_IXION_CALC_STATUS_HPP

#include "ixion/formula_result.hpp"

#include <mutex>
#include <condition_variable>

#include <boost/intrusive_ptr.hpp>

namespace ixion {

struct calc_status
{
    calc_status(const calc_status&) = delete;
    calc_status& operator=(const calc_status&) = delete;

    std::mutex mtx;
    std::condition_variable cond;
    std::unique_ptr<formula_result> result;

    const rc_size_t group_size;

    size_t refcount;

    calc_status();
    calc_status(const rc_size_t& group_size);

    void add_ref();
    void release_ref();
};

inline void intrusive_ptr_add_ref(calc_status* p)
{
    p->add_ref();
}

inline void intrusive_ptr_release(calc_status* p)
{
    p->release_ref();
}

using calc_status_ptr_t = boost::intrusive_ptr<calc_status>;

}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
