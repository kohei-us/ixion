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

template<typename T, typename U = void>
struct null_value;

template<typename T>
struct null_value<T, typename std::enable_if<std::is_pointer<T>::value>::type>
{
    static constexpr std::nullptr_t value = nullptr;
};

template<typename T>
struct null_value<T, typename std::enable_if<std::is_integral<T>::value>::type>
{
    static constexpr T value = 0;
};

class vk_buffer;
class vk_command_buffer;
class vk_command_pool;
class vk_descriptor_set;
class vk_descriptor_set_layout;
class vk_fence;

class vk_instance
{
    VkInstance m_instance = null_value<VkInstance>::value;
    VkDebugUtilsMessengerEXT m_debug_messenger = null_value<VkDebugUtilsMessengerEXT>::value;

public:
    vk_instance();
    ~vk_instance();

    VkInstance& get();
};

class vk_queue
{
    VkQueue m_queue;

public:
    vk_queue(VkQueue queue);
    ~vk_queue();

    void submit(vk_command_buffer& cmd, vk_fence& fence);
};

class vk_device
{
    friend class vk_command_pool;
    friend class vk_buffer;

    static constexpr uint32_t QUEUE_FAMILY_NOT_SET = std::numeric_limits<uint32_t>::max();

    VkPhysicalDevice m_physical_device = null_value<VkPhysicalDevice>::value;
    VkDevice m_device = null_value<VkDevice>::value;
    uint32_t m_queue_family_index = QUEUE_FAMILY_NOT_SET;
    VkQueue m_queue = null_value<VkQueue>::value;

    uint32_t get_queue_family_index() const
    {
        return m_queue_family_index;
    }

public:
    vk_device(vk_instance& instance);
    ~vk_device();

    VkDevice& get();
    const VkDevice& get() const;

    VkPhysicalDevice get_physical_device();

    vk_queue get_queue();
};

class vk_command_pool
{
    friend class vk_command_buffer;

    VkDevice m_device = null_value<VkDevice>::value;
    VkCommandPool m_cmd_pool = null_value<VkCommandPool>::value;

    VkDevice& get_device();
    VkCommandPool& get();

public:
    vk_command_pool(vk_device& device);
    ~vk_command_pool();

    vk_command_buffer create_command_buffer();
};

class vk_command_buffer
{
    friend class vk_command_pool;

    vk_command_pool& m_cmd_pool;
    VkCommandBuffer m_cmd_buffer = null_value<VkCommandBuffer>::value;

    vk_command_buffer(vk_command_pool& cmd_pool);

public:
    ~vk_command_buffer();

    VkCommandBuffer& get();

    void begin();
    void end();

    void copy_buffer(vk_buffer& src, vk_buffer& dst, VkDeviceSize size);
};

class vk_buffer
{
    vk_device& m_device;
    VkBuffer m_buffer = null_value<VkBuffer>::value;
    VkDeviceMemory m_memory = null_value<VkDeviceMemory>::value;

    struct mem_type
    {
        uint32_t index;
        VkDeviceSize size;
    };

    /**
     * Find a suitable device memory type that can be used to store data for
     * the buffer.
     *
     * @param mem_props desired memory properties.
     *
     * @return mem_type memory type as an index into the list of device memory
     *         types, and the memory size as required by the buffer.
     */
    mem_type find_memory_type(VkMemoryPropertyFlags mem_props) const;

public:
    vk_buffer(vk_device& device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags mem_props);
    ~vk_buffer();

    VkBuffer& get();
    const VkBuffer& get() const;

    void write_to_memory(void* data, VkDeviceSize size);
};

class vk_fence
{
    vk_device& m_device;
    VkFence m_fence = null_value<VkFence>::value;

public:
    vk_fence(vk_device& device, VkFenceCreateFlags flags);
    ~vk_fence();

    VkFence& get();

    void wait();
};

class vk_descriptor_pool
{
    vk_device& m_device;
    VkDescriptorPool m_pool = null_value<VkDescriptorPool>::value;

public:
    vk_descriptor_pool(vk_device& device, uint32_t max_sets, std::initializer_list<VkDescriptorPoolSize> sizes);
    ~vk_descriptor_pool();

    vk_descriptor_set allocate(const vk_descriptor_set_layout& ds_layout);
};

class vk_descriptor_set_layout
{
    vk_device& m_device;
    VkDescriptorSetLayout m_ds_layout = null_value<VkDescriptorSetLayout>::value;

public:
    vk_descriptor_set_layout(vk_device& device, std::initializer_list<VkDescriptorSetLayoutBinding> bindings);
    ~vk_descriptor_set_layout();

    VkDescriptorSetLayout& get();
    const VkDescriptorSetLayout& get() const;
};

/**
 * Descriptor set contains a collection of descriptors. Think of a
 * descriptor as a handle into a resource, which can be a buffer or an
 * image.
 *
 * @see https://vkguide.dev/docs/chapter-4/descriptors/
 *
 */
class vk_descriptor_set
{
    friend class vk_descriptor_pool;
    VkDescriptorSet m_set = null_value<VkDescriptorSet>::value;

    vk_descriptor_set(VkDescriptorSet ds);
public:
    ~vk_descriptor_set();

    VkDescriptorSet& get();
    const VkDescriptorSet& get() const;

    /**
     * Update the descriptor set with the content of the device local buffer.
     *
     * @param device logical device
     * @param binding binding position
     * @param type descriptor type.
     * @param buffer device local buffer to get the content from.
     */
    void update(const vk_device& device, uint32_t binding, VkDescriptorType type, const vk_buffer& buffer);
};

class vk_pipeline_layout
{
    vk_device& m_device;
    VkPipelineLayout m_layout = null_value<VkPipelineLayout>::value;

public:
    vk_pipeline_layout(vk_device& device, vk_descriptor_set_layout& ds_layout);
    ~vk_pipeline_layout();

    VkPipelineLayout& get();
    const VkPipelineLayout& get() const;
};

class vk_pipeline_cache
{
    vk_device& m_device;
    VkPipelineCache m_cache = null_value<VkPipelineCache>::value;

public:
    vk_pipeline_cache(vk_device& device);
    ~vk_pipeline_cache();

    VkPipelineCache& get();
    const VkPipelineCache& get() const;
};

class vk_shader_module
{
    vk_device& m_device;
    VkShaderModule m_module = null_value<VkShaderModule>::value;

public:
    enum class module_type { fibonacci };

    vk_shader_module(vk_device& device, module_type mt);
    ~vk_shader_module();

    VkShaderModule& get();
    const VkShaderModule& get() const;
};

class vk_pipeline
{
    vk_device& m_device;
    VkPipeline m_pipeline = null_value<VkPipeline>::value;

public:
    vk_pipeline(vk_device& device, vk_pipeline_layout& pl_layout, vk_pipeline_cache& pl_cache, vk_shader_module& shader);
    ~vk_pipeline();

    VkPipeline& get();
    const VkPipeline& get() const;
};

}}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
