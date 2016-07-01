/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_FORMULA_TOKENS_HPP
#define INCLUDED_FORMULA_TOKENS_HPP

#include "ixion/address.hpp"
#include "ixion/table.hpp"
#include "ixion/formula_opcode.hpp"

#include <string>
#include <vector>

namespace ixion {

/**
 * Get a printable name for a formula opcode.
 *
 * @param oc formula opcode
 *
 * @return printable name for a formula opcode.
 */
IXION_DLLPUBLIC const char* get_opcode_name(fopcode_t oc);

IXION_DLLPUBLIC const char* get_formula_opcode_string(fopcode_t oc);

class IXION_DLLPUBLIC formula_token
{
public:
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
    virtual size_t get_index() const;
    virtual std::string get_name() const;

private:
    formula_token() = delete;

    fopcode_t m_opcode;
};

typedef std::vector<std::unique_ptr<formula_token>> formula_tokens_t;

IXION_DLLPUBLIC bool operator== (const formula_tokens_t& left, const formula_tokens_t& right);

}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
