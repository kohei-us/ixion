/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_FORMULA_VALUE_STACK_HPP
#define INCLUDED_IXION_FORMULA_VALUE_STACK_HPP

#include <ixion/global.hpp>
#include <ixion/model_context.hpp>

#include <deque>

namespace ixion {

struct abs_address_t;
struct abs_range_t;
class matrix;

/**
 * Type of stack value which can be used as intermediate value during
 * formula interpretation.
 */
enum class stack_value_t
{
    value,
    string,
    single_ref,
    range_ref,
    matrix,
};

/**
 * Individual stack value storage.
 */
class stack_value
{
    stack_value_t m_type;

    union
    {
        double m_value;
        abs_address_t* m_address;
        abs_range_t* m_range;
        matrix* m_matrix;
        std::string* m_str;
    };

public:
    stack_value() = delete;
    stack_value(const stack_value&) = delete;
    stack_value& operator= (const stack_value&) = delete;

    explicit stack_value(double val);
    explicit stack_value(std::string str);
    explicit stack_value(const abs_address_t& val);
    explicit stack_value(const abs_range_t& val);
    explicit stack_value(matrix mtx);
    stack_value(stack_value&& other);
    ~stack_value();

    stack_value& operator= (stack_value&& other);

    stack_value_t get_type() const;
    double get_value() const;
    const std::string& get_string() const;
    const abs_address_t& get_address() const;
    const abs_range_t& get_range() const;

    /**
     * Move the matrix value out from storage.  The internal matrix content
     * will be empty after this call.
     */
    matrix pop_matrix();
};

/**
 * FILO stack of values; last pushed value gets popped first.
 */
class formula_value_stack
{
    typedef std::deque<stack_value> store_type;
    store_type m_stack;
    const model_context& m_context;

public:
    formula_value_stack() = delete;
    formula_value_stack(const formula_value_stack&) = delete;
    formula_value_stack& operator= (const formula_value_stack&) = delete;

    explicit formula_value_stack(const model_context& cxt);

    typedef store_type::value_type value_type;
    typedef store_type::iterator iterator;
    typedef store_type::const_iterator const_iterator;
    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;
    value_type release(iterator pos);
    value_type release_back();
    bool empty() const;
    size_t size() const;
    void clear();
    void swap(formula_value_stack& other);

    stack_value& back();
    const stack_value& back() const;
    const stack_value& operator[](size_t pos) const;

    double get_value(size_t pos) const;

    void push_back(value_type&& val);
    void push_value(double val);
    void push_string(std::string str);
    void push_single_ref(const abs_address_t& val);
    void push_range_ref(const abs_range_t& val);
    void push_matrix(matrix mtx);

    double pop_value();
    const std::string pop_string();
    abs_address_t pop_single_ref();
    abs_range_t pop_range_ref();
    matrix pop_range_value();

    stack_value_t get_type() const;
};

}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
