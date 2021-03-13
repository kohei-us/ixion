/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "vulkan_obj.hpp"
#include "debug.hpp"

#include <vector>
#include <cstring>
#include <iostream>
#include <sstream>

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

} // anonymous namespace

vk_instance::vk_instance()
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

vk_instance::~vk_instance()
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

VkInstance vk_instance::get() { return m_instance; }

vk_device::vk_device(vk_instance& instance)
{
    uint32_t n_devices = 0;
    VkResult res = vkEnumeratePhysicalDevices(instance.get(), &n_devices, nullptr);
    if (res != VK_SUCCESS)
        throw std::runtime_error("failed to query the number of physical devices.");

    if (!n_devices)
        throw std::runtime_error("no vulkan devices found!");

    std::vector<VkPhysicalDevice> devices(n_devices);
    res = vkEnumeratePhysicalDevices(instance.get(), &n_devices, devices.data());
    if (res != VK_SUCCESS)
        throw std::runtime_error("failed to obtain the physical device data.");

#ifdef IXION_TRACE_ON
    for (const VkPhysicalDevice pd : devices)
    {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(pd, &props);

        IXION_TRACE("--");
        IXION_TRACE("name: " << props.deviceName);
        IXION_TRACE("vendor ID: " << props.vendorID);
        IXION_TRACE("device ID: " << props.deviceID);
    }
#endif

    VkPhysicalDevice pd = devices[0]; // pick the first physical device for now.

    {
        uint32_t n_qfs;
        vkGetPhysicalDeviceQueueFamilyProperties(pd, &n_qfs, nullptr);

        std::vector<VkQueueFamilyProperties> qf_props(n_qfs);
        vkGetPhysicalDeviceQueueFamilyProperties(pd, &n_qfs, qf_props.data());

        IXION_TRACE("scanning for a queue family that supports compute...");

        uint8_t current_n_flags = std::numeric_limits<uint8_t>::max();

        for (size_t i = 0; i < qf_props.size(); ++i)
        {
            uint8_t n_flags = 0;
            bool supports_compute = false;

            const VkQueueFamilyProperties& props = qf_props[i];

            std::ostringstream os;
            os << "- queue family " << i << ": ";

            if (props.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                os << "graphics ";
                ++n_flags;
            }

            if (props.queueFlags & VK_QUEUE_COMPUTE_BIT)
            {
                os << "compute ";
                ++n_flags;
                supports_compute = true;
            }

            if (props.queueFlags & VK_QUEUE_TRANSFER_BIT)
            {
                os << "transfer ";
                ++n_flags;
            }

            if (props.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
            {
                os << "sparse-binding ";
                ++n_flags;
            }

            if (props.queueFlags & VK_QUEUE_PROTECTED_BIT)
            {
                os << "protected ";
                ++n_flags;
            }

            if (supports_compute && n_flags < current_n_flags)
            {
                current_n_flags = n_flags;
                m_queue_family_index = i;
                os << "(picked)";
            }

            IXION_TRACE(os.str());
        }

        IXION_TRACE("final queue family index: " << m_queue_family_index);

        VkDeviceQueueCreateInfo queue_ci = {};

        const float queue_prio = 0.0f;
        queue_ci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_ci.queueFamilyIndex = m_queue_family_index;
        queue_ci.queueCount = 1;
        queue_ci.pQueuePriorities = &queue_prio;

        VkDeviceCreateInfo device_ci = {};
        device_ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        device_ci.queueCreateInfoCount = 1;
        device_ci.pQueueCreateInfos = &queue_ci;
		res = vkCreateDevice(pd, &device_ci, nullptr, &m_device);
        if (res != VK_SUCCESS)
            throw std::runtime_error("failed to create a logical device.");

        vkGetDeviceQueue(m_device, m_queue_family_index, 0, &m_queue);
    }
}

vk_device::~vk_device()
{
    vkDestroyDevice(m_device, nullptr);
}

VkDevice vk_device::get()
{
    return m_device;
}

vk_command_pool::vk_command_pool(vk_device& device) :
    m_device(device.get())
{
    VkCommandPoolCreateInfo ci = {};
    ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    ci.queueFamilyIndex = device.get_queue_family_index();
    ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VkResult res = vkCreateCommandPool(device.get(), &ci, nullptr, &m_cmd_pool);
    if (res != VK_SUCCESS)
        throw std::runtime_error("failed to create command pool.");
}

vk_command_pool::~vk_command_pool()
{
    vkDestroyCommandPool(m_device, m_cmd_pool, nullptr);
}

}}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
