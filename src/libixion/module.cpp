/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ixion/module.hpp"
#include "ixion/info.hpp"
#include "ixion/compute_engine.hpp"
#include <sstream>
#include <vector>
#include <dlfcn.h>

namespace ixion {

void init_modules()
{
    const char* module_path = std::getenv("IXION_MODULE_PATH");
    if (!module_path)
        return;

    static std::vector<const char*> mod_names = {
        "cuda",
    };

    std::ostringstream os;
    os << "ixion-" << get_api_version_major() << "." << get_api_version_minor() << "-";
    std::string mod_prefix = os.str();

    for (const char* mod_name : mod_names)
    {
        // TODO: use boost.filesystem.
        std::ostringstream os;
        os << module_path << "/" << mod_prefix << mod_name << ".so";
        std::string cuda_path = os.str();

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

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
