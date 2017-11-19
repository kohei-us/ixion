/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/module.hpp"
#include <iostream>
#include <dlfcn.h>

namespace ixion {

void init_modules()
{
    std::cout << __FILE__ << ":" << __LINE__ << " (ixion:init_modules): " << std::endl;
    const char* module_path = std::getenv("IXION_MODULE_PATH");
    if (!module_path)
        return;

    std::cout << __FILE__ << ":" << __LINE__ << " (ixion:init_modules): module path = " << module_path << std::endl;
    std::string cuda_path = module_path;
    cuda_path += "/ixion-0.13-cuda.so";

    std::cout << __FILE__ << ":" << __LINE__ << " (ixion:init_modules): cuda path = " << cuda_path << std::endl;
    void* hdl = dlopen(cuda_path.data(), RTLD_NOW | RTLD_GLOBAL);
    if (!hdl)
        return;

    std::cout << __FILE__ << ":" << __LINE__ << " (ixion:init_modules): loaded " << hdl << std::endl;

    typedef module_def* (*foo)(void);
    foo my_func;
    *(void**)(&my_func) = dlsym(hdl, "register_module");

    if (my_func)
    {
        module_def* md = my_func();
        std::cout << __FILE__ << ":" << __LINE__ << " (ixion:init_modules): md = " << md << std::endl;
    }

    dlclose(hdl);
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
