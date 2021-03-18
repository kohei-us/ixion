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

void compute_engine_vulkan::copy_to_device_local_buffer(array& io)
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

    vk_command_buffer cmd_copy = m_cmd_pool.create_command_buffer();
    cmd_copy.begin();
    cmd_copy.copy_buffer(host_buffer, device_buffer, io.size);
    cmd_copy.end();

    vk_fence fence(m_device, 0);
    vk_queue q = m_device.get_queue();
    q.submit(cmd_copy, fence);
    fence.wait();
}

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
    copy_to_device_local_buffer(io);

    // Create a descriptor pool, by specifying the number of descriptors for
    // each type that can be allocated in a single set, as well as the max
    // number of sets.
    vk_descriptor_pool desc_pool(m_device, 1u,
        {
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u },
        }
    );

    //  2. create descriptor set layout.
    //  3. create pipeline layout.
    //  4. allocate descriptor sets.
    //  5. update descriptor sets.
    //  6. create pipeline cache.
    //  7. load shader module.
    //  8. create compute pipeline.
    //  9. allocate command buffer.
    // 10. create fence.
    //
    // Once here, record the command buffer and continue on...
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
