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

std::size_t get_byte_size(const array& io)
{
    switch (io.type)
    {
        case array_type::uint32:
            return sizeof(uint32_t) * io.size;
        case array_type::float32:
            return sizeof(float) * io.size;
        case array_type::float64:
            return sizeof(double) * io.size;
        case array_type::unknown:
            IXION_DEBUG("array type is unknown.");
    }

    return 0;
}

}

/**
 * This function first writes the source data array to host buffer, then
 * creates a device local buffer, and create and execute a command to copy
 * the data in the host buffer to the device local buffer.
 */
void compute_engine_vulkan::copy_to_device_local_buffer(
    array& io, vk_buffer& host_buffer, vk_buffer& device_buffer)
{
    vk_command_buffer cmd_copy = m_cmd_pool.create_command_buffer();
    cmd_copy.begin();
    cmd_copy.copy_buffer(host_buffer, device_buffer, sizeof(uint32_t)*io.size);
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

std::string_view compute_engine_vulkan::get_name() const
{
    return "vulkan";
}

void compute_engine_vulkan::compute_fibonacci(array& io)
{
    runtime_context cxt;
    cxt.input_buffer_size = io.size;

    std::size_t data_byte_size = get_byte_size(io);

    vk_buffer host_buffer(
        m_device, data_byte_size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    host_buffer.write_to_memory(io.data, data_byte_size);

    vk_buffer device_buffer(
        m_device, data_byte_size,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    copy_to_device_local_buffer(io, host_buffer, device_buffer);

    // Create a descriptor pool, by specifying the number of descriptors for
    // each type that can be allocated in a single set, as well as the max
    // number of sets. Here, we are specifying that we will only allocate one
    // set, and each set will contain only one storage buffer.
    vk_descriptor_pool desc_pool(m_device, 1u,
        {
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u },
        }
    );

    // Create a descriptor set layout. This specifies what descriptor type is
    // bound to what binding location and how many units (in case the
    // descriptor is an array), and which stages can access it.  Here, we are
    // binding one storage buffer to the binding location of 0, for the
    // compute stage.
    vk_descriptor_set_layout ds_layout(m_device,
        {
            // binding id, descriptor type, descriptor count, stage flags, sampler (optional)
            { 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u, VK_SHADER_STAGE_COMPUTE_BIT, nullptr },
        }
    );

    // Create a pipeline layout.  A pipeline layout consists of one or more
    // descriptor set layouts as well as one or more push constants.  It
    // describes the entire resources the pipeline will have access to.  Here
    // we only have one descriptor set layout and no push constants.
    vk_pipeline_layout pl_layout(m_device, ds_layout);

    // Allocate a descriptor set from the descriptor set layout.  This will
    // get bound to command buffer later as part of the recorded commands.
    vk_descriptor_set desc_set = desc_pool.allocate(ds_layout);

    // Update the descriptor set with the content of the device-local buffer.
    // You always have to create a descriptor set first, then update it
    // afterward.  Here, we are binding the actual device buffer instance to
    // the binding location of 0, just like we specified above.
    desc_set.update(m_device, 0u, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, device_buffer);

    // Create pipeline cache.  You need to create this so that you can
    // optinally save the cached pipeline state to disk for re-use, which we
    // don't do here.
    vk_pipeline_cache pl_cache(m_device);

    // Load shader module for fibonnaci.
    vk_shader_module fibonnaci_module(m_device, vk_shader_module::module_type::fibonacci);

    // Create a compute pipeline.
    vk_pipeline pipeline(cxt, m_device, pl_layout, pl_cache, fibonnaci_module);

    // allocate command buffer.
    vk_command_buffer cmd = m_cmd_pool.create_command_buffer();

    // Record command buffer.
    cmd.begin();

    // Ensure that the write to the device buffer gets finished before the
    // compute shader stage.
    cmd.buffer_memory_barrier(
        device_buffer,
        VK_ACCESS_HOST_WRITE_BIT,
        VK_ACCESS_SHADER_READ_BIT,
        VK_PIPELINE_STAGE_HOST_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
    );

    // Bind the pipeline and descriptor set to the compute pipeline.
    cmd.bind_pipeline(pipeline, VK_PIPELINE_BIND_POINT_COMPUTE);
    cmd.bind_descriptor_set(VK_PIPELINE_BIND_POINT_COMPUTE, pl_layout, desc_set);

    // Trigger compute work for data size of (n, 1, 1).
    cmd.dispatch(cxt.input_buffer_size, 1, 1);

    // Ensure that the compute stages finishes before buffer gets copied.
    cmd.buffer_memory_barrier(
        device_buffer,
        VK_ACCESS_SHADER_WRITE_BIT,
        VK_ACCESS_TRANSFER_READ_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT
    );

    // Copy the data from the device local buffer to the host buffer.
    cmd.copy_buffer(device_buffer, host_buffer, data_byte_size);

    // Ensure that the buffer copying gets done before data is read on the
    // host cpu side.
    cmd.buffer_memory_barrier(
        host_buffer,
        VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_ACCESS_HOST_READ_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_HOST_BIT
    );

    cmd.end();

    // Submit the command and wait.
    vk_fence fence(m_device, VK_FENCE_CREATE_SIGNALED_BIT);
    fence.reset();

    vk_queue q = m_device.get_queue();
    q.submit(cmd, fence, VK_PIPELINE_STAGE_TRANSFER_BIT);
    fence.wait();

    // Read the values from the host memory.
    host_buffer.read_from_memory(io.data, data_byte_size);
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

IXION_MOD_EXPORT ixion::draft::module_def* register_module()
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
