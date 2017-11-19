/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_COMPUTE_ENGINE_HPP
#define INCLUDED_IXION_COMPUTE_ENGINE_HPP

#include "ixion/env.hpp"
#include <memory>

namespace ixion {

class IXION_DLLPUBLIC compute_engine
{
    struct impl;
    std::unique_ptr<impl> mp_impl;

public:
    static std::unique_ptr<compute_engine> create(const char* name = nullptr);

    compute_engine();
    virtual ~compute_engine();

    virtual void test();
};

}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
