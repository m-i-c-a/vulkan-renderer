#ifndef RENDERER_GEOMETRY_BUFFER_HPP
#define RENDERER_GEOMETRY_BUFFER_HPP

#include "vk_core.hpp"
#include "UploadInfo.hpp"

#include <vulkan/vulkan.h>

#include <vector>
#include <cmath>

struct GeometryBuffer
{
private:
    VkBuffer m_vk_handle_buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_vk_handle_buffer_memory = VK_NULL_HANDLE;
    VkDeviceSize m_buffer_size = 0;
    VkDeviceSize m_buffer_offset = 0;

    std::vector<UploadInfo> m_queued_upload_list;

public:
    GeometryBuffer(const VkBufferCreateInfo& create_info);
    ~GeometryBuffer();

    int32_t queue_upload(const uint32_t stride, const uint32_t count, std::vector<uint8_t>&& data);

    std::vector<UploadInfo>&& get_queued_uploads() { return std::move(m_queued_upload_list); }
    void reset_queued_uploads() { m_queued_upload_list.clear(); }

    VkBuffer get_vk_handle_buffer() const { return m_vk_handle_buffer; }
};

GeometryBuffer::GeometryBuffer(const VkBufferCreateInfo &create_info)
{
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

#endif // RENDERER_GEOMETRY_BUFFER_HPP