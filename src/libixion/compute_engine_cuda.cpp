/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "compute_engine_cuda.hpp"
#include "ixion/module.hpp"
#include "ixion/env.hpp"
#include <iostream>

namespace ixion {

compute_engine_cuda::compute_engine_cuda() :
    compute_engine()
{
    std::cout << __FILE__ << ":" << __LINE__ << " (compute_engine_cuda:compute_engine_cuda): ctor" << std::endl;
}

compute_engine_cuda::~compute_engine_cuda()
{
    std::cout << __FILE__ << ":" << __LINE__ << " (compute_engine_cuda:~compute_engine_cuda): dtor" << std::endl;
}

void compute_engine_cuda::test()
{
    std::cout << __FILE__ << ":" << __LINE__ << " (compute_engine_cuda:test): cuda" << std::endl;
}

compute_engine* create()
{
    return new compute_engine_cuda();
}

void destroy(const compute_engine* p)
{
    delete static_cast<const compute_engine_cuda*>(p);
}

}

extern "C" {

IXION_DLLPUBLIC ixion::module_def* register_module()
{
    static ixion::module_def md =
    {
        ixion::create,
        ixion::destroy
    };

    return &md;
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
