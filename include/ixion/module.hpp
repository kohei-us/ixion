/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_MODULE_HPP
#define INCLUDED_IXION_MODULE_HPP

#include "ixion/env.hpp"

namespace ixion {

/**
 * Initialize modules if exists.
 */
IXION_DLLPUBLIC void init_modules();

class compute_engine;

using create_compute_engine_t = compute_engine* (*)();
using destroy_compute_engine_t = void (*)(const compute_engine*);

struct module_def
{
    create_compute_engine_t create_compute_engine;
    destroy_compute_engine_t destroy_compute_engine;
};

}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
