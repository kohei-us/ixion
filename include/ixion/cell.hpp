/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_CELL_HPP
#define INCLUDED_IXION_CELL_HPP

#include "ixion/types.hpp"
#include "ixion/formula_tokens_fwd.hpp"

#include <memory>
#include <vector>

namespace ixion {

class formula_result;
class formula_cell;
struct abs_address_t;
struct rc_address_t;

// calc_status is internal.
struct calc_status;
using calc_status_ptr_t = boost::intrusive_ptr<calc_status>;

namespace iface {

class formula_model_access;

}

class IXION_DLLPUBLIC formula_cell
{
    struct impl;
    std::unique_ptr<impl> mp_impl;

public:
    formula_cell(const formula_cell&) = delete;
    formula_cell& operator= (formula_cell) = delete;

    formula_cell();
    formula_cell(const formula_tokens_store_ptr_t& tokens);

    formula_cell(
        row_t group_row, col_t group_col,
        const calc_status_ptr_t& cs,
        const formula_tokens_store_ptr_t& tokens);

    ~formula_cell();

    const formula_tokens_store_ptr_t& get_tokens() const;
    void set_tokens(const formula_tokens_store_ptr_t& tokens);

    double get_value() const;
    double get_value_nowait() const;
    void interpret(iface::formula_model_access& context, const abs_address_t& pos);

    /**
     * Determine if this cell contains circular reference by walking through
     * all its reference tokens.
     */
    void check_circular(const iface::formula_model_access& cxt, const abs_address_t& pos);

    /**
     * Reset cell's internal state.
     */
    void reset();

    /**
     * Get a series of all reference tokens included in the formula
     * expression stored in this cell.
     *
     * @param cxt model context instance.
     * @param pos position of the cell.
     *
     * @return an array of reference formula tokens.  Each element is a
     *         pointer to the actual token instance stored in the cell object.
     *         Be aware that the pointer is valid only as long as the actual
     *         token instance is alive.
     */
    std::vector<const formula_token*> get_ref_tokens(
        const iface::formula_model_access& cxt, const abs_address_t& pos) const;

    const formula_result& get_result_cache() const;
    const formula_result* get_result_cache_nowait() const;
};

}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
