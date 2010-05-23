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
#include <unistd.h>

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/thread.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/mutex.hpp>

#include <stdio.h>
#include <string>
#include <sys/time.h>

namespace {

class StackPrinter
{
public:
    explicit StackPrinter(const char* msg) :
        msMsg(msg)
    {
        fprintf(stdout, "%s: --begin\n", msMsg.c_str());
        mfStartTime = getTime();
    }

    ~StackPrinter()
    {
        double fEndTime = getTime();
        fprintf(stdout, "%s: --end (duration: %g sec)\n", msMsg.c_str(), (fEndTime-mfStartTime));
    }

    void printTime(int line) const
    {
        double fEndTime = getTime();
        fprintf(stdout, "%s: --(%d) (duration: %g sec)\n", msMsg.c_str(), line, (fEndTime-mfStartTime));
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

using namespace std;

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
        StackPrinter __stack_printer__("formula_cell::interpret");
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

class thread_queue_manager
{
public:
    thread_queue_manager();
    ~thread_queue_manager();

    void run();

    void add_to_queue(formula_cell* p);
    void terminate();

private:
    static ::boost::mutex mtx;
    static ::boost::condition_variable cond;
    static bool added_to_queue;
    static bool terminate_requested;
};

::boost::mutex thread_queue_manager::mtx;
::boost::condition_variable thread_queue_manager::cond;
bool thread_queue_manager::added_to_queue = false;
bool thread_queue_manager::terminate_requested = false;

thread_queue_manager::thread_queue_manager()
{
}

thread_queue_manager::~thread_queue_manager()
{
}

void thread_queue_manager::run()
{
    StackPrinter __stack_printer__("thread_queue_manager::run");
    ::boost::unique_lock< ::boost::mutex> guard(mtx);
    while (!terminate_requested)
    {
        cout << "waits..." << endl;
        cout.flush();
        cond.wait(guard);
        if (added_to_queue)
        {
            cout << "added to queue" << endl;
            cout.flush();
            added_to_queue = false;
        }
    }

}

void thread_queue_manager::add_to_queue(formula_cell* p)
{
    {
        ::boost::unique_lock< ::boost::mutex> guard(mtx);
        added_to_queue = true;
    }
    cond.notify_one();
}

void thread_queue_manager::terminate()
{
    {
        ::boost::unique_lock< ::boost::mutex> guard(mtx);
        terminate_requested = true;
    }
    cond.notify_one();
}

int main()
{
    StackPrinter __stack_printer__("::main");
    ::boost::ptr_vector<formula_cell> cells;
    cells.push_back(new formula_cell);
    cells.push_back(new formula_cell);
    cells.push_back(new formula_cell);
    cells.push_back(new formula_cell);
    cells.push_back(new formula_cell);
    cells.push_back(new formula_cell);

#if 1
    thread_queue_manager queue_mgr;
    ::boost::thread thr_queue(::boost::bind(&thread_queue_manager::run, queue_mgr));
    sleep(1);

    ::boost::ptr_vector<formula_cell>::iterator itr, itr_beg = cells.begin(), itr_end = cells.end();
    for (itr = itr_beg; itr != itr_end; ++itr)
    {
        formula_cell* p = &(*itr);
        queue_mgr.add_to_queue(p);
        sleep(1);
    }

    queue_mgr.terminate();
    thr_queue.join();
#else

    ::boost::ptr_vector<formula_cell>::iterator itr, itr_beg = cells.begin(), itr_end = cells.end();
    for (itr = itr_beg; itr != itr_end; ++itr)
        itr->interpret_thread();

    for (itr = itr_beg; itr != itr_end; ++itr)
        itr->interpret_join();
#endif
}
