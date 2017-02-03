/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_CONCRETE_FORMULA_TOKENS_HPP
#define INCLUDED_IXION_CONCRETE_FORMULA_TOKENS_HPP

#include "ixion/formula_tokens.hpp"

namespace ixion {

// ============================================================================

/**
 * Very simple token that stores opcode only.
 */
class opcode_token : public formula_token
{
public:
    explicit opcode_token(fopcode_t oc);
    opcode_token(const opcode_token& r);
    virtual ~opcode_token() override;
    virtual void write_string(std::ostream& os) const override;
};

// ============================================================================

class value_token : public formula_token
{
public:
    explicit value_token(double value);
    virtual ~value_token() override;

    virtual double get_value() const override;
    virtual void write_string(std::ostream& os) const override;

private:
    double m_value;
};

class string_token : public formula_token
{
    string_token() = delete;
public:
    explicit string_token(size_t str_identifier);
    virtual ~string_token() override;

    virtual size_t get_index() const override;
    virtual void write_string(std::ostream& os) const override;

private:
    size_t m_str_identifier;
};

// ============================================================================

/**
 * Token that stores a cell reference.  Note that the address it stores may
 * be either relative to the origin cell or absolute.
 */
class single_ref_token : public formula_token
{
public:
    single_ref_token(const address_t& addr);
    single_ref_token(const single_ref_token& r);
    virtual ~single_ref_token() override;

    virtual address_t get_single_ref() const override;
    virtual void write_string(std::ostream& os) const override;

private:
    address_t m_address;
};

// ============================================================================

class range_ref_token : public formula_token
{
public:
    range_ref_token(const range_t& range);
    range_ref_token(const range_ref_token& r);
    virtual ~range_ref_token() override;

    virtual range_t get_range_ref() const override;
    virtual void write_string(std::ostream& os) const override;

private:
    range_t m_range;
};

class table_ref_token : public formula_token
{
public:
    table_ref_token(const table_t& table);
    table_ref_token(const table_ref_token& r);
    virtual ~table_ref_token() override;

    virtual table_t get_table_ref() const override;
    virtual void write_string(std::ostream& os) const override;

private:
    table_t m_table;
};

// ============================================================================

/**
 * Token that stores a named expression.
 */
class named_exp_token : public formula_token
{
public:
    named_exp_token(const char* p, size_t n);
    named_exp_token(const named_exp_token& r);
    virtual ~named_exp_token() override;
    virtual std::string get_name() const override;
    virtual void write_string(std::ostream& os) const override;

private:
    ::std::string m_name;
};

// ============================================================================

class function_token : public formula_token
{
public:
    function_token(size_t func_oc);
    function_token(const function_token& r);
    virtual ~function_token() override;

    virtual size_t get_index() const override;
    virtual void write_string(std::ostream& os) const override;

private:
    size_t m_func_oc;
};

}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
