#include "vk_core.hpp"
#include "renderer.hpp"

#include "Camera.hpp"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <assert.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <cstring> // Add this line to include the header file for string manipulation functions
#include <iostream>

constexpr uint32_t window_width = 800u;
constexpr uint32_t window_height = 800u;
constexpr uint32_t frame_resource_count = 1u;

struct Entity
{
    const uint32_t renderable_id;
    const uint16_t sortbin_id;
};

Entity load_entity(const VkCommandPool vk_handle_cmd_pool, const VkCommandBuffer vk_handle_cmd_buff);
void blit(const uint32_t frame_resource_idx, const VkCommandBuffer vk_handle_cmd_buff);

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    const auto update_xyz_dir = [&](const glm::vec3& dir)
    {
        if (action == GLFW_PRESS)
        {
            Camera::update_xyz_dir(dir);
        }
        else if (action == GLFW_RELEASE)
        {
            Camera::update_xyz_dir(-dir);
        }
    };

    switch (key)
    {
        case GLFW_KEY_W:
            update_xyz_dir(glm::vec3(0.0f, 0.0f, 1.0f));
            break;
        case GLFW_KEY_S:
            update_xyz_dir(glm::vec3(0.0f, 0.0f, -1.0f));
            break;
        case GLFW_KEY_D:
            update_xyz_dir(glm::vec3(-1.0f, 0.0f, 0.0f));
            break;
        case GLFW_KEY_A:
            update_xyz_dir(glm::vec3(1.0f, 0.0f, 0.0f));
            break;
        case GLFW_KEY_E:
            update_xyz_dir(glm::vec3(0.0f, 1.0f, 0.0f));
            break;
        case GLFW_KEY_Q:
            update_xyz_dir(glm::vec3(0.0f, -1.0f, 0.0f));
            break;
    }

}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow *glfw_window = glfwCreateWindow(static_cast<int>(window_width), static_cast<int>(window_height), "App", nullptr, nullptr);
    assert(glfw_window && "Failed to create window");
    glfwSetKeyCallback(glfw_window, key_callback);

    vk_core::init(window_width, window_height, glfw_window, "/home/mica/Desktop/clean-start/examples/camera/data/json/vulkan_info.json");

    const renderer::InitInfo renderer_init_info {
        .window_width = window_width, 
        .window_height = window_height, 
        .frame_resource_count = frame_resource_count, 
        .app_config_file = "/home/mica/Desktop/clean-start/examples/camera/data/json/app_info.json", 
        .sortbin_config_file = "/home/mica/Desktop/clean-start/examples/camera/data/json/sortbin_info.json",
        .sortbin_reflection_config_file = "/home/mica/Desktop/clean-start/examples/camera/data/json/sortbin_reflection_info_simple.json",
        .shader_root_path = "/home/mica/Desktop/clean-start/examples/camera/data/shaders/spirv/",
    };

    renderer::init(renderer_init_info);

    const VkCommandPool vk_handle_cmd_pool = vk_core::create_command_pool(0x0);
    const VkCommandBuffer vk_handle_cmd_buff = vk_core::allocate_command_buffer(vk_handle_cmd_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    const VkFence vk_handle_swapchain_image_acquire_fence = vk_core::create_fence(0x0);

    Camera::proj_mat = glm::perspective(glm::radians(60.0f), 1.0f, 0.1f, 100.0f);
    Camera::view_mat[3] = glm::vec4(0.0f, 0.0f, -2.0f, 1.0f); 

    Entity entity = load_entity(vk_handle_cmd_pool, vk_handle_cmd_buff);

    renderer::add_renderable_to_sortbin(entity.renderable_id, entity.sortbin_id);

    uint32_t frame_idx = 0;
    float rotation_angle = 0.0f;

    while (!glfwWindowShouldClose(glfw_window))
    {
        glfwPollEvents();

        const uint32_t frame_resource_idx = frame_idx % frame_resource_count;

        vk_core::acquire_next_swapchain_image(VK_NULL_HANDLE, vk_handle_swapchain_image_acquire_fence);
        vk_core::wait_for_fences(1, &vk_handle_swapchain_image_acquire_fence, VK_TRUE, UINT64_MAX);
        vk_core::reset_fences(1, &vk_handle_swapchain_image_acquire_fence);

        if (Camera::proj_dirty)
        {
            renderer::update_uniform(renderer::BufferType::eFrame, "proj_mat", &(Camera::proj_mat[0][0]), frame_resource_idx);
            Camera::proj_dirty = false;
        }

        if (Camera::view_dirty)
        {
            Camera::update_xyz();
            renderer::update_uniform(renderer::BufferType::eFrame, "view_mat", &(Camera::view_mat[0][0]), frame_resource_idx);
        }

        glm::mat4x4 model_mat { 1.0 };
        model_mat = glm::rotate(model_mat, glm::radians(rotation_angle++), glm::vec3(0.0, 0.0, 1.0));
        renderer::update_uniform(renderer::BufferType::eDraw, "model_mat", &(model_mat[0][0]), entity.renderable_id);

        bool uploads_pending = false;
        renderer::flush_coherent_buffer_uploads(renderer::BufferType::eFrame, frame_resource_idx);
        uploads_pending = renderer::flush_buffer_uploads_to_staging(renderer::BufferType::eMaterial, frame_resource_idx);
        uploads_pending = renderer::flush_buffer_uploads_to_staging(renderer::BufferType::eDraw, frame_resource_idx);

        vk_core::reset_command_pool(vk_handle_cmd_pool);

        const VkCommandBufferBeginInfo cmd_buff_begin_info {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = 0x0,
            .pInheritanceInfo = nullptr,
        };

        vkBeginCommandBuffer(vk_handle_cmd_buff, &cmd_buff_begin_info);

        if (uploads_pending)
        {
            renderer::flush_staging_to_device(vk_handle_cmd_buff);

            // Pipeline barrier
        }
        
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

Entity load_entity(const VkCommandPool vk_handle_cmd_pool, const VkCommandBuffer vk_handle_cmd_buff)
{
    const float x = 0.707;
    const float tan_60 = glm::tan(glm::radians(60.0f));
    const float height = x * tan_60;
    const float height_down = x / tan_60;
    const float height_up = height - height_down;
    const float vertex_data[9] = {
         0.0, -height_up  , 0.0,
           x,  height_down, 0.0,
          -x,  height_down, 0.0
    };

    const uint32_t index_data[3] = { 0, 1, 2 };

    renderer::RenderableInitInfo renderable_init_info {
        .default_sortbin_id = 0,
        .vertex_stride = 12,
        .vertex_count = 3,
        .vertex_data = (uint8_t*)vertex_data,
        .index_count = 3,
        .index_data = (uint8_t*)index_data,
        .index_stride = 4,
        .material_data = {},
        .draw_data = {},
    };

    renderable_init_info.material_data.resize(12, 0);
    std::vector<float> material_data {{ 0.0, 1.0, 0.0 }};
    memcpy(renderable_init_info.material_data.data(), material_data.data(), 12);

    renderable_init_info.draw_data.resize(64, 0);
    glm::mat4x4 model_mat { 1.0 };
    memcpy(renderable_init_info.draw_data.data(), &(model_mat[0][0]), 64);

    const auto renderable_sortbin_id_pair = renderer::create_renderable(renderable_init_info, 0u);

    const VkCommandBufferBeginInfo cmd_buff_begin_info {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = 0x0,
        .pInheritanceInfo = nullptr,
    };

    vk_core::reset_command_pool(vk_handle_cmd_pool);
    
    vkBeginCommandBuffer(vk_handle_cmd_buff, &cmd_buff_begin_info);

    renderer::flush_staging_to_device(vk_handle_cmd_buff);

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

    Entity entity {
        .renderable_id = renderable_sortbin_id_pair.first,
        .sortbin_id = renderable_sortbin_id_pair.second,
    };

    return entity;
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