#ifndef VK_CORE_HPP
#define VK_CORE_HPP

#include <vulkan/vulkan.h>

#include <string_view>
#include <vector>

class GLFWwindow;

namespace vk_core
{
    VkSampler create_sampler(const VkSamplerCreateInfo& create_info);

    VkImage create_image(const VkImageCreateInfo& create_info);
    VkImageView create_image_view(const VkImageViewCreateInfo& create_info);
    VkDeviceMemory allocate_image_memory(const VkImage vk_handle_image, const VkMemoryPropertyFlags flags);
    void bind_image_memory(const VkImage vk_handle_image, const VkDeviceMemory vk_handle_image_memory);
    void destroy_image(const VkImage vk_handle_image);
    void destroy_image_view(const VkImageView vk_handle_image_view);

    VkBuffer create_buffer(const VkBufferCreateInfo& create_info);
    VkDeviceMemory allocate_buffer_memory(const VkBuffer vk_handle_buffer, const VkMemoryPropertyFlags flags, VkDeviceSize& size);
    void bind_buffer_memory(const VkBuffer vk_handle_buffer, const VkDeviceMemory vk_handle_buffer_memory); 
    void destroy_buffer(const VkBuffer vk_handle_buffer);

    VkShaderModule create_shader_module(const VkShaderModuleCreateInfo& create_info);
    void destroy_shader_module(const VkShaderModule vk_handle_shader_module);

    VkPipeline create_graphics_pipeline(const VkGraphicsPipelineCreateInfo& create_info);
    void destroy_pipeline(const VkPipeline vk_handle_pipeline);

    VkCommandPool create_command_pool(const VkCommandPoolCreateFlags flags);
    void reset_command_pool(const VkCommandPool pool);
    void destroy_command_pool(const VkCommandPool pool);

    VkDescriptorPool create_desc_pool(const VkDescriptorPoolCreateInfo& create_info);
    void destroy_desc_pool(const VkDescriptorPool vk_handle_desc_pool);

    VkDescriptorSetLayout create_desc_set_layout(const VkDescriptorSetLayoutCreateInfo& create_info);
    void destroy_desc_set_layout(const VkDescriptorSetLayout vk_handle_desc_set_layout);

    std::vector<VkDescriptorSet> allocate_desc_sets(const VkDescriptorSetAllocateInfo& alloc_info);
    void update_desc_sets(const uint32_t update_count, const VkWriteDescriptorSet* const p_write_desc_set_list, const uint32_t copy_count, const VkCopyDescriptorSet* const p_copy_desc_set_list);

    VkPipelineLayout create_pipeline_layout(const VkPipelineLayoutCreateInfo& create_info);
    void destroy_pipeline_layout(const VkPipelineLayout vk_handle_pipeline_layout);

    VkCommandBuffer allocate_command_buffer(const VkCommandPool cmd_pool, const VkCommandBufferLevel level);

    void map_memory(const VkDeviceMemory memory, const VkDeviceSize offset, const VkDeviceSize size, const VkMemoryMapFlags flags, void** data);
    void unmap_memory(const VkDeviceMemory vk_handle_memory);
    void free_memory(const VkDeviceMemory vk_handle_memory);

    void init(const uint32_t window_width, const uint32_t window_height, GLFWwindow* window, const std::string_view config_file);
    void terminate();


    // Events

    void queue_submit(const uint32_t submit_count, const VkSubmitInfo* const p_submit_infos, const VkFence vk_handle_signal_fence);

    void device_wait_idle();

    void present(const uint32_t wait_sem4_count, const VkSemaphore* const vk_handle_wait_sem4_list);
    void acquire_next_swapchain_image(const VkSemaphore vk_handle_signal_sem4, const VkFence vk_handle_signal_fence);

    VkFence create_fence(const VkFenceCreateFlags flags);
    void wait_for_fences(const uint32_t fence_count, const VkFence* vk_handle_fence_list, const VkBool32 wait_all, const uint64_t timeout);
    void reset_fences(const uint32_t fence_count, const VkFence* vk_handle_fence_list);
    void destroy_fence(const VkFence vk_handle_fence);

    // Getters

    VkImageMemoryBarrier get_active_swapchain_image_memory_barrier(const VkAccessFlags src_access_flags, const VkAccessFlags dst_access_flags, const VkImageLayout old_layout, const VkImageLayout new_layout);
    uint32_t get_queue_family_idx();
    VkImage get_active_swapchain_image();
};


#endif // VK_CORE_HPP