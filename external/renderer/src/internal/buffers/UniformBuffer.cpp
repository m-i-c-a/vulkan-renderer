#include "UniformBuffer.hpp"
#include "../misc/logger.hpp"

UniformBuffer::UniformBuffer(const uint32_t frame_resource_count, const uint64_t per_frame_buffer_size, const std::unordered_map<std::string, DescriptorVariable>&& member_var_refl_set)
    : m_frame_resource_count { frame_resource_count }
    , m_per_frame_buffer_size { per_frame_buffer_size }
    , m_member_var_refl_set{ member_var_refl_set.begin(), member_var_refl_set.end() }
{
    const VkBufferCreateInfo buffer_create_info {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0x0,
        .size = m_frame_resource_count * per_frame_buffer_size,
        .usage = (VkBufferUsageFlags)(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT),
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    };

    uint64_t allocated_size = 0;
    m_vk_handle_buffer = vk_core::create_buffer(buffer_create_info);
    m_vk_handle_memory = vk_core::allocate_buffer_memory(m_vk_handle_buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, allocated_size);
    vk_core::bind_buffer_memory(m_vk_handle_buffer, m_vk_handle_memory);
    vk_core::map_memory(m_vk_handle_memory, 0, allocated_size, 0x0, (void**)&m_mapped_data);

    m_cpu_data.resize(m_per_frame_buffer_size);
    m_per_frame_dirty_members.resize(m_frame_resource_count);
}

UniformBuffer::~UniformBuffer()
{
    vk_core::unmap_memory(m_vk_handle_memory);
    vk_core::destroy_buffer(m_vk_handle_buffer);
    vk_core::free_memory(m_vk_handle_memory);
}

void UniformBuffer::update_member(const std::string& member_name, const void* data)
{
    const auto it = m_member_var_refl_set.find(member_name);
    ASSERT(it != m_member_var_refl_set.end(), "Member variable %s not found in uniform buffer!\n", member_name.c_str());

    const DescriptorVariable& member_var_refl = it->second;
    const uint64_t offset = member_var_refl.offset;
    const uint64_t size = member_var_refl.size;

    memcpy(m_cpu_data.data() + offset, data, size);

    for (uint32_t i = 0; i < m_frame_resource_count; i++)
    {
        m_per_frame_dirty_members[i].insert(member_var_refl);
    }
}

void UniformBuffer::flush_updates(const uint32_t frame_resource_idx)
{
    for (const DescriptorVariable& member_var_refl : m_per_frame_dirty_members[frame_resource_idx])
    {
        const uint64_t offset = member_var_refl.offset;
        const uint64_t size = member_var_refl.size;

        memcpy(m_mapped_data + frame_resource_idx * m_per_frame_buffer_size + offset, m_cpu_data.data() + offset, size);
    }

    m_per_frame_dirty_members[frame_resource_idx].clear();
}

VkDescriptorBufferInfo UniformBuffer::get_descriptor_buffer_info(const uint32_t frame_resource_idx) const
{
    return VkDescriptorBufferInfo {
        .buffer = m_vk_handle_buffer,
        .offset = frame_resource_idx * m_per_frame_buffer_size,
        .range = m_per_frame_buffer_size,
    };
}

