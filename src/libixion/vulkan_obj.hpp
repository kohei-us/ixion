/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_IXION_VULKAN_OBJ_HPP
#define INCLUDED_IXION_VULKAN_OBJ_HPP

#include <vulkan/vulkan.h>
#include <memory>

namespace ixion { namespace draft {

class vk_instance
{
    VkInstance m_instance = nullptr;
    VkDebugUtilsMessengerEXT m_debug_messenger = nullptr;

public:
    vk_instance();
    ~vk_instance();

    VkInstance get();
};

class vk_device
{
    VkDevice m_device = nullptr;

public:
    vk_device(vk_instance& instance);
    ~vk_device();

    VkDevice get();
};

}}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
