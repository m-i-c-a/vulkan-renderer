#include "vk_core.hpp"
#include "renderer.hpp"

#include <GLFW/glfw3.h>

#include <assert.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <string.h>
    
constexpr uint32_t window_width = 400u;
constexpr uint32_t window_height = 400u;
constexpr uint32_t frame_resource_count = 1u;

renderer::Renderable load_renderable(const VkCommandPool vk_handle_cmd_pool, const VkCommandBuffer vk_handle_cmd_buff);
void blit(const uint32_t frame_resource_idx, const VkCommandBuffer vk_handle_cmd_buff);

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow *glfw_window = glfwCreateWindow(static_cast<int>(window_width), static_cast<int>(window_height), "App", nullptr, nullptr);
    assert(glfw_window && "Failed to create window");

    vk_core::init(window_width, window_height, glfw_window, "/home/mica/Desktop/clean-start/data/json/vulkan_info.json");

    const renderer::InitInfo renderer_init_info {
        .window_width = window_width, 
        .window_height = window_height, 
        .frame_resource_count = frame_resource_count, 
        .app_config_file = "/home/mica/Desktop/clean-start/data/json/app_info.json", 
        .sortbin_config_file = "/home/mica/Desktop/clean-start/data/json/sortbin_info.json",
        .sortbin_reflection_config_file = "/home/mica/Desktop/clean-start/data/json/sortbin_reflection_info_simple.json",
        .shader_root_path = "/home/mica/Desktop/clean-start/data/shaders/spirv/",
    };

    renderer::init(renderer_init_info);

    const VkCommandPool vk_handle_cmd_pool = vk_core::create_command_pool(0x0);
    const VkCommandBuffer vk_handle_cmd_buff = vk_core::allocate_command_buffer(vk_handle_cmd_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    const VkFence vk_handle_swapchain_image_acquire_fence = vk_core::create_fence(0x0);

    renderer::Renderable test_renderable = load_renderable(vk_handle_cmd_pool, vk_handle_cmd_buff);

    renderer::add_renderable_to_sortbin(test_renderable);

    uint32_t frame_idx = 0;

    while (!glfwWindowShouldClose(glfw_window))
    {
        glfwPollEvents();

        const uint32_t frame_resource_idx = frame_idx % frame_resource_count;

        vk_core::acquire_next_swapchain_image(VK_NULL_HANDLE, vk_handle_swapchain_image_acquire_fence);
        vk_core::wait_for_fences(1, &vk_handle_swapchain_image_acquire_fence, VK_TRUE, UINT64_MAX);
        vk_core::reset_fences(1, &vk_handle_swapchain_image_acquire_fence);

        vk_core::reset_command_pool(vk_handle_cmd_pool);

        const VkCommandBufferBeginInfo cmd_buff_begin_info {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = 0x0,
            .pInheritanceInfo = nullptr,
        };

        vkBeginCommandBuffer(vk_handle_cmd_buff, &cmd_buff_begin_info);
        
        // Bind Set 0

        renderer::record_render_pass(0, vk_handle_cmd_buff, {0, 0, window_width, window_height}, frame_resource_idx);

        blit(frame_resource_idx, vk_handle_cmd_buff);

        vkEndCommandBuffer(vk_handle_cmd_buff);

        const VkPipelineStageFlags none_flag = VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM;

        const VkSubmitInfo submit_info {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = nullptr,
            .waitSemaphoreCount = 0,
            .pWaitSemaphores = nullptr,
            .pWaitDstStageMask = &none_flag,
            .commandBufferCount = 1,
            .pCommandBuffers = &vk_handle_cmd_buff,
            .signalSemaphoreCount = 0,
            .pSignalSemaphores = nullptr,
        };

        vk_core::queue_submit(1, &submit_info, VK_NULL_HANDLE);
        
        vk_core::device_wait_idle();

        vk_core::present(0, nullptr);

        frame_idx++;
    }

    vk_core::destroy_command_pool(vk_handle_cmd_pool);
    vk_core::destroy_fence(vk_handle_swapchain_image_acquire_fence);

    glfwDestroyWindow(glfw_window);
    glfwTerminate();

    renderer::terminate();
    vk_core::terminate();

    return 0;
}

