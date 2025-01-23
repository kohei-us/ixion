/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_DOCUMENT_HPP
#define INCLUDED_IXION_DOCUMENT_HPP

#include "types.hpp"
#include "address.hpp"

#include <memory>
#include <string>
#include <variant>

namespace ixion {

class cell_access;

/**
 * Higher level document representation designed to handle both cell value
 * storage as well as formula cell calculations.
 */
class IXION_DLLPUBLIC document
{
    struct impl;
    std::unique_ptr<impl> mp_impl;
public:
    document();

    /**
     * Constructor with custom cell address type.
     *
     * @param cell_address_type cell address type to use for cell addresses
     *                          represented by string values.
     */
    document(formula_name_resolver_t cell_address_type);
    ~document();

    struct IXION_DLLPUBLIC cell_pos
    {
        enum class cp_type { string, address };
        cp_type type;

        std::variant<std::string_view, ixion::abs_address_t> value;

        cell_pos() = delete;
        cell_pos(const char* p);
        cell_pos(std::string_view s);
        cell_pos(const std::string& s);
        cell_pos(const abs_address_t& addr);
        cell_pos(const cell_pos& other);

        cell_pos& operator=(const cell_pos& other);
    };

    void append_sheet(std::string name);

    /**
     * Set a new name to an existing sheet.
     *
     * @param sheet 0-based sheet index.
     * @param name New name of a sheet.
     */
    void set_sheet_name(sheet_t sheet, std::string name);

    cell_access get_cell_access(cell_pos pos) const;

    void set_numeric_cell(cell_pos pos, double val);

    void set_string_cell(cell_pos pos, std::string_view s);

    void set_boolean_cell(cell_pos pos, bool val);

    void empty_cell(cell_pos pos);

    double get_numeric_value(cell_pos pos) const;

    std::string_view get_string_value(cell_pos pos) const;

    void set_formula_cell(cell_pos pos, std::string_view formula);

    /**
     * Calculate all the "dirty" formula cells in the document.
     *
     * @param thread_count number of threads to use to perform calculation.
     *                     When 0 is specified, it only uses the main thread.
     */
    void calculate(size_t thread_count);
};

}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
