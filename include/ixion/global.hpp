/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __IXION_GLOBAL_HPP__
#define __IXION_GLOBAL_HPP__

#include "ixion/types.hpp"
#include "ixion/env.hpp"

#include <memory>
#include <string>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/interprocess/smart_ptr/unique_ptr.hpp>
#include <boost/noncopyable.hpp>

#define __IXION_DEBUG_OUT__ ::std::cout << __FILE__ << "#" << __LINE__ << ": "

namespace ixion {

class formula_cell;
struct abs_address_t;
struct abs_range_t;
class matrix;

namespace iface {

class formula_model_access;

}

IXION_DLLPUBLIC const char* get_formula_result_output_separator();

// ============================================================================

class IXION_DLLPUBLIC global
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
    static void sleep(unsigned int mseconds);

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

IXION_DLLPUBLIC const char* get_formula_error_name(formula_error_t fe);

// ============================================================================

class IXION_DLLPUBLIC formula_error : public std::exception
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
class stack_value : boost::noncopyable
{
    stack_value_t m_type;
    union {
        double m_value;
        abs_address_t* m_address;
        abs_range_t* m_range;
        size_t m_str_identifier;
    };

public:
    explicit stack_value(double val);
    explicit stack_value(size_t sid);
    explicit stack_value(const abs_address_t& val);
    explicit stack_value(const abs_range_t& val);
    ~stack_value();

    stack_value_t get_type() const;
    double get_value() const;
    size_t get_string() const;
    const abs_address_t& get_address() const;
    const abs_range_t& get_range() const;
};

class value_stack_t
{
    typedef ::boost::ptr_vector<stack_value> store_type;
    store_type m_stack;
    const iface::formula_model_access& m_context;

    value_stack_t(); // disabled
public:
    explicit value_stack_t(const iface::formula_model_access& cxt);

    typedef store_type::auto_type auto_type;
    typedef store_type::iterator iterator;
    typedef store_type::const_iterator const_iterator;
    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;
    auto_type release(iterator pos);
    bool empty() const;
    size_t size() const;
    void clear();
    void swap(value_stack_t& other);

    const stack_value& back() const;
    const stack_value& operator[](size_t pos) const;

    double get_value(size_t pos) const;

    void push_back(auto_type val);
    void push_value(double val);
    void push_string(size_t sid);
    void push_single_ref(const abs_address_t& val);
    void push_range_ref(const abs_range_t& val);

    double pop_value();
    const std::string pop_string();
    abs_address_t pop_single_ref();
    abs_range_t pop_range_ref();
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

template<typename _T>
struct default_deleter
{
    void operator() (const _T* p) const
    {
        delete p;
    }
};

template<typename _T>
class unique_ptr : public boost::interprocess::unique_ptr<_T, default_deleter<_T> >
{
public:
    unique_ptr(_T* p) : boost::interprocess::unique_ptr<_T, default_deleter<_T> >(p) {}
};

template<typename T, typename ...Args>
std::unique_ptr<T> make_unique(Args&& ...args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

}

#endif
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
