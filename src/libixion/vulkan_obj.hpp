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
#include <limits>

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
    static constexpr uint32_t QUEUE_FAMILY_NOT_SET = std::numeric_limits<uint32_t>::max();

    VkDevice m_device = nullptr;
    uint32_t m_queue_family_index = QUEUE_FAMILY_NOT_SET;
    VkQueue m_queue = nullptr;

public:
    vk_device(vk_instance& instance);
    ~vk_device();

    VkDevice get();
};

}}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
