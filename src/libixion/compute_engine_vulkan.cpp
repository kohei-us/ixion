/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "compute_engine_vulkan.hpp"
#include "ixion/module.hpp"
#include "ixion/env.hpp"
#include "ixion/exceptions.hpp"

#include <iostream>
#include <vector>

namespace ixion { namespace draft {

compute_engine_vulkan::compute_engine_vulkan() : compute_engine()
{
    std::vector<const char*> validation_layers = {
        "VK_LAYER_LUNARG_standard_validation",
    };

    const char* validation_ext = "VK_EXT_debug_report";

    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "ixion-compute-engine-vulkan";
    app_info.pEngineName = "none";
    app_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instance_ci = {};
    instance_ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_ci.pApplicationInfo = &app_info;

    instance_ci.ppEnabledLayerNames = validation_layers.data();
    instance_ci.enabledLayerCount = validation_layers.size();
    instance_ci.enabledExtensionCount = 1;
    instance_ci.ppEnabledExtensionNames = &validation_ext;

    VkResult res = vkCreateInstance(&instance_ci, nullptr, &m_instance);
    if (res != VK_SUCCESS)
        throw general_error("failed to create a vulkan instance.");
}

compute_engine_vulkan::~compute_engine_vulkan()
{
    vkDestroyInstance(m_instance, nullptr);
}

const char* compute_engine_vulkan::get_name() const
{
    return "vulkan";
}

compute_engine* create()
{
    return new compute_engine_vulkan();
}

void destroy(const compute_engine* p)
{
    delete static_cast<const compute_engine_vulkan*>(p);
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
