#ifndef RENDERER_UNIFORM_BUFFER_HPP
#define RENDERER_UNIFORM_BUFFER_HPP

#include "../pod/DescriptorVariable.hpp"
#include "vk_core.hpp"

#include <vulkan/vulkan.h>

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>
#include <string.h>

struct UniformBuffer
{
private:
protected:
    const uint32_t m_frame_resource_count = 0;
    const uint64_t m_per_frame_buffer_size = 0;
    const std::unordered_map<std::string, DescriptorVariable> m_member_var_refl_set;

    VkBuffer m_vk_handle_buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_vk_handle_memory = VK_NULL_HANDLE;
    uint8_t* m_mapped_data = nullptr;

    std::vector<uint8_t> m_cpu_data;
    std::vector<std::unordered_set<DescriptorVariable, DescriptorVariable::Hash>> m_per_frame_dirty_members;
public:
    UniformBuffer(const uint32_t frame_resource_count, const uint64_t per_frame_buffer_size, const std::unordered_map<std::string, DescriptorVariable>&& member_var_refl_set);
    ~UniformBuffer();

    UniformBuffer(const UniformBuffer&) = delete;
    UniformBuffer& operator=(const UniformBuffer&) = delete;
    UniformBuffer(UniformBuffer&&) = delete;
    UniformBuffer& operator=(UniformBuffer&&) = delete;

    void update_member(const std::string& member_name, const void* const data);
    void flush_updates(const uint32_t frame_resource_idx);
    VkDescriptorBufferInfo get_descriptor_buffer_info(const uint32_t frame_resource_idx) const;
};

#endif // RENDERER_UNIFORM_BUFFER_HPP