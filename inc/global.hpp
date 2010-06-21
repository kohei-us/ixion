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

#ifndef __IXION_GLOBAL_HPP__
#define __IXION_GLOBAL_HPP__

#include <string>
#include <unordered_set>
#include <unordered_map>
#include <boost/ptr_container/ptr_map.hpp>

namespace ixion {

class base_cell;
class formula_cell;

typedef ::boost::ptr_map< ::std::string, base_cell>             cell_name_ptr_map_t;
typedef ::std::unordered_map<const base_cell*, ::std::string>   cell_ptr_name_map_t;

/**
 * Dirty cells are those cells that have been modified or cells that 
 * reference modified cells.  Note that dirty cells can be of any cell 
 * types. 
 */
typedef ::std::unordered_set<base_cell*> dirty_cells_t;

const char* get_formula_result_output_separator();


// ============================================================================

class global
{
public:
    static void build_ptr_name_map(const cell_name_ptr_map_t& cells, cell_ptr_name_map_t& cell_names);
    static void set_cell_name_map(const cell_ptr_name_map_t* p);
    static ::std::string get_cell_name(const base_cell* cell);

    /**
     * Get current time in seconds since epoch.  Note that the value 
     * representing a time may differ from platform to platform.  Use this 
     * value only to measure relative time. 
     * 
     * @return current time in seconds since epoch.
     */
    static double get_current_time();

    /**
     * Suspend execution of the calling thread for specified seconds.
     * 
     * @param seconds duration of sleep.
     */
    static void sleep(unsigned int seconds);

private:
    global();
    global(const global& r);
    ~global();
};

// ============================================================================

class general_error : public ::std::exception
{
public:
    explicit general_error(const ::std::string& msg);
    ~general_error() throw();
    virtual const char* what() const throw();
private:
    ::std::string m_msg;
};

// ============================================================================

enum formula_error_t
{
    fe_no_error = 0,
    fe_ref_result_not_available,
    fe_division_by_zero,
    fe_invalid_expression
};

const char* get_formula_error_name(formula_error_t fe);

// ============================================================================

class formula_error : public ::std::exception
{
public:
    explicit formula_error(formula_error_t fe);
    ~formula_error() throw();
    virtual const char* what() const throw();

    formula_error_t get_error() const;
private:
    formula_error_t m_ferror;
};

}

#endif
