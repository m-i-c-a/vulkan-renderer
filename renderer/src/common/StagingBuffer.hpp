#ifndef RENDERER_STAGING_BUFFER_HPP
#define RENDERER_STAGING_BUFFER_HPP

#include "vk_core.hpp"

#include <vulkan/vulkan.h>

#include <unordered_map>
#include <cstring>
#include <vector>

// This technically needs to consider frame resources as well. We don't want to change the CPU data while uploading.

struct StagingBuffer
{
private:
    VkDeviceSize m_buffer_size = 0;
    VkBuffer m_vk_handle_buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_vk_handle_memory = VK_NULL_HANDLE;

    uint8_t* m_mapped_ptr = nullptr;
    VkDeviceSize m_buffer_offset = 0;

    std::unordered_map<VkBuffer, std::vector<VkBufferCopy>> m_dst_buffer_copy_map;
public:
    StagingBuffer(const VkDeviceSize size);
    ~StagingBuffer();

    void queue_upload(const VkBuffer vk_handle_dst_buffer, const VkDeviceSize dst_offset, const VkDeviceSize upload_size, const void* const data);
    void flush(const VkCommandBuffer vk_handle_cmd_buff);
};

StagingBuffer::StagingBuffer(const VkDeviceSize size)
{
    const VkBufferCreateInfo buffer_create_info {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0x0,
        .size = size,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    };

    m_vk_handle_buffer = vk_core::create_buffer(buffer_create_info);
    m_vk_handle_memory = vk_core::allocate_buffer_memory(m_vk_handle_buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_buffer_size);
    vk_core::bind_buffer_memory(m_vk_handle_buffer, m_vk_handle_memory);
    vk_core::map_memory(m_vk_handle_memory, 0, m_buffer_size, 0x0, (void**)&m_mapped_ptr);
}


void StagingBuffer::queue_upload(const VkBuffer vk_handle_dst_buffer, const VkDeviceSize dst_offset, const VkDeviceSize upload_size, const void* const data)
{
    memcpy(m_mapped_ptr + m_buffer_offset, data, upload_size); 

    const VkBufferCopy buffer_copy {
        .srcOffset = m_buffer_offset,
        .dstOffset = dst_offset,
        .size = upload_size,
    };

    m_buffer_offset += upload_size;

    m_dst_buffer_copy_map[vk_handle_dst_buffer].push_back(buffer_copy);
}


void StagingBuffer::flush(const VkCommandBuffer vk_handle_cmd_buff)
{
    LOG("Flushing staging buffer\n");
    for (const auto& [vk_handle_dst_buffer, buff_copies] : m_dst_buffer_copy_map)
    {
        vkCmdCopyBuffer(vk_handle_cmd_buff, m_vk_handle_buffer, vk_handle_dst_buffer, static_cast<uint32_t>(buff_copies.size()), buff_copies.data());
    }

    m_dst_buffer_copy_map.clear();
    m_buffer_offset = 0; // this needs to wait until upload in complete
}

#endif // RENDERER_STAGING_BUFFER_HPP