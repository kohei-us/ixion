/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "compute_engine_cuda.hpp"
#include <iostream>

namespace ixion {

compute_engine_cuda::compute_engine_cuda() :
    compute_engine()
{
}

compute_engine_cuda::~compute_engine_cuda()
{
}

void compute_engine_cuda::test()
{
    std::cout << __FILE__ << ":" << __LINE__ << " (compute_engine_cuda:test): cuda" << std::endl;
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
