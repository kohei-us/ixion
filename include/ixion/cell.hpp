/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once
#include "types.hpp"
#include "formula_tokens_fwd.hpp"

#include <memory>
#include <vector>
#include <string>

namespace ixion {

namespace detail { class model_context_impl; }

class formula_result;
class formula_cell;
class model_context;
struct abs_address_t;
struct rc_address_t;

// calc_status is internal.
struct calc_status;
using calc_status_ptr_t = boost::intrusive_ptr<calc_status>;

/**
 * A formula cell.  It holds its formula tokens, which may be shared with
 * other formula cells, and its cached result.  A formula cell may belong to a
 * formula group, in which case all cells of the group share one set of tokens
 * and one result as a matrix, and each cell reads its own element of that
 * matrix.
 */
class IXION_DLLPUBLIC formula_cell
{
    friend class detail::model_context_impl;

    struct impl;
    std::unique_ptr<impl> mp_impl;

    /**
     * Replace the store of the formula tokens of this cell.  The cached
     * result is left as is, and the dependency tracker is not informed.
     *
     * @param tokens Token store to use.
     */
    void set_tokens(const formula_tokens_store_ptr_t& tokens);

public:
    formula_cell(const formula_cell&) = delete;
    formula_cell& operator= (formula_cell) = delete;

    /** Construct a formula cell with no tokens. */
    formula_cell();
    /** Construct a formula cell that shares the specified token store. */
    formula_cell(const formula_tokens_store_ptr_t& tokens);

    /**
     * Construct a member of a formula group.  This constructor is used by
     * model_context::set_grouped_formula_cells().
     *
     * @param group_row Row position of the cell within its group.
     * @param group_col Column position of the cell within its group.
     * @param cs Calculation status shared by all cells of the group.
     * @param tokens Token store shared by all cells of the group.
     */
    formula_cell(
        row_t group_row, col_t group_col,
        const calc_status_ptr_t& cs,
        const formula_tokens_store_ptr_t& tokens);

    ~formula_cell();

    /**
     * Function object that clones formula cell instances such that the
     * cloned cells whose source cells belong to the same group share the
     * same cloned calc status instance.  Use one cloner instance per batch
     * of cells being cloned together, such as cells stored in the same column.
     *
     * A cloned cell shares the formula token store with its source cell,
     * and inherits the source cell's position within a group in case the
     * source cell is grouped.
     */
    class IXION_DLLPUBLIC cloner
    {
        struct impl;
        std::unique_ptr<impl> mp_impl;

    public:
        cloner(const cloner&) = delete;
        cloner& operator=(const cloner&) = delete;

        cloner();
        ~cloner();

        /**
         * Create a new instance that is a clone of the source instance.
         *
         * @param src Source instance to clone.
         *
         * @return Cloned instance.
         */
        std::unique_ptr<formula_cell> operator()(const formula_cell& src);
    };

    /**
     * Get the store of the formula tokens of this cell.
     *
     * @return Pointer to the token store, which may be shared with other
     *         formula cells.
     */
    const formula_tokens_store_ptr_t& get_tokens() const;

    /**
     * Get the cached result as a numeric value.
     *
     * @param policy Action to take in case the result is not yet available.
     *
     * @return Numeric value of the result.
     *
     * @throw formula_error If the result is not convertible to a numeric
     *                      value.
     *
     * @note When the result is a matrix, the value of one element is returned:
     *       the element at the cell's position within its group for a grouped
     *       formula cell, or the top-left element for a non-grouped formula
     *       cell.
     */
    double get_value(formula_result_wait_policy_t policy) const;

    /**
     * Get the cached result as a string value.
     *
     * @param policy Action to take in case the result is not yet available.
     *
     * @return String value of the result.
     *
     * @throw formula_error If the result is not a string value.
     *
     * @note When the result is a matrix, the value of one element is returned:
     *       the element at the cell's position within its group for a grouped
     *       formula cell, or the top-left element for a non-grouped formula
     *       cell.
     */
    std::string_view get_string(formula_result_wait_policy_t policy) const;

    /**
     * Interpret the formula tokens and store the result in the cell.  For a
     * member of a formula group other than its top-left cell, this does
     * nothing, as the top-left cell calculates the result of the whole
     * group.
     *
     * @param context Model context the cell belongs to.
     * @param pos Position of the cell.
     */
    void interpret(model_context& context, const abs_address_t& pos);

    /**
     * Determine if this cell contains circular reference by walking through
     * all its reference tokens.
     */
    void check_circular(const model_context& cxt, const abs_address_t& pos);

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
        const model_context& cxt, const abs_address_t& pos) const;

    /**
     * Get the cached result without post-processing in case of a grouped
     * formula cell.
     *
     * @param policy action to take in case the result is not yet available.
     *
     * @return formula result.
     */
    const formula_result& get_raw_result_cache(formula_result_wait_policy_t policy) const;

    /**
     * Get the cached result as a single cell.  For a non-grouped formula
     * cell, it should be identical to the value from the get_raw_result_cache()
     * call.  For a grouped formula cell, you'll get a single value assigned to
     * the position of the cell in case the original result is a matrix value.
     *
     * @param policy action to take in case the result is not yet available.
     *
     * @return formula result.
     *
     * @note A non-grouped formula cell whose result is a matrix gets the
     *       matrix back as is.
     */
    formula_result get_result_cache(formula_result_wait_policy_t policy) const;

    /**
     * Set a cached result to this formula cell instance.
     *
     *
     * @param result cached result.
     */
    void set_result_cache(formula_result result);

    /**
     * Get the group properties of this cell: whether it is grouped, and if
     * so, the size and the identity of its group.
     *
     * @return Group properties of the cell.
     */
    formula_group_t get_group_properties() const;

    /**
     * Get the absolute parent position of a grouped formula cell.  If the
     * cell is not grouped, it simply returns the original position passed to
     * this method.
     *
     * @param pos original position from which to calculate the parent
     *            position.
     *
     * @return parent position of the grouped formula cell.
     */
    abs_address_t get_parent_position(const abs_address_t& pos) const;
};

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
