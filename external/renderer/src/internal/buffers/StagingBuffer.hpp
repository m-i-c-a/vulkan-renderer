#ifndef RENDERER_STAGING_BUFFER_HPP
#define RENDERER_STAGING_BUFFER_HPP

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

#endif // RENDERER_STAGING_BUFFER_HPP