/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __IXION_CELL_QUEUE_MANAGER_HPP__
#define __IXION_CELL_QUEUE_MANAGER_HPP__

#include "ixion/global.hpp"

#include <cstdlib>

namespace ixion {

struct abs_address_t;

namespace iface {

class model_context;

}

/**
 * This class manages parallel cell interpretation using threads.  This
 * class should never be instantiated.
 */
class cell_queue_manager
{
public:
    /**
     * Initialize queue manager thread, with specified number of worker
     * threads.
     *
     * @param thread_count desired number of worker threads.
     */
    static void init(size_t thread_count, iface::model_context& context);

    /**
     * Add new cell to queue to interpret.
     *
     * @param cell pointer to cell instance to interpret.
     */
    static void add_cell(const abs_address_t& cell);

    /**
     * Terminate the queue manager thread, along with all spawned worker
     * threads.
     */
    static void terminate();

private:
    cell_queue_manager();
    cell_queue_manager(const cell_queue_manager& r);
    ~cell_queue_manager();
};

}

#endif
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
