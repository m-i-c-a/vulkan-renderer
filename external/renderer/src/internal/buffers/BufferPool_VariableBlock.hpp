#ifndef RENDERER_BUFFER_POOL_VARIABLE_BLOCK_HPP
#define RENDERER_BUFFER_POOL_VARIABLE_BLOCK_HPP

#include "../pod/UploadInfo.hpp"

#include <vulkan/vulkan.h>

#include <inttypes.h>
#include <vector>
#include <unordered_set>
#include <cmath>

struct BufferPool_VariableBlock
{
private:
protected:

    struct DirtyBlockID
    {
        uint32_t block_size;
        uint32_t block_id;

        bool operator==(const DirtyBlockID& other) const
        {
            return block_size == other.block_size && 
                   block_id   == other.block_id;
        }

        struct Hash
        {
            std::size_t operator()(const DirtyBlockID& obj) const
            {
                return std::hash<uint32_t>()(obj.block_id * obj.block_size);
            }
        };
    };

    const uint32_t m_frame_resource_count = 0;
    const uint64_t m_per_frame_buffer_size = 0;
    VkBuffer m_vk_handle_buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_vk_handle_memory = VK_NULL_HANDLE;

    uint64_t m_current_offset = 0;
    std::vector<uint8_t> m_cpu_data;
    std::vector<std::unordered_set<DirtyBlockID, DirtyBlockID::Hash>> m_per_frame_dirty_blocks;

public:

    BufferPool_VariableBlock(const uint32_t frame_resource_count, const uint64_t per_frame_buffer_size);
    ~BufferPool_VariableBlock();

    BufferPool_VariableBlock(const BufferPool_VariableBlock&) = delete;
    BufferPool_VariableBlock& operator=(const BufferPool_VariableBlock&) = delete;
    BufferPool_VariableBlock(BufferPool_VariableBlock&&) = delete;
    BufferPool_VariableBlock& operator=(BufferPool_VariableBlock&&) = delete;

    uint32_t acquire_block(const uint32_t block_size);
    void* get_writable_block(const uint32_t block_size, const uint32_t block_id);
    const std::vector<UploadInfo> get_queued_uploads(const uint32_t frame_resource_idx); 

    VkBuffer get_vk_handle_buffer() const { return m_vk_handle_buffer; }
    VkDescriptorBufferInfo get_descriptor_buffer_info(const uint32_t frame_resource_idx) const;
};

#endif // RENDERER_BUFFER_POOL_VARIABLE_BLOCK_HPP