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

struct module_def
{
    void* (*create_compute_engine)();
    void (*destroy_compute_engine)(const void*);
};

}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
