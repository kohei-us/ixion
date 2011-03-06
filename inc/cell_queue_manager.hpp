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

#ifndef __IXION_CELL_QUEUE_MANAGER_HPP__
#define __IXION_CELL_QUEUE_MANAGER_HPP__

#include "global.hpp"

#include <cstdlib>

namespace ixion {

class formula_cell;
class model_context;

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
    static void init(size_t thread_count, const cell_ptr_name_map_t& names, const model_context& context);

    /**
     * Add new cell to queue to interpret. 
     * 
     * @param cell pointer to cell instance to interpret.
     */
    static void add_cell(formula_cell* cell);

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
