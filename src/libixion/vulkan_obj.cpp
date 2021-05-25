/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "vulkan_obj.hpp"
#include "debug.hpp"

#include "ixion/macros.hpp"

#include <vector>
#include <cstring>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <bitset>

namespace ixion { namespace draft {

namespace {

#include "vulkan_spirv_blobs.inl"

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

VkInstance& vk_instance::get() { return m_instance; }

vk_queue::vk_queue(VkQueue queue) : m_queue(queue) {}

vk_queue::~vk_queue() {}

void vk_queue::submit(vk_command_buffer& cmd, vk_fence& fence)
{
    VkSubmitInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    info.commandBufferCount = 1;
    info.pCommandBuffers = &cmd.get();

    VkResult res = vkQueueSubmit(m_queue, 1, &info, fence.get());
    if (res != VK_SUCCESS)
        throw std::runtime_error("failed to submit command to queue.");
}

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

    m_physical_device = devices[0]; // pick the first physical device for now.

    {
        uint32_t n_qfs;
        vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &n_qfs, nullptr);

        std::vector<VkQueueFamilyProperties> qf_props(n_qfs);
        vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &n_qfs, qf_props.data());

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
		res = vkCreateDevice(m_physical_device, &device_ci, nullptr, &m_device);
        if (res != VK_SUCCESS)
            throw std::runtime_error("failed to create a logical device.");

        vkGetDeviceQueue(m_device, m_queue_family_index, 0, &m_queue);
    }
}

vk_device::~vk_device()
{
    vkDestroyDevice(m_device, nullptr);
}

VkDevice& vk_device::get()
{
    return m_device;
}

const VkDevice& vk_device::get() const
{
    return m_device;
}

VkPhysicalDevice vk_device::get_physical_device()
{
    return m_physical_device;
}

vk_queue vk_device::get_queue()
{
    return vk_queue(m_queue);
}

VkDevice& vk_command_pool::get_device()
{
    return m_device;
}

