/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_CELL_HPP
#define INCLUDED_IXION_CELL_HPP

#include "ixion/formula_tokens.hpp"
#include "ixion/address.hpp"
#include "ixion/types.hpp"

#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>

#include <memory>

namespace ixion {

class formula_result;
class formula_cell;

namespace iface {

class formula_model_access;

}

class formula_cell
{
    struct impl;

    std::unique_ptr<impl> mp_impl;

    void reset_flag();

    /**
     * Block until the result becomes available.
     *
     * @param lock mutex lock associated with the result cache data.
     */
    void wait_for_interpreted_result(::boost::mutex::scoped_lock& lock) const;

    /**
     * Check if this cell contains a circular reference.
     *
     * @return true if this cell contains no circular reference, hence
     *         considered "safe", false otherwise.
     */
    bool is_circular_safe() const;

    bool check_ref_for_circular_safety(const formula_cell& ref, const abs_address_t& pos);

    double fetch_value_from_result() const;

    formula_cell(const formula_cell&) = delete;
public:
    IXION_DLLPUBLIC formula_cell();
    IXION_DLLPUBLIC formula_cell(size_t tokens_identifier);
    IXION_DLLPUBLIC ~formula_cell();

    IXION_DLLPUBLIC size_t get_identifier() const;
    void set_identifier(size_t identifier);

    IXION_DLLPUBLIC double get_value() const;
    IXION_DLLPUBLIC double get_value_nowait() const;
    IXION_DLLPUBLIC void interpret(iface::formula_model_access& context, const abs_address_t& pos);

    /**
     * Determine if this cell contains circular reference by walking through
     * all its reference tokens.
     */
    void check_circular(const iface::formula_model_access& cxt, const abs_address_t& pos);

    /**
     * Reset cell's internal state.
     */
    IXION_DLLPUBLIC void reset();

    IXION_DLLPUBLIC void get_ref_tokens(
        const iface::formula_model_access& cxt, const abs_address_t& pos, std::vector<const formula_token_base*>& tokens);

    IXION_DLLPUBLIC const formula_result* get_result_cache() const;

    IXION_DLLPUBLIC bool is_shared() const;
    IXION_DLLPUBLIC void set_shared(bool b);
};

}

#endif
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
