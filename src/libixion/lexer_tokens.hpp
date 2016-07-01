/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_LEXER_TOKENS_HPP
#define INCLUDED_IXION_LEXER_TOKENS_HPP

#include "ixion/mem_str_buf.hpp"
#include "ixion/env.hpp"

#include <vector>
#include <memory>

namespace ixion {

class lexer_token_base;

typedef std::vector<std::unique_ptr<lexer_token_base>> lexer_tokens_t;

std::string print_tokens(const lexer_tokens_t& tokens, bool verbose);

// ============================================================================

enum class lexer_opcode_t
{
    // data types
    value,
    string,
    name,

    // arithmetic operators
    plus,
    minus,
    divide,
    multiply,

    // relational operators
    equal,
    less,
    greater,

    // parentheses, separators
    open,
    close,
    sep,
};

const char* get_opcode_name(lexer_opcode_t oc);

// ============================================================================

class lexer_token_base
{
public:
    lexer_token_base(lexer_opcode_t oc);
    lexer_token_base(const lexer_token_base& r);
    virtual ~lexer_token_base();

    virtual double get_value() const;
    virtual mem_str_buf get_string() const;
    virtual ::std::string print() const = 0;

    lexer_opcode_t get_opcode() const;
private:
    lexer_opcode_t m_opcode;
};

// ============================================================================

class lexer_token : public lexer_token_base
{
public:
    lexer_token(lexer_opcode_t oc);
    virtual ~lexer_token();
    virtual ::std::string print() const;
};

// ============================================================================

class lexer_value_token : public lexer_token_base
{
public:
    lexer_value_token(double val);
    lexer_value_token(const lexer_value_token& r);
    virtual ~lexer_value_token();

    virtual double get_value() const;
    virtual ::std::string print() const;

private:
    double m_val;
};

// ============================================================================

class lexer_string_token : public lexer_token_base
{
public:
    lexer_string_token(const char* p, size_t n);
    lexer_string_token(const lexer_string_token& r);
    virtual ~lexer_string_token();

    virtual mem_str_buf get_string() const;
    virtual ::std::string print() const;
private:
    mem_str_buf m_str;
};

// ============================================================================

class lexer_name_token : public lexer_token_base
{
public:
    lexer_name_token(const char* p, size_t n);
    lexer_name_token(const lexer_name_token& r);
    virtual ~lexer_name_token();

    virtual mem_str_buf get_string() const;
    virtual ::std::string print() const;
private:
    mem_str_buf m_str;
};

}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
