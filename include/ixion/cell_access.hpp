/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_CELL_ACCESS_HPP
#define INCLUDED_CELL_ACCESS_HPP

#include "ixion/types.hpp"

#include <memory>

namespace ixion {

class model_context;
struct abs_address_t;

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

    celltype_t get_type() const;
};

}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