VkCommandPool& vk_command_pool::get()
{
    return m_cmd_pool;
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

vk_command_buffer vk_command_pool::create_command_buffer()
{
    return vk_command_buffer(*this);
}

vk_command_buffer::vk_command_buffer(vk_command_pool& cmd_pool) :
    m_cmd_pool(cmd_pool)
{
    VkCommandBufferAllocateInfo cb_ai {};
    cb_ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cb_ai.commandPool = m_cmd_pool.get();
    cb_ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cb_ai.commandBufferCount = 1;

    VkResult res = vkAllocateCommandBuffers(m_cmd_pool.get_device(), &cb_ai, &m_cmd_buffer);
    if (res != VK_SUCCESS)
        throw std::runtime_error("failed to create a command buffer.");
}

vk_command_buffer::~vk_command_buffer()
{
    vkFreeCommandBuffers(m_cmd_pool.get_device(), m_cmd_pool.get(), 1, &m_cmd_buffer);
}

VkCommandBuffer& vk_command_buffer::get()
{
    return m_cmd_buffer;
}

void vk_command_buffer::begin()
{
    VkCommandBufferBeginInfo info{};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(m_cmd_buffer, &info);
}

void vk_command_buffer::end()
{
    vkEndCommandBuffer(m_cmd_buffer);
}

void vk_command_buffer::copy_buffer(vk_buffer& src, vk_buffer& dst, VkDeviceSize size)
{
    VkBufferCopy copy_region{};
    copy_region.size = size;
    vkCmdCopyBuffer(m_cmd_buffer, src.get(), dst.get(), 1, &copy_region);
}

vk_buffer::mem_type vk_buffer::find_memory_type(VkMemoryPropertyFlags mem_props) const
{
    mem_type ret;

    VkPhysicalDeviceMemoryProperties pm_props;
    vkGetPhysicalDeviceMemoryProperties(m_device.get_physical_device(), &pm_props);

    VkMemoryRequirements mem_reqs;
    vkGetBufferMemoryRequirements(m_device.get(), m_buffer, &mem_reqs);

    ret.size = mem_reqs.size;

    IXION_TRACE("buffer memory requirements:");
    IXION_TRACE("  - size: " << mem_reqs.size);
    IXION_TRACE("  - alignment: " << mem_reqs.alignment);
    IXION_TRACE("  - memory type bits: "
        << std::bitset<32>(mem_reqs.memoryTypeBits)
        << " (" << mem_reqs.memoryTypeBits << ")");

    IXION_TRACE("requested memory property flags: 0x"
        << std::setw(2) << std::hex << std::setfill('0') << mem_props);

    IXION_TRACE("memory types: n=" << std::dec << pm_props.memoryTypeCount);
    for (uint32_t i = 0; i < pm_props.memoryTypeCount; ++i)
    {
        auto mt = pm_props.memoryTypes[i];
        IXION_TRACE("- "
            << std::setw(2)
            << std::setfill('0')
            << i
            << ": property flags: " << std::bitset<32>(mt.propertyFlags) << " (0x"
            << std::setw(2)
            << std::hex
            << std::setfill('0')
            << mt.propertyFlags
            << "); heap index: "
            << std::dec
            << mt.heapIndex);

        if ((mem_reqs.memoryTypeBits & 1) == 1)
        {
            // The buffer can take this memory type.  Now, check against
            // the requested usage types.

            if ((mt.propertyFlags & mem_props) == mem_props)
            {
                ret.index = i;
                return ret;
            }
        }

        mem_reqs.memoryTypeBits >>= 1;
    }

    throw std::runtime_error("no usable memory type found!");
}

vk_buffer::vk_buffer(vk_device& device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags mem_props) :
    m_device(device)
{
    VkBufferCreateInfo buf_ci {};
    buf_ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buf_ci.usage = usage;
    buf_ci.size = size;
    buf_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    IXION_TRACE("buffer usage flags: " << std::bitset<32>(usage) << " (0x" << std::hex << usage << ")");
    IXION_TRACE("memory property flags: " << std::bitset<32>(mem_props) << " (0x" << std::hex << mem_props << ")");
    IXION_TRACE("buffer size: " << std::dec << size);

    VkResult res = vkCreateBuffer(m_device.get(), &buf_ci, nullptr, &m_buffer);
    if (res != VK_SUCCESS)
        throw std::runtime_error("failed to create buffer.");

    mem_type mt = find_memory_type(mem_props);

    IXION_TRACE("memory type: (index=" << mt.index << "; size=" << mt.size << ")");

    VkMemoryAllocateInfo mem_ai {};
    mem_ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mem_ai.allocationSize = mt.size;
    mem_ai.memoryTypeIndex = mt.index;

    res = vkAllocateMemory(m_device.get(), &mem_ai, nullptr, &m_memory);
    if (res != VK_SUCCESS)
        throw std::runtime_error("failed to allocate memory.");

    res = vkBindBufferMemory(m_device.get(), m_buffer, m_memory, 0);
    if (res != VK_SUCCESS)
        throw std::runtime_error("failed to bind buffer to memory.");
}

vk_buffer::~vk_buffer()
{
    vkFreeMemory(m_device.get(), m_memory, nullptr);
    vkDestroyBuffer(m_device.get(), m_buffer, nullptr);
}

VkBuffer& vk_buffer::get()
{
    return m_buffer;
}

const VkBuffer& vk_buffer::get() const
{
    return m_buffer;
}

void vk_buffer::write_to_memory(void* data, VkDeviceSize size)
{
    IXION_TRACE("copying data of size " << size);
    void* mapped = nullptr;
    VkResult res = vkMapMemory(m_device.get(), m_memory, 0, size, 0, &mapped);
    if (res != VK_SUCCESS)
        throw std::runtime_error("failed to map memory.");

    memcpy(mapped, data, size);

    // flush the modified memory range.
    VkMappedMemoryRange flush_range{};
    flush_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    flush_range.memory = m_memory;
    flush_range.offset = 0;
    flush_range.size = size;
    vkFlushMappedMemoryRanges(m_device.get(), 1, &flush_range);

    vkUnmapMemory(m_device.get(), m_memory);
}

vk_fence::vk_fence(vk_device& device, VkFenceCreateFlags flags) :
    m_device(device)
{
    VkFenceCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    info.flags = flags;

    vkCreateFence(m_device.get(), &info, nullptr, &m_fence);
}

vk_fence::~vk_fence()
{
    vkDestroyFence(m_device.get(), m_fence, nullptr);
}

VkFence& vk_fence::get()
{
    return m_fence;
}

void vk_fence::wait()
{
    VkResult res = vkWaitForFences(m_device.get(), 1, &m_fence, VK_TRUE, UINT64_MAX);
    if (res != VK_SUCCESS)
        throw std::runtime_error("failed to wait for a fence.");
}

vk_descriptor_pool::vk_descriptor_pool(
    vk_device& device, uint32_t max_sets, std::initializer_list<VkDescriptorPoolSize> sizes) :
    m_device(device)
{
    VkDescriptorPoolCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.poolSizeCount = sizes.size();
    info.pPoolSizes = sizes.begin();
    info.maxSets = max_sets;

    VkResult res = vkCreateDescriptorPool(m_device.get(), &info, nullptr, &m_pool);
    if (res != VK_SUCCESS)
        throw std::runtime_error("failed to create a descriptor pool.");
}

vk_descriptor_pool::~vk_descriptor_pool()
{
    vkDestroyDescriptorPool(m_device.get(), m_pool, nullptr);
}

vk_descriptor_set vk_descriptor_pool::allocate(const vk_descriptor_set_layout& ds_layout)
{
    VkDescriptorSetAllocateInfo ai{};
    ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    ai.descriptorPool = m_pool;
    ai.pSetLayouts = &ds_layout.get();
    ai.descriptorSetCount = 1u;

    VkDescriptorSet ds;
    VkResult res = vkAllocateDescriptorSets(m_device.get(), &ai, &ds);
    if (res != VK_SUCCESS)
        throw std::runtime_error("failed to allocate a descriptor set.");

    return vk_descriptor_set(ds);
}

vk_descriptor_set_layout::vk_descriptor_set_layout(
    vk_device& device, std::initializer_list<VkDescriptorSetLayoutBinding> bindings) :
    m_device(device)
{
    VkDescriptorSetLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.pBindings = bindings.begin();
    info.bindingCount = bindings.size();

    VkResult res = vkCreateDescriptorSetLayout(m_device.get(), &info, nullptr, &m_ds_layout);
    if (res != VK_SUCCESS)
        throw std::runtime_error("failed to create a descriptor set layout.");
}

vk_descriptor_set_layout::~vk_descriptor_set_layout()
{
    vkDestroyDescriptorSetLayout(m_device.get(), m_ds_layout, nullptr);
}

VkDescriptorSetLayout& vk_descriptor_set_layout::get()
{
    return m_ds_layout;
}

const VkDescriptorSetLayout& vk_descriptor_set_layout::get() const
{
    return m_ds_layout;
}

vk_descriptor_set::vk_descriptor_set(VkDescriptorSet ds) :
    m_set(ds) {}

vk_descriptor_set::~vk_descriptor_set() {}

VkDescriptorSet& vk_descriptor_set::get()
{
    return m_set;
}

const VkDescriptorSet& vk_descriptor_set::get() const
{
    return m_set;
}

void vk_descriptor_set::update(
    const vk_device& device, uint32_t binding, VkDescriptorType type,
    const vk_buffer& buffer)
{
    VkDescriptorBufferInfo buffer_desc = { buffer.get(), 0, VK_WHOLE_SIZE };

    VkWriteDescriptorSet write_ds{};
    write_ds.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_ds.dstSet = m_set;
    write_ds.descriptorType = type;
    write_ds.dstBinding = binding;
    write_ds.pBufferInfo = &buffer_desc;
    write_ds.descriptorCount = 1u;

    vkUpdateDescriptorSets(device.get(), 1u, &write_ds, 0, nullptr);
}

vk_pipeline_layout::vk_pipeline_layout(
    vk_device& device, vk_descriptor_set_layout& ds_layout) :
    m_device(device)
{
    VkPipelineLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info.setLayoutCount = 1;
    info.pSetLayouts = &ds_layout.get();

    VkResult res = vkCreatePipelineLayout(m_device.get(), &info, nullptr, &m_layout);
    if (res != VK_SUCCESS)
        throw std::runtime_error("failed to create a pipeline layout.");
}

vk_pipeline_layout::~vk_pipeline_layout()
{
    vkDestroyPipelineLayout(m_device.get(), m_layout, nullptr);
}

VkPipelineLayout& vk_pipeline_layout::get()
{
    return m_layout;
}

const VkPipelineLayout& vk_pipeline_layout::get() const
{
    return m_layout;
}

vk_pipeline_cache::vk_pipeline_cache(vk_device& device) :
    m_device(device)
{
    VkPipelineCacheCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    VkResult res = vkCreatePipelineCache(m_device.get(), &info, nullptr, &m_cache);
    if (res != VK_SUCCESS)
        throw std::runtime_error("failed to create a pipeline cache.");
}

vk_pipeline_cache::~vk_pipeline_cache()
{
    vkDestroyPipelineCache(m_device.get(), m_cache, nullptr);
}

VkPipelineCache& vk_pipeline_cache::get()
{
    return m_cache;
}

const VkPipelineCache& vk_pipeline_cache::get() const
{
    return m_cache;
}

vk_shader_module::vk_shader_module(vk_device& device, module_type mt) :
    m_device(device)
{
    const uint32_t* array = nullptr;
    std::size_t n_array = 0;

    switch (mt)
    {
        case module_type::fibonacci:
        {
            array = reinterpret_cast<const uint32_t*>(fibonacci_spirv);
            n_array = IXION_N_ELEMENTS(fibonacci_spirv);

            IXION_TRACE("module type: fibonacci (size=" << n_array << ")");

            break;
        }
        default:
            throw std::runtime_error("invalid module type");
    }

    VkShaderModuleCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ci.codeSize = n_array;
    ci.pCode = array;

    VkResult res = vkCreateShaderModule(m_device.get(), &ci, NULL, &m_module);
    if (res != VK_SUCCESS)
        throw std::runtime_error("failed to create a shader module.");
}

vk_shader_module::~vk_shader_module()
{
    vkDestroyShaderModule(m_device.get(), m_module, nullptr);
}

VkShaderModule& vk_shader_module::get()
{
    return m_module;
}

const VkShaderModule& vk_shader_module::get() const
{
    return m_module;
}

vk_pipeline::vk_pipeline(
    vk_device& device, vk_pipeline_layout& pl_layout, vk_pipeline_cache& pl_cache,
    vk_shader_module& shader) :
    m_device(device)
{
    struct sp_data_type {
        uint32_t BUFFER_ELEMENTS = 32;
    } sp_data;

    VkSpecializationMapEntry sp_map_entry{};
    sp_map_entry.constantID = 0;
    sp_map_entry.offset = 0;
    sp_map_entry.size = sizeof(uint32_t);

    VkSpecializationInfo sp_info{};
    sp_info.mapEntryCount = 1;
    sp_info.pMapEntries = &sp_map_entry;
    sp_info.dataSize = sizeof(sp_data_type);
    sp_info.pData = &sp_data;

    // Data about the shader module, with special constant data via specialiation
    // info member.
    VkPipelineShaderStageCreateInfo shader_stage_ci{};
    shader_stage_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage_ci.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shader_stage_ci.module = shader.get();
    shader_stage_ci.pName = "main";
    shader_stage_ci.pSpecializationInfo = &sp_info;

    VkComputePipelineCreateInfo pipeline_ci{};
    pipeline_ci.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_ci.layout = pl_layout.get();
    pipeline_ci.flags = 0;
    pipeline_ci.stage = shader_stage_ci;

    vkCreateComputePipelines(
        m_device.get(), pl_cache.get(), 1, &pipeline_ci, nullptr, &m_pipeline);
}

vk_pipeline::~vk_pipeline()
{
    vkDestroyPipeline(m_device.get(), m_pipeline, nullptr);
}

VkPipeline& vk_pipeline::get()
{
    return m_pipeline;
}

const VkPipeline& vk_pipeline::get() const
{
    return m_pipeline;
}

}}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
