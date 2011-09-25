/*************************************************************************
 *
 * Copyright (c) 2010, 2011 Kohei Yoshida
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

#include "ixion/types.hpp"

#include <string>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#define __IXION_DEBUG_OUT__ ::std::cout << __FILE__ << "#" << __LINE__ << ": "

namespace ixion {

extern const sheet_t global_scope;

class base_cell;
class formula_cell;
struct abs_address_t;
struct abs_range_t;
class matrix;

namespace interface {

class model_context;

}

const char* get_formula_result_output_separator();

// ============================================================================

class global
{
public:
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

    /**
     * Load the entire content of a file on disk.  When loading, this function
     * appends an extra character to the end, the pointer to which can be used
     * as the position past the last character.
     *
     * @param filepath path of the file to load.
     *
     * @param content string instance to pass the file content to.
     */
    static void load_file_content(const ::std::string& filepath, ::std::string& content);

    static double to_double(const char* p, size_t n);
private:
    global();
    global(const global& r);
    ~global();
};

// ============================================================================

enum formula_error_t
{
    fe_no_error = 0,
    fe_ref_result_not_available,
    fe_division_by_zero,
    fe_invalid_expression,
    fe_stack_error,
    fe_general_error,
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

/**
 * Type of stack value which can be used as intermediate value during
 * formula interpretation.
 */
enum stack_value_t {
    sv_value = 0,
    sv_string,
    sv_single_ref,
    sv_range_ref,
};

/**
 * Individual stack value storage.
 */
class stack_value
{
    stack_value_t m_type;
    union {
        double m_value;
        abs_address_t* m_address;
        abs_range_t* m_range;
        ::std::string* m_str;
    };

public:
    explicit stack_value(double val);
    explicit stack_value(const abs_address_t& val);
    explicit stack_value(const abs_range_t& val);
    ~stack_value();

    stack_value_t get_type() const;
    double get_value() const;
    const std::string* get_string() const;
    const abs_address_t& get_address() const;
    const abs_range_t& get_range() const;
};

class value_stack_t
{
    typedef ::boost::ptr_vector<stack_value> store_type;
    store_type m_stack;
    const interface::model_context& m_context;

    value_stack_t(); // disabled
public:
    explicit value_stack_t(const interface::model_context& cxt);

    typedef store_type::const_iterator const_iterator;
    const_iterator begin() const;
    const_iterator end() const;
    bool empty() const;
    size_t size() const;
    void clear();
    const stack_value& back() const;

    void push_value(double val);
    void push_single_ref(const abs_address_t& val);
    void push_range_ref(const abs_range_t& val);
    double pop_value();
    matrix pop_range_value();
    stack_value_t get_type() const;
};

template<typename _T>
struct delete_element : public std::unary_function<_T*, void>
{
    void operator() (_T* p)
    {
        delete p;
    }
};

template<typename _T>
struct delete_map_value : public std::unary_function<typename _T::value_type, void>
{
    void operator() (typename _T::value_type& v)
    {
        delete v.second;
    }
};

}

#endif
