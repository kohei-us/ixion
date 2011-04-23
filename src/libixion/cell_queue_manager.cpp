/*************************************************************************
 *
 * Copyright (c) 2010 Kohei Yoshida
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

#include "ixion/cell_queue_manager.hpp"

#include "ixion/cell.hpp"
#include "ixion/model_context.hpp"

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/thread.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/mutex.hpp>

#include <iostream>
#include <queue>
#include <string>

#define DEBUG_QUEUE_MANAGER 0

using ::std::string;
using ::std::cout;
using ::std::endl;
using ::std::ostringstream;
using ::std::queue;
using ::boost::mutex;
using ::boost::thread;
using ::boost::condition_variable;
using ::boost::ptr_vector;

namespace {

mutex tprintf_mtx;

void tprintf(const string& s)
{
#if DEBUG_QUEUE_MANAGER
    mutex::scoped_lock(tprintf_mtx);
    cout << s << endl;
    cout.flush();
#endif
}

class StackPrinter
{
public:
    explicit StackPrinter(const char* msg) :
        msMsg(msg)
    {
#if DEBUG_QUEUE_MANAGER
        string s = msg + string(": --begin");
        tprintf(s);
        mfStartTime = getTime();
#endif
    }

    ~StackPrinter()
    {
#if DEBUG_QUEUE_MANAGER
        double fEndTime = global::get_current_time();
        ostringstream os;
        os << msMsg << ": --end (durtion: " << (fEndTime-mfStartTime) << " sec)";
        tprintf(os.str());
#endif
    }

private:
    ::std::string msMsg;
    double mfStartTime;
};

}

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
        formula_cell* cell;
        bool terminate_requested;

        action_data() : cell(NULL), terminate_requested(false) {}
    };

    thread thr_main;

    init_status_data init_status;
    action_data action;

    worker_thread_data() {}
};

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
void worker_main(worker_thread_data* data, const model_context* context)
{
    StackPrinter __stack_printer__("manage_queue::worker_main");
    mutex::scoped_lock lock_cell(data->action.mtx);
    {
        mutex::scoped_lock lock_ready(data->init_status.mtx);
        data->init_status.ready = true;
        data->init_status.cond.notify_all();
    }

    tprintf("worker ready");

    while (!data->action.terminate_requested)
    {
        tprintf("worker waits...");
        {
            // Register itself as an idle thread.
            mutex::scoped_lock lock_wts(wts.mtx);
            wts.idle_wts.push(data);
            wts.cond.notify_all();
        }
        data->action.cond.wait(lock_cell);

        if (!data->action.cell)
            continue;

        data->action.cell->interpret(*context);
        data->action.cell = NULL;
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
    ptr_vector<worker_thread_data> workers;
    condition_variable cond_thread_ready;
    bool thread_ready;

    // queue status

    mutex mtx_queue;
    condition_variable cond_queue;
    queue<formula_cell*> cells;
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

void init_workers(size_t worker_count, const model_context* context)
{
    // Create specified number of worker threads.
    for (size_t i = 0; i < worker_count; ++i)
    {
        data.workers.push_back(new worker_thread_data);
        worker_thread_data& wt = data.workers.back();
        wt.thr_main = thread(::boost::bind(worker_main, &wt, context));
    }

    // Wait until the worker threads become ready.
    ptr_vector<worker_thread_data>::iterator itr = data.workers.begin(), itr_end = data.workers.end();
    for (; itr != itr_end; ++itr)
    {
        worker_thread_data& wt = *itr;
        mutex::scoped_lock lock(wt.init_status.mtx);
        while (!wt.init_status.ready)
            wt.init_status.cond.wait(lock);
    }
}

void terminate_workers()
{
#if DEBUG_QUEUE_MANAGER
    ostringstream os;
    os << "terminating all workers..." << endl;
    cout << os.str();
#endif
    ptr_vector<worker_thread_data>::iterator itr = data.workers.begin(), itr_end = data.workers.end();
    for (; itr != itr_end; ++itr)
    {
        worker_thread_data& wt = *itr;
        mutex::scoped_lock lock(wt.action.mtx);
        wt.action.terminate_requested = true;
        wt.action.cond.notify_all();
    }

    itr = data.workers.begin();
    for (; itr != itr_end; ++itr)
        itr->thr_main.join();
}

void interpret_cell(worker_thread_data& wt)
{
    mutex::scoped_lock lock(wt.action.mtx);

    // When we obtain the lock, the cell pointer is expected to be NULL.
    assert(!wt.action.cell);

    wt.action.cell = data.cells.front();
    data.cells.pop();
    wt.action.cond.notify_all();
}

/**
 * Main queue manager thread routine.
 */
void manage_queue_main(size_t worker_count, const model_context* context)
{
    StackPrinter __stack_printer__("::manage_queue_main");
    mutex::scoped_lock lock(data.mtx_queue);
    {
        mutex::scoped_lock lock(data.mtx_thread_ready);
        init_workers(worker_count, context);
        data.thread_ready = true;
        data.cond_thread_ready.notify_all();
    }

    while (data.action != qm_terminate_requested)
    {
        tprintf("waiting...");
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

    tprintf("terminating manage queue thread...");
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

void add_cell_to_queue(formula_cell* p)
{
#if DEBUG_QUEUE_MANAGER
//  ostringstream os;
//  os << "adding cell " << global::get_cell_name(p) << " to queue..." << endl;
//  cout << os.str();
#endif

    ::boost::mutex::scoped_lock lock(data.mtx_queue);
    data.cells.push(p);
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

void cell_queue_manager::init(size_t thread_count, const model_context& context)
{
    // Don't forget to reset the global data.
    data.reset();
    wts.reset();

    thread thr(::boost::bind(manage_queue_main, thread_count, &context));
    thr_queue.swap(thr);
    wait_init();
}

void cell_queue_manager::add_cell(formula_cell* cell)
{
    add_cell_to_queue(cell);
}

void cell_queue_manager::terminate()
{
    terminate_queue_thread();
    thr_queue.join();
}

}
