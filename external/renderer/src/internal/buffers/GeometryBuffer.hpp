#ifndef RENDERER_GEOMETRY_BUFFER_HPP
#define RENDERER_GEOMETRY_BUFFER_HPP

#include "../pod/UploadInfo.hpp"

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
    GeometryBuffer(const uint64_t size);
    ~GeometryBuffer();

    int32_t queue_upload(const uint32_t stride, const uint32_t count, std::vector<uint8_t>&& data);

    std::vector<UploadInfo>&& get_queued_uploads() { return std::move(m_queued_upload_list); }
    void reset_queued_uploads() { m_queued_upload_list.clear(); }

    VkBuffer get_vk_handle_buffer() const { return m_vk_handle_buffer; }
};

#endif // RENDERER_GEOMETRY_BUFFER_HPP