/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_CELL_ACCESS_HPP
#define INCLUDED_IXION_CELL_ACCESS_HPP

#include "types.hpp"

#include <memory>
#include <string>

namespace ixion {

class model_context;
class formula_cell;
class formula_result;
struct abs_address_t;

/**
 * This class provides a read-only access to a single cell.  It's more
 * efficient to use this class if you need to make multiple successive
 * queries to the same cell.
 *
 * Note that an instance of this class will get invalidated when the content
 * of ixion::model_context is modified.
 */
class IXION_DLLPUBLIC cell_access
{
    friend class model_context;

    struct impl;
    std::unique_ptr<impl> mp_impl;

    cell_access(const model_context& cxt, const abs_address_t& addr);
public:
    cell_access(cell_access&& other);
    cell_access& operator= (cell_access&& other);
    ~cell_access();

    cell_t get_type() const;

    cell_value_t get_value_type() const;

    const formula_cell* get_formula_cell() const;

    formula_result get_formula_result() const;

    double get_numeric_value() const;

    bool get_boolean_value() const;

    std::string_view get_string_value() const;

    string_id_t get_string_identifier() const;

    formula_error_t get_error_value() const;
};

}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
