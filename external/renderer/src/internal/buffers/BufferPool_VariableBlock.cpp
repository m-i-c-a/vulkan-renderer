#include "BufferPool_VariableBlock.hpp"
#include "vk_core.hpp"

BufferPool_VariableBlock::BufferPool_VariableBlock(const uint32_t frame_resource_count, const uint64_t per_frame_buffer_size)
    : m_frame_resource_count { frame_resource_count }
    , m_per_frame_buffer_size { per_frame_buffer_size }
{
    const VkBufferCreateInfo buffer_create_info {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0x0,
        .size = m_frame_resource_count * per_frame_buffer_size,
        .usage = (VkBufferUsageFlags)(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT),
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    };

    uint64_t allocated_size = 0;
    m_vk_handle_buffer = vk_core::create_buffer(buffer_create_info);
    m_vk_handle_memory = vk_core::allocate_buffer_memory(m_vk_handle_buffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, allocated_size);
    vk_core::bind_buffer_memory(m_vk_handle_buffer, m_vk_handle_memory);

    m_cpu_data.resize(per_frame_buffer_size, 0);
    m_per_frame_dirty_blocks.resize(frame_resource_count);
}

BufferPool_VariableBlock::~BufferPool_VariableBlock()
{
    vk_core::free_memory(m_vk_handle_memory);
    vk_core::destroy_buffer(m_vk_handle_buffer);
} 

uint32_t BufferPool_VariableBlock::acquire_block(const uint32_t block_size)
{
    if (block_size <= 0)
        return -1;

    m_current_offset = std::ceil(static_cast<float>(m_current_offset) / static_cast<float>(block_size)) * block_size + block_size;

    return (m_current_offset / block_size) - 1;
}

void* BufferPool_VariableBlock::get_writable_block(const uint32_t block_size, const uint32_t block_id)
{
    for (uint32_t i = 0; i < m_frame_resource_count; i++)
    {
        m_per_frame_dirty_blocks[i].insert({block_size, block_id});
    }

    return (void*)(&(m_cpu_data[block_id * block_size]));
}

const std::vector<UploadInfo> BufferPool_VariableBlock::get_queued_uploads(const uint32_t frame_resource_idx)
{
   std::unordered_set<DirtyBlockID, DirtyBlockID::Hash>& frame_dirty_blocks = m_per_frame_dirty_blocks[frame_resource_idx]; 

   if (frame_dirty_blocks.empty())
   {
       return {};
   }   

   std::vector<UploadInfo> upload_info_list {};
   upload_info_list.reserve(frame_dirty_blocks.size());

   for (const DirtyBlockID& dirty_block : frame_dirty_blocks)
   {
       const VkDeviceSize offset = dirty_block.block_size * dirty_block.block_id;
       const UploadInfo upload_info {
            .dst_offset = offset,
            .size = dirty_block.block_size,
            .data_pointer = &m_cpu_data[offset], 
       };

       upload_info_list.push_back(upload_info);
   }

   frame_dirty_blocks.clear();

   return upload_info_list;
} 

VkDescriptorBufferInfo BufferPool_VariableBlock::get_descriptor_buffer_info(const uint32_t frame_resource_idx) const
{
    const VkDescriptorBufferInfo buffer_info {
        .buffer = m_vk_handle_buffer,
        .offset = m_per_frame_buffer_size * frame_resource_idx,
        .range = m_per_frame_buffer_size
    };

    return buffer_info;
};
