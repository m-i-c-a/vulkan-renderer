#ifndef RENDERER_BUFFER_POOL_CONSTANT_BLOCK_HPP
#define RENDERER_BUFFER_POOL_CONSTANT_BLOCK_HPP

#include <vulkan/vulkan.h>

#include <vector>
#include <unordered_set>
#include <stack>

class StagingBuffer;

struct BufferPool_ConstantBlock
{
private:
    const VkDevice m_device = VK_NULL_HANDLE;
    const uint32_t m_block_stride = 0;
    const uint32_t m_frame_count = 0;
    const uint32_t m_per_frame_block_count = 0;
    const VkDeviceSize m_per_frame_buffer_size = 0;

    std::vector<std::unordered_set<uint32_t>> m_per_frame_dirty_block_ids;

    std::vector<uint8_t> m_cpu_data;
    std::stack<uint32_t> m_free_block_ids;

    uint32_t m_active_block_count = 0;

    VkBuffer m_buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
public:
    BufferPool_ConstantBlock(const uint32_t frame_count, const uint32_t block_stride, const uint32_t block_count, const VkBufferUsageFlags buffer_usage); 
    ~BufferPool_ConstantBlock();

    uint32_t acquire_block();
    const void* const get_readable_block(const uint32_t id) const;
    void* get_writable_block(const uint32_t id);
    void queue_upload_dirty_blocks(const VkCommandBuffer vk_cmdBuff, StagingBuffer* staging_buffer, const uint32_t frame_idx);

    VkDescriptorBufferInfo get_desc_buffer_info(const uint32_t frame_idx) const;

#ifdef DEBUG
    VkBuffer getStagingBufferHandle() const;
    VkBuffer getStorageBufferHandle() const;
#endif
};

#endif // RENDERER_BUFFER_POOL_CONSTANT_BLOCK_HPP