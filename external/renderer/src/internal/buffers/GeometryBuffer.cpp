#include "GeometryBuffer.hpp"
#include "vk_core.hpp"

GeometryBuffer::GeometryBuffer(const uint64_t size)
{    
    const VkBufferCreateInfo create_info {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0x0,
        .size = 1024 * 1024, 
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr
    };

    m_vk_handle_buffer = vk_core::create_buffer(create_info);
    m_vk_handle_buffer_memory = vk_core::allocate_buffer_memory(m_vk_handle_buffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_buffer_size);
    vk_core::bind_buffer_memory(m_vk_handle_buffer, m_vk_handle_buffer_memory);
}

GeometryBuffer::~GeometryBuffer()
{
    vk_core::destroy_buffer(m_vk_handle_buffer);
    vk_core::free_memory(m_vk_handle_buffer_memory);
}

int32_t GeometryBuffer::queue_upload(const uint32_t stride, const uint32_t count, std::vector<uint8_t>&& data)
{
    const int32_t nth_entity = m_buffer_offset == 0 ? 0 : std::ceil(m_buffer_offset / static_cast<float>(stride));
    m_buffer_offset = nth_entity * stride;
    const VkDeviceSize upload_size = count * stride;

    if (m_buffer_size < m_buffer_offset + upload_size)
    {
        return -1;
    }

    m_queued_upload_list.emplace_back(UploadInfo{
        m_buffer_offset,
        upload_size,
        nullptr,
        std::move(data)
    });

    m_buffer_offset += upload_size;

    data.clear();

    return nth_entity;
}
