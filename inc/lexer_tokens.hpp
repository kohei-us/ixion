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

#ifndef __IXION_LEXER_TOKENS_HPP__
#define __IXION_LEXER_TOKENS_HPP__

#include <string>

#include <boost/ptr_container/ptr_vector.hpp>

namespace ixion {

class lexer_token_base;

typedef ::boost::ptr_vector<lexer_token_base> lexer_tokens_t;

const char* print_tokens(const lexer_tokens_t& tokens, bool verbose);

// ============================================================================

enum lexer_opcode_t
{
    // data types
    op_value,
    op_string,
    op_name,

    // arithmetic operators
    op_plus,
    op_minus,
    op_divide,
    op_multiply,

    // parentheses, separators
    op_open,
    op_close,
    op_sep,
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
    virtual ::std::string get_string() const;
    virtual const char* print() const = 0;

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
    virtual const char* print() const;
};

// ============================================================================

class lexer_value_token : public lexer_token_base
{
public:
    lexer_value_token(double val);
    lexer_value_token(const lexer_value_token& r);
    virtual ~lexer_value_token();

    virtual double get_value() const;
    virtual const char* print() const;

private:
    double m_val;
};

// ============================================================================

class lexer_string_token : public lexer_token_base
{
public:
    lexer_string_token(const ::std::string& str);
    virtual ~lexer_string_token();

    virtual ::std::string get_string() const;
    virtual const char* print() const;
private:
    ::std::string m_str;
};

// ============================================================================

class lexer_name_token : public lexer_token_base
{
public:
    lexer_name_token(const ::std::string& name);
    virtual ~lexer_name_token();

    virtual ::std::string get_string() const;
    virtual const char* print() const;
private:
    ::std::string m_name;
};

// ============================================================================

// We need the following inline functions for boost::ptr_container.

inline lexer_token_base* new_clone(const lexer_token_base& r)
{
    lexer_opcode_t oc = r.get_opcode();

    switch (oc)
    {
        case op_value:
            return new lexer_value_token(r.get_value());
        case op_string:
            return new lexer_string_token(r.get_string());
        case op_name:
            return new lexer_name_token(r.get_string());
        case op_close:
        case op_divide:
        case op_minus:
        case op_multiply:
        case op_open:
        case op_plus:
        case op_sep:
        default:
            ;
    }

    return new lexer_token(oc);
}

inline void delete_clone(const lexer_token_base* p)
{
    delete p;
}

// ============================================================================

}

#endif
