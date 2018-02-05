/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_COMPUTE_ENGINE_CUDA_HPP
#define INCLUDED_IXION_COMPUTE_ENGINE_CUDA_HPP

#include "ixion/compute_engine.hpp"

namespace ixion {

class compute_engine_cuda : public compute_engine
{
public:
    compute_engine_cuda();
    virtual ~compute_engine_cuda();

    virtual const char* get_name() const override;
};

}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
