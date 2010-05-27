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

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <unistd.h>

#include <queue>

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/thread.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/mutex.hpp>

#include <stdio.h>
#include <string>
#include <sys/time.h>

using namespace std;
using ::boost::mutex;
using ::boost::thread;
using ::boost::condition_variable;
using ::boost::ptr_vector;

namespace {

mutex tprintf_mtx;

void tprintf(const string& s)
{
    mutex::scoped_lock(tprintf_mtx);
    cout << s << endl;
    cout.flush();
}

class StackPrinter
{
public:
    explicit StackPrinter(const char* msg) :
        msMsg(msg)
    {
        string s = msg + string(": --begin");
        tprintf(s);
        mfStartTime = getTime();
    }

    ~StackPrinter()
    {
        double fEndTime = getTime();
        ostringstream os;
        os << msMsg << ": --end (durtion: " << (fEndTime-mfStartTime) << " sec)";
        tprintf(os.str());
    }

private:
    double getTime() const
    {
        timeval tv;
        gettimeofday(&tv, NULL);
        return tv.tv_sec + tv.tv_usec / 1000000.0;
    }

    ::std::string msMsg;
    double mfStartTime;
};

}

class formula_cell
{
public:
    formula_cell() : mp_thread(NULL) {}

    void interpret_thread()
    {
        mp_thread = new ::boost::thread(::boost::bind(&formula_cell::interpret, this));
    }

    void interpret()
    {
//      StackPrinter __stack_printer__("formula_cell::interpret");
        sleep(1);
    }

    void interpret_join()
    {
        mp_thread->join();
        delete mp_thread;
        mp_thread = NULL;
    }
private:
    ::boost::thread* mp_thread;
};

namespace manage_queue {

/**
 * Data relevant for each worker thread.
 */
struct worker_thread_data
{
    thread thr_main;

    mutex mtx_ready;
    condition_variable cond_ready;
    bool thread_ready;

    mutex mtx_cell;
    condition_variable cond_cell;
    formula_cell* cell;
    bool terminate_requested;

    worker_thread_data() :
        thread_ready(false),
        cell(NULL),
        terminate_requested(false) {}
};

struct worker_thread_status
{
    mutex mtx;
    condition_variable cond;
    queue<worker_thread_data*> idle_wts;
    worker_thread_status() {}
};

worker_thread_status wts;

/**
 * Main worker thread routine.
 */
void worker_main(worker_thread_data* data)
{
    StackPrinter __stack_printer__("manage_queue::worker_main");
    mutex::scoped_lock lock_cell(data->mtx_cell);
    {
        mutex::scoped_lock lock_ready(data->mtx_ready);
        data->thread_ready = true;
        data->cond_ready.notify_all();
    }

    tprintf("worker ready");

    while (!data->terminate_requested)
    {
        tprintf("worker waits...");
        {
            // Register itself as an idle thread.
            mutex::scoped_lock lock_wts(wts.mtx);
            wts.idle_wts.push(data);
            wts.cond.notify_all();
        }
        data->cond_cell.wait(lock_cell);

        if (!data->cell)
            continue;

        ostringstream os;
        os << "interpret cell " << data->cell;
        tprintf(os.str());
        data->cell->interpret();
        data->cell = NULL;
    }
}

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
    bool added_to_queue;
    bool terminate_requested;

    manage_queue_data() :
        thread_ready(false),
        added_to_queue(false), 
        terminate_requested(false) {}
};

manage_queue_data data;

void init_workers(size_t worker_count)
{
    // Create specified number of worker threads.
    for (size_t i = 0; i < worker_count; ++i)
    {
        data.workers.push_back(new worker_thread_data);
        worker_thread_data& wt = data.workers.back();
        wt.thr_main = thread(::boost::bind(worker_main, &wt));
    }

    // Wait until the worker threads become ready.
    ptr_vector<worker_thread_data>::iterator itr = data.workers.begin(), itr_end = data.workers.end();
    for (; itr != itr_end; ++itr)
    {
        worker_thread_data& wt = *itr;
        mutex::scoped_lock lock(wt.mtx_ready);
        while (!wt.thread_ready)
            wt.cond_ready.wait(lock);
    }
}

void terminate_workers()
{
    tprintf("terminate all workers.");
    ptr_vector<worker_thread_data>::iterator itr = data.workers.begin(), itr_end = data.workers.end();
    for (; itr != itr_end; ++itr)
    {
        worker_thread_data& wt = *itr;
        mutex::scoped_lock lock(wt.mtx_cell);
        wt.terminate_requested = true;
        wt.cond_cell.notify_all();
    }

    itr = data.workers.begin();
    for (; itr != itr_end; ++itr)
        itr->thr_main.join();
}

void interpret_cell(worker_thread_data& wt)
{
    mutex::scoped_lock lock(wt.mtx_cell);

    // When we obtain the lock, the cell pointer is expected to be NULL.
    assert(!wt.cell);

    wt.cell = data.cells.front();
    data.cells.pop();
    wt.cond_cell.notify_all();
}

/**
 * Main queue manager thread routine.
 */
void main(size_t worker_count)
{
    StackPrinter __stack_printer__("::manage_queue_main");
    mutex::scoped_lock lock(data.mtx_queue);
    {
        mutex::scoped_lock lock(data.mtx_thread_ready);
        init_workers(worker_count);
        data.thread_ready = true;
        data.cond_thread_ready.notify_all();
    }

    while (!data.terminate_requested)
    {
        tprintf("waiting...");
        data.cond_queue.wait(lock);
        if (data.added_to_queue)
        {
            data.added_to_queue = false;

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

void add_cell(formula_cell* p)
{
    tprintf("adding to queue...");
    ::boost::mutex::scoped_lock lock(data.mtx_queue);
    data.cells.push(p);
    data.added_to_queue = true;
    data.cond_queue.notify_all();
}

void terminate()
{
    ::boost::mutex::scoped_lock lock(data.mtx_queue);
    data.terminate_requested = true;
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

} // namespace manage_queue

int main()
{
    StackPrinter __stack_printer__("::main");

    size_t cell_count = 25;
    size_t worker_count = 8;
    cout << "number of cells to interpret: " << cell_count << endl;
    cout << "number of worker threads: " << worker_count << endl;

    ::boost::ptr_vector<formula_cell> cells;
    for (size_t i = 0; i < cell_count; ++i)
        cells.push_back(new formula_cell);

    thread thr_queue(::boost::bind(manage_queue::main, worker_count));
    manage_queue::wait_init();
    
    ::boost::ptr_vector<formula_cell>::iterator itr, itr_beg = cells.begin(), itr_end = cells.end();
    for (itr = itr_beg; itr != itr_end; ++itr)
    {
        formula_cell* p = &(*itr);
        manage_queue::add_cell(p);
    }

    manage_queue::terminate();
    thr_queue.join();
    assert(manage_queue::data.cells.empty());
}
