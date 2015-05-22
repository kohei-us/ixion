/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef IXION_FORMULA_VALUE_STACK_HPP
#define IXION_FORMULA_VALUE_STACK_HPP

#include "ixion/global.hpp"

#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

namespace ixion {

namespace iface {

class formula_model_access;

}

struct abs_address_t;
struct abs_range_t;
class matrix;

/**
 * Type of stack value which can be used as intermediate value during
 * formula interpretation.
 */
enum class stack_value_t {
    value,
    string,
    single_ref,
    range_ref
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

}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
