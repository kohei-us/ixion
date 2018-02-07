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

namespace ixion { namespace draft {

compute_engine_cuda::compute_engine_cuda() : compute_engine()
{
    std::cout << __FILE__ << ":" << __LINE__ << " (compute_engine_cuda:compute_engine_cuda): ctor" << std::endl;
}

compute_engine_cuda::~compute_engine_cuda()
{
    std::cout << __FILE__ << ":" << __LINE__ << " (compute_engine_cuda:~compute_engine_cuda): dtor" << std::endl;
}

const char* compute_engine_cuda::get_name() const
{
    return "cuda";
}

compute_engine* create()
{
    return new compute_engine_cuda();
}

void destroy(const compute_engine* p)
{
    delete static_cast<const compute_engine_cuda*>(p);
}

}}

extern "C" {

IXION_DLLPUBLIC ixion::draft::module_def* register_module()
{
    static ixion::draft::module_def md =
    {
        ixion::draft::create,
        ixion::draft::destroy
    };

    return &md;
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
