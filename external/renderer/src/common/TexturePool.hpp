#ifndef RENDERER_TEXTURE_POOL_HPP
#define RENDERER_TEXTURE_POOL_HPP

#include "defines.hpp"

#include <vulkan/vulkan.h>

#include <inttypes.h>
#include <vector>
#include <stack>


struct TexturePool
{
protected:
    std::stack<uint32_t> m_free_index_stack;
    std::vector<VkDescriptorImageInfo> m_slice_list;
public:
    TexturePool();

    uint32_t acquire_slice();

    void update_slice(const VkDescriptorImageInfo& desc_image_info, const uint32_t slice_idx);

    VkDescriptorImageInfo get_desc_image_info(const uint32_t slice_idx) const;
};

void TexturePool::update_slice(const VkDescriptorImageInfo& desc_image_info, const uint32_t slice_idx)
{
    ASSERT(slice_idx < m_slice_list.size(), "Invalid slice index");;

    VkWriteDescriptorSet write_desc_set {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = vk_handle_desc_set_list[i],
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pImageInfo = nullptr,
        .pBufferInfo = &frame_ubo_desc_buffer_info,
        .pTexelBufferView = nullptr,
    }; 
}

VkDescriptorImageInfo TexturePool::get_desc_image_info(const uint32_t slice_idx) const
{
    ASSERT(slice_idx < m_slice_list.size(), "Invalid slice index");;

    return m_slice_list[slice_idx];
}

#endif // RENDERER_TEXTURE_POOL_HPP