renderer::Renderable load_renderable(const VkCommandPool vk_handle_cmd_pool, const VkCommandBuffer vk_handle_cmd_buff)
{
    const float vertex_data[9] = {
        0.0, -0.5, 0.0,
        0.5,  0.5, 0.0,
        -0.5, 0.5, 0.0
    };

    const uint32_t index_data[3] = { 0, 1, 2 };

    renderer::MeshInitInfo mesh_init_info {
        .sortbin_id = 0,
        .vertex_stride = 12,
        .vertex_count = 3,
        .vertex_data = (uint8_t*)vertex_data,
        .index_count = 3,
        .index_data = (uint8_t*)index_data,
        .index_stride = 4,
        .material_data = {},
        .draw_data = {},
    };

    mesh_init_info.material_data.resize(12, 0);
    std::vector<float> material_data {{ 0.0, 1.0, 0.0 }};
    memcpy(mesh_init_info.material_data.data(), material_data.data(), 12);

    const renderer::Renderable renderable = renderer::load_mesh(mesh_init_info);

    const VkCommandBufferBeginInfo cmd_buff_begin_info {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = 0x0,
        .pInheritanceInfo = nullptr,
    };

    vk_core::reset_command_pool(vk_handle_cmd_pool);
    
    vkBeginCommandBuffer(vk_handle_cmd_buff, &cmd_buff_begin_info);

    renderer::flush_geometry_uploads(vk_handle_cmd_buff);

    vkEndCommandBuffer(vk_handle_cmd_buff);

    const VkPipelineStageFlags none_flag = VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM;

    const VkSubmitInfo submit_info {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = nullptr,
        .pWaitDstStageMask = &none_flag,
        .commandBufferCount = 1,
        .pCommandBuffers = &vk_handle_cmd_buff,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = nullptr,
    };

    vk_core::queue_submit(1, &submit_info, VK_NULL_HANDLE);

    vk_core::device_wait_idle();

    return renderable;
}

void blit(const uint32_t frame_resource_idx, const VkCommandBuffer vk_handle_cmd_buff)
{
    const VkImageMemoryBarrier pre_blit_image_barriers[2] {
        vk_core::get_active_swapchain_image_memory_barrier(
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 
            VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        ,
        {   // Base Color Attachment
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            .srcQueueFamilyIndex = vk_core::get_queue_family_idx(),
            .dstQueueFamilyIndex = vk_core::get_queue_family_idx(),
            .image = renderer::get_attachment_image(0, frame_resource_idx),
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            }
        }
    };

    vkCmdPipelineBarrier(
        vk_handle_cmd_buff,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_DEPENDENCY_BY_REGION_BIT,
        0, nullptr,
        0, nullptr,
        2, pre_blit_image_barriers);

    const VkImageBlit blit_info{
        .srcSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .srcOffsets = {
            { 0, 0, 0 },
            {static_cast<int32_t>(window_width), static_cast<uint32_t>(window_height), 1}
        },
        .dstSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .dstOffsets = {
            {0, 0, 0},
            {static_cast<int32_t>(window_width), static_cast<uint32_t>(window_height), 1}
        },
    };

    vkCmdBlitImage(
        vk_handle_cmd_buff,
        renderer::get_attachment_image(0, frame_resource_idx), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        vk_core::get_active_swapchain_image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &blit_info,
        VK_FILTER_NEAREST);

    const VkImageMemoryBarrier post_blit_image_barriers[2]{
        vk_core::get_active_swapchain_image_memory_barrier(
            VK_ACCESS_TRANSFER_WRITE_BIT,
            0,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
        ,
        {   // Base Color Attachment
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .srcQueueFamilyIndex = vk_core::get_queue_family_idx(),
            .dstQueueFamilyIndex = vk_core::get_queue_family_idx(),
            .image = renderer::get_attachment_image(0, frame_resource_idx),
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            }
        }
    };

    vkCmdPipelineBarrier(
        vk_handle_cmd_buff,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_DEPENDENCY_BY_REGION_BIT,
        0, nullptr,
        0, nullptr,
        2, post_blit_image_barriers);
}