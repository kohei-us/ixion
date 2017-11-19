/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/compute_engine.hpp"
#include "ixion/global.hpp"

#include <iostream>

namespace ixion {

struct compute_engine::impl
{
    impl() {}
};

std::unique_ptr<compute_engine> compute_engine::create(const char* name)
{
    return ixion::make_unique<compute_engine>();
}

compute_engine::compute_engine() :
    mp_impl(ixion::make_unique<impl>())
{
}

compute_engine::~compute_engine()
{
}

void compute_engine::test()
{
    std::cout << __FILE__ << ":" << __LINE__ << " (compute_engine:test): base" << std::endl;
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
