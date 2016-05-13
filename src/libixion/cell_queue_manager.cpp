/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/cell_queue_manager.hpp"
#include "ixion/cell.hpp"

#include "ixion/interface/formula_model_access.hpp"

#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>

#include <cassert>
#include <iostream>
#include <queue>
#include <string>
#include <vector>

using ::std::string;
using ::std::cout;
using ::std::endl;
using ::std::ostringstream;
using ::std::queue;
using ::boost::mutex;
using ::boost::thread;
using ::boost::condition_variable;

namespace ixion {

namespace {

/**
 * Data for each worker thread.
 */
struct worker_thread_data
{
    struct init_status_data
    {
        mutex mtx;
        condition_variable cond;
        bool ready;

        init_status_data() : ready(false) {}
    };

    struct action_data
    {
        mutex mtx;
        condition_variable cond;
        abs_address_t fcell;
        formula_cell* cell;
        bool cell_active;
        bool terminate_requested;

        action_data() : cell(NULL), cell_active(false), terminate_requested(false) {}
    };

    thread thr_main;

    init_status_data init_status;
    action_data action;

    worker_thread_data() {}
};

typedef std::vector<std::unique_ptr<worker_thread_data>> worker_threads_type;

/**
 * This structure keeps track of idle worker threads.
 */
struct worker_thread_status
{
    mutex mtx;
    condition_variable cond;
    queue<worker_thread_data*> idle_wts;
    worker_thread_status() {}

    void reset()
    {
        while (!idle_wts.empty())
            idle_wts.pop();
    }
};

worker_thread_status wts;

/**
 * Main worker thread routine.
 */
void worker_main(worker_thread_data* data, iface::formula_model_access* context)
{
    mutex::scoped_lock lock_cell(data->action.mtx);
    {
        mutex::scoped_lock lock_ready(data->init_status.mtx);
        data->init_status.ready = true;
        data->init_status.cond.notify_all();
    }

    while (!data->action.terminate_requested)
    {
        {
            // Register itself as an idle thread.
            mutex::scoped_lock lock_wts(wts.mtx);
            wts.idle_wts.push(data);
            wts.cond.notify_all();
        }
        data->action.cond.wait(lock_cell);

        if (!data->action.cell_active)
            continue;

        formula_cell* p = context->get_formula_cell(data->action.fcell);
        p->interpret(*context, data->action.fcell);
        data->action.cell_active = false;
    }
}

enum manage_queue_action_t
{
    qm_no_action,
    qm_cell_added_to_queue,
    qm_terminate_requested
};

struct manage_queue_data
{
    // thread ready

    mutex mtx_thread_ready;
    worker_threads_type workers;
    condition_variable cond_thread_ready;
    bool thread_ready;

    // queue status

    mutex mtx_queue;
    condition_variable cond_queue;
    queue<abs_address_t> cells;
    manage_queue_action_t action;

    manage_queue_data() :
        thread_ready(false),
        action(qm_no_action) {}

    void reset()
    {
        thread_ready = false;
        action = qm_no_action;
        while (!cells.empty())
            cells.pop();
    }
};

manage_queue_data data;

void init_workers(size_t worker_count, iface::formula_model_access* context)
{
    // Create specified number of worker threads.
    for (size_t i = 0; i < worker_count; ++i)
    {
        data.workers.push_back(make_unique<worker_thread_data>());
        worker_thread_data& wt = *data.workers.back();
        wt.thr_main = thread(::boost::bind(worker_main, &wt, context));
    }

    // Wait until the worker threads become ready.
    worker_threads_type::iterator itr = data.workers.begin(), itr_end = data.workers.end();
    for (; itr != itr_end; ++itr)
    {
        worker_thread_data& wt = **itr;
        mutex::scoped_lock lock(wt.init_status.mtx);
        while (!wt.init_status.ready)
            wt.init_status.cond.wait(lock);
    }
}

void terminate_workers()
{
    worker_threads_type::iterator itr = data.workers.begin(), itr_end = data.workers.end();
    for (; itr != itr_end; ++itr)
    {
        worker_thread_data& wt = **itr;
        mutex::scoped_lock lock(wt.action.mtx);
        wt.action.terminate_requested = true;
        wt.action.cond.notify_all();
    }

    itr = data.workers.begin();
    for (; itr != itr_end; ++itr)
        (*itr)->thr_main.join();
}

void interpret_cell(worker_thread_data& wt)
{
    mutex::scoped_lock lock(wt.action.mtx);

    // When we obtain the lock, the cell is expected to be inactive.
    assert(!wt.action.cell_active);

    wt.action.fcell = data.cells.front();
    data.cells.pop();
    wt.action.cond.notify_all();
}

/**
 * Main queue manager thread routine.
 */
void manage_queue_main(size_t worker_count, iface::formula_model_access* context)
{
    mutex::scoped_lock lock(data.mtx_queue);
    {
        mutex::scoped_lock lock(data.mtx_thread_ready);
        init_workers(worker_count, context);
        data.thread_ready = true;
        data.cond_thread_ready.notify_all();
    }

    while (data.action != qm_terminate_requested)
    {
        data.cond_queue.wait(lock);
        if (data.action == qm_cell_added_to_queue)
        {
            data.action = qm_no_action;

            mutex::scoped_lock wts_lock(wts.mtx);
            while (!wts.idle_wts.empty())
            {
                if (data.cells.empty())
                    // No more cells to interpret.  Bail out.
                    break;

                worker_thread_data& wt = *wts.idle_wts.front();
                wts.idle_wts.pop();
                interpret_cell(wt);
            }
        }
    }

    // Termination is being requested.  Finish interpreting the rest of the
    // cells, as no more new cells will be added.

    while (!data.cells.empty())
    {
        mutex::scoped_lock wts_lock(wts.mtx);
        if (wts.idle_wts.empty())
            // In case no threads are idle, wait until one becomes idle.
            wts.cond.wait(wts_lock);

        while (!wts.idle_wts.empty())
        {
            if (data.cells.empty())
                // No more cells to interpret.  Bail out.
                break;

            worker_thread_data& wt = *wts.idle_wts.front();
            wts.idle_wts.pop();
            interpret_cell(wt);
        }
    }

    terminate_workers();
}

void add_cell_to_queue(const abs_address_t& cell)
{
    ::boost::mutex::scoped_lock lock(data.mtx_queue);
    data.cells.push(cell);
    data.action = qm_cell_added_to_queue;
    data.cond_queue.notify_all();
}

void terminate_queue_thread()
{
    ::boost::mutex::scoped_lock lock(data.mtx_queue);
    data.action = qm_terminate_requested;
    data.cond_queue.notify_all();
}

/**
 * Wait until the manage queue thread becomes ready.
 */
void wait_init()
{
    mutex::scoped_lock lock(data.mtx_thread_ready);
    while (!data.thread_ready)
        data.cond_thread_ready.wait(lock);
}

thread thr_queue;

} // anonymous namespace

void cell_queue_manager::init(size_t thread_count, iface::formula_model_access& context)
{
    // Don't forget to reset the global data.
    data.reset();
    wts.reset();

    thread thr(::boost::bind(manage_queue_main, thread_count, &context));
    thr_queue.swap(thr);
    wait_init();
}

void cell_queue_manager::add_cell(const abs_address_t& cell)
{
    add_cell_to_queue(cell);
}

void cell_queue_manager::terminate()
{
    terminate_queue_thread();
    thr_queue.join();
}

}
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
