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

#ifndef __FORMULA_TOKENS_HPP__
#define __FORMULA_TOKENS_HPP__

#include <boost/ptr_container/ptr_vector.hpp>

namespace ixion {

class formula_token_base;
class base_cell;

typedef ::boost::ptr_vector<formula_token_base> formula_tokens_t;

const char* print_tokens(const formula_tokens_t& tokens, bool verbose);

// ============================================================================

/** formula opcode type */
enum fopcode_t {
    // data types
    fop_single_ref,
    fop_string,
    fop_value,

    // arithmetic operators
    fop_plus,
    fop_minus,
    fop_divide,
    fop_multiply,

    // parentheses, separators
    fop_open,
    fop_close,
    fop_sep,

    // error conditions
    fop_err_no_ref,
};

// ============================================================================

class formula_token_base
{
public:
    formula_token_base(fopcode_t op);
    formula_token_base(const formula_token_base& r);
    virtual ~formula_token_base() = 0;

    fopcode_t get_opcode() const;

    virtual const base_cell* get_single_ref() const;

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
    virtual ~opcode_token();
};

// ============================================================================

/**
 * Token that stores a cell reference.
 */
class single_ref_token : public formula_token_base
{
public:
    single_ref_token(const base_cell* p_cell);
    single_ref_token(const single_ref_token& r);
    virtual ~single_ref_token();

    virtual const base_cell* get_single_ref() const;

private:
    const base_cell* mp_cell; // referenced cell, pointer only.
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
            return new opcode_token(r.get_opcode())
        case fop_single_ref:
            return new single_ref_token(r.get_single_ref());
        case fop_string:
            break;
        case fop_value:
            break;
        default:
            ;
    }
    return NULL;
}

inline void delete_clone(const formula_token_base* p)
{
    delete p;
}

// ============================================================================

}

#endif
