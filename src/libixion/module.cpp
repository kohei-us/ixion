/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/module.hpp"
#include "ixion/compute_engine.hpp"
#include <iostream>
#include <dlfcn.h>

namespace ixion {

void init_modules()
{
    const char* module_path = std::getenv("IXION_MODULE_PATH");
    if (!module_path)
        return;

    // TODO: use boost.filesystem.
    std::string cuda_path = module_path;
    cuda_path += "/ixion-0.13-cuda.so";

    // TODO: make this cross-platform.
    void* hdl = dlopen(cuda_path.data(), RTLD_NOW | RTLD_GLOBAL);
    if (!hdl)
        return;

    typedef module_def* (*register_module_type)(void);
    register_module_type register_module;
    *(void**)(&register_module) = dlsym(hdl, "register_module");

    if (register_module)
    {
        module_def* md = register_module();
        compute_engine::add_class("cuda", md->create_compute_engine, md->destroy_compute_engine);
    }
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
