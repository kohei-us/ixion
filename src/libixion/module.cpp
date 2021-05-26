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
#include <boost/filesystem.hpp>
#ifndef _WIN32
#include <dlfcn.h>
#endif

namespace fs = boost::filesystem;

namespace ixion { namespace draft {

#ifdef _WIN32

void init_modules()
{
}

void unload_module(void* /*handler*/)
{
}

#else

void init_modules()
{
    static std::vector<const char*> mod_names = {
        "vulkan",
    };

    std::string mod_prefix;
    {
        std::ostringstream os;
        os << "ixion-" << get_api_version_major() << "." << get_api_version_minor() << "-";
        mod_prefix = os.str();
    }

    for (const char* mod_name : mod_names)
    {
        std::ostringstream os;
        os << mod_prefix << mod_name << ".so";

        // TODO: make this cross-platform.
        void* hdl = dlopen(os.str().data(), RTLD_NOW | RTLD_GLOBAL);
        if (!hdl)
            return;

        typedef module_def* (*register_module_type)(void);
        register_module_type register_module;
        *(void**)(&register_module) = dlsym(hdl, "register_module");

        if (register_module)
        {
            module_def* md = register_module();
            compute_engine::add_class(
                hdl, mod_name, md->create_compute_engine, md->destroy_compute_engine);
        }
    }
}

void unload_module(void* handler)
{
    dlclose(handler);
}

#endif

}}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
