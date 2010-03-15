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

#ifndef __TOKENS_HPP__
#define __TOKENS_HPP__

#include <string>

namespace ixion {

// ============================================================================

enum opcode_t {
    // data types
    oc_value,
    oc_string,
    oc_name,

    // arithmetic operators
    oc_plus,
    oc_minus,
    oc_divide,
    oc_multiply,

    // parentheses, separators
    oc_open,
    oc_close,
    oc_sep,
};

const char* get_opcode_name(opcode_t oc);

// ============================================================================

class token_base
{
public:
    token_base(opcode_t oc);
    token_base(const token_base& r);
    virtual ~token_base();

    virtual double get_value() const;
    virtual ::std::string get_string() const;
    virtual const char* print() const = 0;

    opcode_t get_opcode() const;
private:
    opcode_t m_opcode;
};

// ============================================================================

class token : public token_base
{
public:
    token(opcode_t oc);
    virtual ~token();
    virtual const char* print() const;
};

// ============================================================================

class value_token : public token_base
{
public:
    value_token(double val);
    value_token(const value_token& r);
    virtual ~value_token();

    virtual double get_value() const;
    virtual const char* print() const;

private:
    double m_val;
};

// ============================================================================

class string_token : public token_base
{
public:
    string_token(const ::std::string& str);
    virtual ~string_token();

    virtual ::std::string get_string() const;
    virtual const char* print() const;
private:
    ::std::string m_str;
};

// ============================================================================

class name_token : public token_base
{
public:
    name_token(const ::std::string& name);
    virtual ~name_token();

    virtual ::std::string get_string() const;
    virtual const char* print() const;
private:
    ::std::string m_name;
};

// ============================================================================

// We need the following inline functions for boost::ptr_container.

inline token_base* new_clone(const token_base& r)
{
    opcode_t oc = r.get_opcode();
    switch (oc)
    {
        case oc_value:
            return new value_token(r.get_value());
        case oc_string:
            return new string_token(r.get_string());
        case oc_name:
            return new name_token(r.get_string());
    }

    return new token(oc);
}

inline void delete_clone(const token_base* p)
{
    delete p;
}

// ============================================================================

}

#endif
