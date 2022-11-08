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
#include "formula_function_opcode.hpp"
#include "formula_tokens_fwd.hpp"

#include <string>
#include <variant>

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

struct IXION_DLLPUBLIC formula_token final
{
    using value_type = std::variant<
        address_t, range_t, table_t, formula_function_t,
        double, string_id_t, std::size_t, std::string>;

    fopcode_t opcode;
    value_type value;

    formula_token() = delete;

    formula_token(fopcode_t op);
    formula_token(const address_t& addr);
    formula_token(const range_t& range);
    formula_token(const table_t& table);
    formula_token(formula_function_t func);
    formula_token(double v);
    formula_token(string_id_t sid);
    formula_token(std::string name);
    formula_token(const formula_token& r);
    formula_token(formula_token&& r);
    ~formula_token();

    bool operator== (const formula_token& r) const;
    bool operator!= (const formula_token& r) const;

    void write_string(std::ostream& os) const;
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
