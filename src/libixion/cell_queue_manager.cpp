/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "cell_queue_manager.hpp"
#include "queue_entry.hpp"
#include "ixion/cell.hpp"

#include "ixion/interface/formula_model_access.hpp"

#include <cassert>
#include <queue>
#include <future>
#include <algorithm>

#if !IXION_THREADS
#error "This file is not to be compiled when the threads are disabled."
#endif

namespace ixion {

namespace {

class scoped_guard
{
    std::thread m_thread;
public:
    scoped_guard(std::thread thread) : m_thread(std::move(thread)) {}
    scoped_guard(scoped_guard&& other) : m_thread(std::move(other.m_thread)) {}

    scoped_guard(const scoped_guard&) = delete;
    scoped_guard& operator= (const scoped_guard&) = delete;

    ~scoped_guard()
    {
        m_thread.join();
    }
};

class interpreter_queue
{
    using future_type = std::future<void>;

    iface::formula_model_access& m_context;

    std::queue<future_type> m_futures;
    std::mutex m_mtx;
    std::condition_variable m_cond;

    size_t m_max_queue;

    void interpret(formula_cell* p, const abs_address_t& pos)
    {
        p->interpret(m_context, pos);
    }

public:
    interpreter_queue(iface::formula_model_access& cxt, size_t max_queue) :
        m_context(cxt), m_max_queue(max_queue) {}

    /**
     * Push one formula cell to the interpreter queue for future
     * intepretation.
     *
     * @param p pointer to formula cell instance.
     * @param pos position of the formual cell.
     */
    void push(formula_cell* p, const abs_address_t& pos)
    {
        std::unique_lock<std::mutex> lock(m_mtx);

        while (m_futures.size() >= m_max_queue)
            m_cond.wait(lock);

        future_type f = std::async(
            std::launch::async, &interpreter_queue::interpret, this, p, pos);
        m_futures.push(std::move(f));
        lock.unlock();

        m_cond.notify_one();
    }

    /**
     * Wait for one formula cell to finish its interpretation.
     */
    void wait_one()
    {
        std::unique_lock<std::mutex> lock(m_mtx);

        while (m_futures.empty())
            m_cond.wait(lock);

        future_type ret = std::move(m_futures.front());
        m_futures.pop();
        lock.unlock();

        ret.get();  // This may throw if an exception was thrown on the thread.

        m_cond.notify_one();
    }
};

}

struct formula_cell_queue::impl
{
    iface::formula_model_access& m_context;
    std::vector<queue_entry> m_cells;
    size_t m_thread_count;

    impl(iface::formula_model_access& cxt, std::vector<queue_entry>&& cells, size_t thread_count) :
        m_context(cxt),
        m_cells(cells),
        m_thread_count(thread_count) {}

    void thread_launch(interpreter_queue* queue)
    {
        for (queue_entry& e : m_cells)
            queue->push(e.p, e.pos);
    }

    void run()
    {
        interpreter_queue queue(m_context, m_thread_count);

        std::thread t(&formula_cell_queue::impl::thread_launch, this, &queue);
        scoped_guard guard(std::move(t));

        for (size_t i = 0, n = m_cells.size(); i < n; ++i)
            queue.wait_one();
    }
};

formula_cell_queue::formula_cell_queue(
    iface::formula_model_access& cxt, std::vector<queue_entry>&& cells, size_t thread_count) :
    mp_impl(std::make_unique<impl>(cxt, std::move(cells), thread_count)) {}

formula_cell_queue::~formula_cell_queue() {}

void formula_cell_queue::run()
{
    mp_impl->run();
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
