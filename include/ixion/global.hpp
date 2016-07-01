/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_GLOBAL_HPP
#define INCLUDED_IXION_GLOBAL_HPP

#include "ixion/types.hpp"
#include "ixion/env.hpp"

#include <memory>
#include <string>

#define __IXION_DEBUG_OUT__ ::std::cout << __FILE__ << "#" << __LINE__ << ": "

namespace ixion {

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

/**
 * Formula error types.
 */
enum class formula_error_t
{
    no_error = 0,
    ref_result_not_available = 1,
    division_by_zero = 2,
    invalid_expression = 3,
    stack_error = 4,
    general_error = 5,
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

template<typename T, typename ...Args>
std::unique_ptr<T> make_unique(Args&& ...args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

inline bool is_digit(char c)
{
    return '0' <= c && c <= '9';
}

}

#endif
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
