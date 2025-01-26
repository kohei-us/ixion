/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "utils.hpp"
#include <ixion/exceptions.hpp>
#include <ixion/formula_result.hpp>

#include <sstream>

namespace ixion { namespace detail {

cell_t to_celltype(mdds::mtv::element_t mtv_type)
{
    switch (mtv_type)
    {
        case element_type_empty:
            return cell_t::empty;
        case element_type_numeric:
            return cell_t::numeric;
        case element_type_boolean:
            return cell_t::boolean;
        case element_type_string:
            return cell_t::string;
        case element_type_formula:
            return cell_t::formula;
        default:
            ;
    }

    std::ostringstream os;
    os << "unknown cell type (" << mtv_type << ")";
    throw general_error(os.str());
}

cell_value_t to_cell_value_type(
    const column_store_t::const_position_type& pos, formula_result_wait_policy_t policy)
{
    cell_t raw_type = to_celltype(pos.first->type);

    if (raw_type != cell_t::formula)
        // cell_t and cell_value_t are numerically equivalent except for the formula slot.
        return static_cast<cell_value_t>(raw_type);

    const formula_cell* fc = formula_element_block::at(*pos.first->data, pos.second);
    formula_result res = fc->get_result_cache(policy); // by calling this we should not get a matrix result.

    switch (res.get_type())
    {
        case formula_result::result_type::boolean:
            return cell_value_t::boolean;
        case formula_result::result_type::value:
            return cell_value_t::numeric;
        case formula_result::result_type::string:
            return cell_value_t::string;
        case formula_result::result_type::error:
            return cell_value_t::error;
        case formula_result::result_type::matrix:
            throw std::logic_error("we shouldn't be getting a matrix result type here.");
    }

    return cell_value_t::unknown;
}

}}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
