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

#ifndef __FORMULA_TOKENS_HPP__
#define __FORMULA_TOKENS_HPP__

#include <string>
#include <boost/ptr_container/ptr_vector.hpp>

#include "ixion/address.hpp"
#include "ixion/formula_tokens_fwd.hpp"
#include "ixion/formula_opcode.hpp"

namespace ixion {

/**
 * Get a printable name for a formula opcode.
 *
 * @param oc formula opcode
 *
 * @return printable name for a formula opcode.
 */
const char* get_opcode_name(fopcode_t oc);

const char* get_formula_opcode_string(fopcode_t oc);

// ============================================================================

class formula_token_base
{
public:
    formula_token_base(fopcode_t op);
    formula_token_base(const formula_token_base& r);
    virtual ~formula_token_base() = 0;

    fopcode_t get_opcode() const;

    virtual address_t get_single_ref() const;
    virtual range_t get_range_ref() const;
    virtual double get_value() const;
    virtual size_t get_index() const;
    virtual std::string get_name() const;

private:
    formula_token_base(); // disabled

    fopcode_t m_opcode;
};

// ============================================================================

/**
 * Very simple token that stores opcode only.
 */
class opcode_token : public formula_token_base
{
public:
    explicit opcode_token(fopcode_t oc);
    opcode_token(const opcode_token& r);
    virtual ~opcode_token();
};

// ============================================================================

class value_token : public formula_token_base
{
public:
    explicit value_token(double value);
    virtual ~value_token();

    virtual double get_value() const;
private:
    double m_value;
};

// ============================================================================

/**
 * Token that stores an unresolved name.
 */
class unresolved_ref_token : public formula_token_base
{
public:
    unresolved_ref_token(const std::string& name);
    virtual ~unresolved_ref_token();

    virtual std::string get_name() const;

private:
    std::string m_name; // unresolved reference name
};

// ============================================================================

/**
 * Token that stores a cell reference.  Note that the address it stores may
 * be either relative to the origin cell or absolute.
 */
class single_ref_token : public formula_token_base
{
public:
    single_ref_token(const address_t& addr);
    single_ref_token(const single_ref_token& r);
    virtual ~single_ref_token();

    virtual address_t get_single_ref() const;

private:
    address_t m_address;
};

// ============================================================================

class range_ref_token : public formula_token_base
{
public:
    range_ref_token(const range_t& range);
    range_ref_token(const range_ref_token& r);
    virtual ~range_ref_token();

    virtual range_t get_range_ref() const;

private:
    range_t m_range;
};

// ============================================================================

/**
 * Token that stores a named expression.
 */
class named_exp_token : public formula_token_base
{
public:
    named_exp_token(const char* p, size_t n);
    named_exp_token(const named_exp_token& r);
    virtual ~named_exp_token();
    virtual std::string get_name() const;
private:
    ::std::string m_name;
};

// ============================================================================

class function_token : public formula_token_base
{
public:
    function_token(size_t func_oc);
    function_token(const function_token& r);
    virtual ~function_token();

    virtual size_t get_index() const;

private:
    size_t m_func_oc;
};

// ============================================================================

// We need the following inline functions for boost::ptr_container.

inline formula_token_base* new_clone(const formula_token_base& r)
{
    switch (r.get_opcode())
    {
        case fop_close:
        case fop_divide:
        case fop_minus:
        case fop_multiply:
        case fop_open:
        case fop_plus:
        case fop_sep:
            return new opcode_token(r.get_opcode());
        case fop_single_ref:
            return new single_ref_token(r.get_single_ref());
        case fop_range_ref:
            return new range_ref_token(r.get_range_ref());
        case fop_named_expression:
            return new named_exp_token(static_cast<const named_exp_token&>(r));
        case fop_unresolved_ref:
            return new unresolved_ref_token(r.get_name());
        case fop_string:
            break;
        case fop_value:
            return new value_token(r.get_value());
            break;
        case fop_function:
            return new function_token(r.get_index());
        default:
            ;
    }
    return NULL;
}

// ============================================================================

}

#endif
