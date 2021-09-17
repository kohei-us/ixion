/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_FORMULA_TOKENS_HPP
#define INCLUDED_FORMULA_TOKENS_HPP

#include "address.hpp"
#include "table.hpp"
#include "formula_opcode.hpp"
#include "formula_tokens_fwd.hpp"

#include <string>

namespace ixion {

/**
 * Get a printable name for a formula opcode.  The printable name is to be
 * used only for informational purposes.
 *
 * @param oc formula opcode
 *
 * @return printable name for a formula opcode.
 */
IXION_DLLPUBLIC std::string_view get_opcode_name(fopcode_t oc);

/**
 * Get the string representation of a simple formula opcode.  This function
 * will return a non-empty string only for operator opcodes.
 *
 * @param oc formula opcode
 *
 * @return string representation of a formula opcode.
 */
IXION_DLLPUBLIC std::string_view get_formula_opcode_string(fopcode_t oc);

class IXION_DLLPUBLIC formula_token
{
    fopcode_t m_opcode;

public:
    formula_token() = delete;

    formula_token(fopcode_t op);
    formula_token(const formula_token& r);
    virtual ~formula_token() = 0;

    fopcode_t get_opcode() const;

    bool operator== (const formula_token& r) const;
    bool operator!= (const formula_token& r) const;

    virtual address_t get_single_ref() const;
    virtual range_t get_range_ref() const;
    virtual table_t get_table_ref() const;
    virtual double get_value() const;
    virtual uint32_t get_uint32() const;
    virtual std::string get_name() const;
    virtual void write_string(std::ostream& os) const;
};

class IXION_DLLPUBLIC formula_tokens_store
{
    friend void intrusive_ptr_add_ref(formula_tokens_store*);
    friend void intrusive_ptr_release(formula_tokens_store*);

    struct impl;
    std::unique_ptr<impl> mp_impl;

    void add_ref();
    void release_ref();

    formula_tokens_store();

public:

    static formula_tokens_store_ptr_t create();

    ~formula_tokens_store();

    formula_tokens_store(const formula_tokens_store&) = delete;
    formula_tokens_store& operator= (const formula_tokens_store&) = delete;

    size_t get_reference_count() const;

    formula_tokens_t& get();
    const formula_tokens_t& get() const;
};

inline void intrusive_ptr_add_ref(formula_tokens_store* p)
{
    p->add_ref();
}

inline void intrusive_ptr_release(formula_tokens_store* p)
{
    p->release_ref();
}

struct IXION_DLLPUBLIC named_expression_t
{
    abs_address_t origin;
    formula_tokens_t tokens;

    named_expression_t();
    named_expression_t(const abs_address_t& _origin, formula_tokens_t _tokens);
    named_expression_t(const named_expression_t&) = delete;
    named_expression_t(named_expression_t&& other);
    ~named_expression_t();
};

IXION_DLLPUBLIC bool operator== (const formula_tokens_t& left, const formula_tokens_t& right);

IXION_DLLPUBLIC std::ostream& operator<< (std::ostream& os, const formula_token& ft);

}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
