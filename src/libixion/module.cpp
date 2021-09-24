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

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace fs = boost::filesystem;

namespace ixion { namespace draft {

namespace {

const char* mod_names[] = { "vulkan" };

std::string get_module_prefix()
{
    std::ostringstream os;
    os << "ixion-" << get_api_version_major() << "." << get_api_version_minor() << "-";
    return os.str();
}

typedef module_def* (*fp_register_module_type)(void);

void register_module(void* mod_handler, std::string_view mod_name, fp_register_module_type fp_register_module)
{
    if (!fp_register_module)
        return;

    module_def* md = fp_register_module();
    compute_engine::add_class(
        mod_handler, mod_name, md->create_compute_engine, md->destroy_compute_engine);
}

} // anonymous namespace

#ifdef _WIN32

void init_modules()
{
    std::string mod_prefix = get_module_prefix();

    for (const char* mod_name : mod_names)
    {
        std::ostringstream os;
        os << mod_prefix << mod_name << ".dll";

        std::string modfile_name = os.str();

        HMODULE hdl = LoadLibrary(modfile_name.c_str());
        if (!hdl)
            continue;

        fp_register_module_type fp_register_module;
        *(void**)(&fp_register_module) = GetProcAddress(hdl, "register_module");

        register_module(hdl, mod_name, fp_register_module);
    }
}

void unload_module(void* handler)
{
    FreeLibrary((HMODULE)handler);
}

#else

void init_modules()
{
    std::string mod_prefix = get_module_prefix();

    for (const char* mod_name : mod_names)
    {
        std::ostringstream os;
        os << mod_prefix << mod_name << ".so";

        void* hdl = dlopen(os.str().data(), RTLD_NOW | RTLD_GLOBAL);
        if (!hdl)
            continue;

        fp_register_module_type fp_register_module;
        *(void**)(&fp_register_module) = dlsym(hdl, "register_module");

        register_module(hdl, mod_name, fp_register_module);
    }
}

void unload_module(void* handler)
{
    dlclose(handler);
}

#endif

}}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
