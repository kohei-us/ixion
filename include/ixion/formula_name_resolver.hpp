/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __IXION_FORMULA_NAME_RESOLVER_HPP__
#define __IXION_FORMULA_NAME_RESOLVER_HPP__

#include <string>

#include "ixion/address.hpp"
#include "ixion/formula_function_opcode.hpp"

namespace ixion {

namespace iface {

class model_context;

}

struct formula_name_type
{
    enum name_type {
        cell_reference,
        range_reference,
        named_expression,
        function,
        invalid
    };

    struct address_type {
        sheet_t sheet;
        row_t row;
        col_t col;
        bool abs_sheet:1;
        bool abs_row:1;
        bool abs_col:1;
    };

    struct range_type {
        address_type first;
        address_type last;
    };

    name_type type;
    union {
        address_type address;
        range_type range;
        formula_function_t func_oc; // function opcode
    };

    formula_name_type();

    /**
     * Return a string that represents the data stored internally.  Useful for
     * debugging.
     */
    ::std::string to_string() const;
};

IXION_DLLPUBLIC address_t to_address(const formula_name_type::address_type& src);
IXION_DLLPUBLIC range_t to_range(const formula_name_type::range_type& src);

class formula_name_resolver
{
public:
    formula_name_resolver();
    virtual ~formula_name_resolver() = 0;
    virtual formula_name_type resolve(const char* p, size_t n, const abs_address_t& pos) const = 0;
    virtual std::string get_name(const address_t& addr, const abs_address_t& pos, bool sheet_name) const = 0;
    virtual std::string get_name(const range_t& range, const abs_address_t& pos, bool sheet_name) const = 0;

    /**
     * Given a numerical representation of column position, return its
     * textural representation.
     *
     * @param col numerical column position.
     *
     * @return textural representation of column position.
     */
    virtual std::string get_column_name(col_t col) const = 0;

    /**
     * Create a formula name resolver instance according to the requested
     * type.
     *
     * @param type type formula name resolver being requested.
     * @param cxt document model context for resolving sheet names, or NULL in
     *            case names being resolved don't contain sheet names.
     *
     * @return formula name resolver instance created on the heap.  The caller
     *         is responsible for managing its life cycle.
     */
    IXION_DLLPUBLIC static formula_name_resolver*
        get(formula_name_resolver_t type, const iface::model_context* cxt);
};

}

#endif
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
