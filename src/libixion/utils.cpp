/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "utils.hpp"
#include "ixion/exceptions.hpp"

#include <sstream>

namespace ixion { namespace detail {

celltype_t to_celltype(mdds::mtv::element_t mtv_type)
{
    switch (mtv_type)
    {
        case element_type_empty:
            return celltype_t::empty;
        case element_type_numeric:
            return celltype_t::numeric;
        case element_type_boolean:
            return celltype_t::boolean;
        case element_type_string:
            return celltype_t::string;
        case element_type_formula:
            return celltype_t::formula;
        default:
            ;
    }

    std::ostringstream os;
    os << "unknown cell type (" << mtv_type << ")";
    throw general_error(os.str());
}

}}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
