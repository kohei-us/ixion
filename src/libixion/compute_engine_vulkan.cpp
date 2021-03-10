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

#include "debug.hpp"

#include <iostream>
#include <vector>
#include <cstring>

namespace ixion { namespace draft {

namespace {

VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* cb_data,
    void* user)
{
    IXION_DEBUG(cb_data->pMessage);
    return VK_FALSE;
}

}

compute_engine_vulkan::compute_engine_vulkan() : compute_engine()
{
    const char* validation_layer = "VK_LAYER_KHRONOS_validation";
    const char* validation_ext = "VK_EXT_debug_utils";

    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "ixion-compute-engine-vulkan";
    app_info.pEngineName = "none";
    app_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instance_ci = {};
    instance_ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_ci.pApplicationInfo = &app_info;

    uint32_t n_layers;
    vkEnumerateInstanceLayerProperties(&n_layers, nullptr);
    std::vector<VkLayerProperties> layers(n_layers);
    vkEnumerateInstanceLayerProperties(&n_layers, layers.data());

    IXION_TRACE("scanning for validation layer...");
    bool has_validation_layer = false;
    for (const VkLayerProperties& props : layers)
    {
        if (!strcmp(props.layerName, validation_layer))
        {
            IXION_TRACE("- " << props.layerName << " (found)");
            has_validation_layer = true;
            break;
        }

        IXION_TRACE("- " << props.layerName);
    }

    if (has_validation_layer)
    {
        instance_ci.ppEnabledLayerNames = &validation_layer;
        instance_ci.enabledLayerCount = 1;
        instance_ci.enabledExtensionCount = 1;
        instance_ci.ppEnabledExtensionNames = &validation_ext;
    }

    VkResult res = vkCreateInstance(&instance_ci, nullptr, &m_instance);
    if (res != VK_SUCCESS)
        throw std::runtime_error("failed to create a vulkan instance.");

    if (has_validation_layer)
    {
        VkDebugUtilsMessengerCreateInfoEXT debug_ci{};
        debug_ci.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debug_ci.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debug_ci.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debug_ci.pfnUserCallback = vulkan_debug_callback;
        debug_ci.pUserData = nullptr;

        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            m_instance, "vkCreateDebugUtilsMessengerEXT");

        if (func)
        {
            res = func(m_instance, &debug_ci, nullptr, &m_debug_messenger);
            if (res != VK_SUCCESS)
                throw std::runtime_error("failed to create debug utils messenger.");
        }
    }
}

compute_engine_vulkan::~compute_engine_vulkan()
{
    if (m_debug_messenger)
    {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            m_instance, "vkDestroyDebugUtilsMessengerEXT");

        if (func)
            func(m_instance, m_debug_messenger, nullptr);
    }

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
