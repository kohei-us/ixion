/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_CELL_QUEUE_MANAGER_HPP
#define INCLUDED_IXION_CELL_QUEUE_MANAGER_HPP

#include "ixion/global.hpp"

#include <memory>
#include <vector>

namespace ixion {

struct abs_address_t;

namespace iface {

class formula_model_access;

}

/**
 * Class that manages multi-threaded calculation of formula cells.
 */
class formula_cell_queue
{
    struct impl;
    std::unique_ptr<impl> mp_impl;

public:
    formula_cell_queue() = delete;

    formula_cell_queue(
        iface::formula_model_access& cxt,
        std::vector<abs_address_t>&& cells,
        size_t thread_count);

    ~formula_cell_queue();

    void run();
};

}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
