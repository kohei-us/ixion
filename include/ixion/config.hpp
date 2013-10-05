/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef __IXION_CONFIG_HPP__
#define __IXION_CONFIG_HPP__

#include "ixion/env.hpp"

namespace ixion {

/**
 * This structure store parameters that influence various aspects of the
 * ixion formula engine.
 */
struct IXION_DLLPUBLIC config
{
    /**
     * Function argument separator.  By default it's ','.
     */
    char sep_function_arg;

    config();
    config(const config& r);
};

}

#endif
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
