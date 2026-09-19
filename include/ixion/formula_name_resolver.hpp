/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once
#include "address.hpp"
#include "formula_function_opcode.hpp"

#include <string>
#include <memory>
#include <variant>

namespace ixion {

class model_context;
struct table_ref_t;

/**
 * Structure that represents the type of a 'name' in a formula expression.
 *
 * A name can be either one of:
 * <ul>
 * <li>cell reference</li>
 * <li>range reference</li>
 * <li>table reference</li>
 * <li>named expression</li>
 * <li>function</li>
 * </ul>
 */
struct IXION_DLLPUBLIC formula_name_t
{
    /** Kind of name. */
    enum name_type
    {
        /** The string is not a valid name of any kind. */
        invalid = 0,
        /** Reference to a single cell. */
        cell_reference,
        /** Reference to a range of cells. */
        range_reference,
        /** Reference to a table or a part of one. */
        table_reference,
        /** Named expression. */
        named_expression,
        /** Built-in function. */
        function,
    };

    /**
     * Table information for a table reference name.  Unlike the ixion::table_ref_t
     * counterpart, we store strings as string views as the resolver doesn't
     * have access to the string pool.
     */
    struct table_type
    {
        /** Name of the table, or empty when the reference does not name it. */
        std::string_view name;
        /** Name of the first column, or empty for an area-only reference. */
        std::string_view column_first;
        /** Name of the last column, or empty for a single-column reference. */
        std::string_view column_last;
        /** Areas covered, as a combination of table_area_t flags. */
        table_areas_t areas;
    };

    /**
     * Type of the resolved value: an address for a cell reference, a range
     * for a range reference, a table_type for a table reference, or a
     * function opcode for a function.  A named expression has no value.
     */
    using value_type = std::variant<address_t, range_t, table_type, formula_function_t>;

    /** Kind of name. */
    name_type type;
    /** Resolved value, whose alternative depends on the kind of name. */
    value_type value;

    formula_name_t();

    /**
     * Return a string that represents the data stored internally.  Useful for
     * debugging.
     */
    std::string to_string() const;
};

/**
 * Formula name resolvers resolves a name in a formula expression to a more
 * concrete name type.
 */
class formula_name_resolver
{
public:
    formula_name_resolver();
    virtual ~formula_name_resolver() = 0;

    /**
     * Parse and resolve a reference string.
     *
     * @param s reference string to be parsed.
     * @param pos base cell position, which influences the resolved reference
     *            position(s) containing relative address(es).  When the
     *            reference string does not contain an explicit sheet name,
     *            the sheet address of the base cell position is used.
     *
     * @return result of the resovled reference.
     */
    virtual formula_name_t resolve(std::string_view s, const abs_address_t& pos) const = 0;

    /**
     * Get the string representation of a cell reference in the syntax of
     * this resolver.
     *
     * @param addr Cell reference, whose components, if relative, are offsets
     *             from the base position.
     * @param pos Base cell position the relative components, if any, are
     *            resolved against.
     * @param sheet_name Whether to include the sheet name.
     *
     * @return String representation of the reference.
     */
    virtual std::string get_name(const address_t& addr, const abs_address_t& pos, bool sheet_name) const = 0;

    /**
     * Get the string representation of a range reference in the syntax of
     * this resolver.
     *
     * @param range Range reference, whose components, if relative, are
     *              offsets from the base position.
     * @param pos Base cell position the relative components, if any, are
     *            resolved against.
     * @param sheet_name Whether to include the sheet name.
     *
     * @return String representation of the reference.
     */
    virtual std::string get_name(const range_t& range, const abs_address_t& pos, bool sheet_name) const = 0;

    /**
     * Get the string representation of a table reference in the syntax of
     * this resolver.
     *
     * @param table Table reference.
     *
     * @return String representation of the reference.
     */
    virtual std::string get_name(const table_ref_t& table) const = 0;

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
     * @param cxt document model context for resolving sheet names, or nullptr
     *            in case names being resolved don't contain sheet names.
     *
     * @return formula name resolver instance created on the heap.  The caller
     *         is responsible for managing its life cycle.
     */
    IXION_DLLPUBLIC static std::unique_ptr<formula_name_resolver>
        get(formula_name_resolver_t type, const model_context* cxt);
};

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
