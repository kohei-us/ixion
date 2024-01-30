/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <ixion/formula_opcode.hpp>
#include <ixion/formula_tokens.hpp>

#include <ostream>

namespace ixion {

std::ostream& operator<<(std::ostream& os, fopcode_t oc)
{
    os << "(name='" << get_formula_opcode_name(oc) << "'; op='"
        << get_formula_opcode_string(oc) << "'; v=" << int(oc) << ')';
    return os;
}

} // namespace ixion

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
