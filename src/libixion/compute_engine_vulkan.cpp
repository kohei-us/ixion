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

compute_engine_vulkan::compute_engine_vulkan() :
    compute_engine(),
    m_instance(),
    m_device(m_instance),
    m_cmd_pool(m_device)
{
}

compute_engine_vulkan::~compute_engine_vulkan()
{
}

const char* compute_engine_vulkan::get_name() const
{
    return "vulkan";
}

void compute_engine_vulkan::compute_fibonacci(array& io)
{
    vk_buffer host_buffer(
        m_device, io.size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    host_buffer.write_to_memory(io.data, sizeof(uint32_t)*io.size);

    vk_buffer device_buffer(
        m_device, io.size,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // TODO : transfer the data from host to device.
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
