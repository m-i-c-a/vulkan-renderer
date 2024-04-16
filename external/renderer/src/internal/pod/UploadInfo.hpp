#ifndef RENDERER_UPLOAD_INFO_HPP
#define RENDERER_UPLOAD_INFO_HPP

#include <vulkan/vulkan.h>

#include <inttypes.h>
#include <vector>

struct UploadInfo
{
    VkDeviceSize dst_offset;
    VkDeviceSize size;
    const void* data_pointer;
    std::vector<uint8_t> data_vector;
};

#endif // RENDERER_UPLOAD_INFO_HPP