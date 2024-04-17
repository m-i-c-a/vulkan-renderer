#include "vk_core.hpp"
#include "renderer.hpp"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <assert.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <cstring> 

constexpr uint32_t window_width = 800u;
constexpr uint32_t window_height = 800u;
constexpr uint32_t frame_resource_count = 1u;

static std::pair<std::vector<float>, std::vector<uint32_t>> generate_triangle_data();
static void flush_uploads(const VkCommandBuffer vk_handle_cmd_buff, const VkCommandPool vk_handle_cmd_pool, const uint32_t frame_resource_idx, const bool first_frame);
static void blit(const uint32_t frame_resource_idx, const VkCommandBuffer vk_handle_cmd_buff);

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow *glfw_window = glfwCreateWindow(static_cast<int>(window_width), static_cast<int>(window_height), "App", nullptr, nullptr);
    assert(glfw_window && "Failed to create window");

    vk_core::init(window_width, window_height, glfw_window, "/home/mica/Desktop/clean-start/examples/00_triangle/data/json/vulkan_state.json");

    const renderer::InitInfo renderer_init_info {
        .window_width = window_width, 
        .window_height = window_height, 
        .frame_resource_count = frame_resource_count, 
        .refl_file_frame_desc_set_def = "/home/mica/Desktop/clean-start/examples/00_triangle/data/json/reflection/frame_desc_set_reflection.json",
        .refl_file_sortbin_mat_draw_def = "/home/mica/Desktop/clean-start/examples/00_triangle/data/json/reflection/sortbin_reflection.json",
        .file_sortbin_pipeline_state = "/home/mica/Desktop/clean-start/examples/00_triangle/data/json/sortbin_pipeline_state.json",
        .file_app_state = "/home/mica/Desktop/clean-start/examples/00_triangle/data/json/app_state.json", 
        .path_shader_root = "/home/mica/Desktop/clean-start/examples/00_triangle/data/shaders/spirv/",
    };

    renderer::init(renderer_init_info);

    glm::mat4x4 proj_mat { 1.0 };
    glm::mat4x4 view_mat { 1.0 };

    renderer::update_uniform(renderer::BufferType::eFrame, "proj_mat", &(proj_mat[0][0]));
    renderer::update_uniform(renderer::BufferType::eFrame, "view_mat", &(view_mat[0][0]));

    uint32_t material_ID = 0u;
    uint32_t renderable_ID = 0u;
    uint16_t sort_bin_ID = 0u;

    {
        const auto [vertex_data, index_data] = generate_triangle_data();

        const renderer::MeshInitInfo mesh_init_info {
            .vertex_stride = 12,
            .vertex_count = 3,
            .vertex_data = (uint8_t*)(vertex_data.data()),
            .index_count = 3,
            .index_data = (uint8_t*)(index_data.data()),
            .index_stride = 4,
        };

        const uint32_t mesh_ID = renderer::create_mesh(mesh_init_info);

        std::vector<float> material_data { 0.0, 0.0, 0.0 };

        const renderer::MaterialInitInfo material_init_info {
            .name = "triangle_material",
            .material_data_ptr = (uint8_t*)material_data.data(),
            .material_data_size = static_cast<uint32_t>(material_data.size() * sizeof(float)),
            .default_sort_bin_name = "default_v3_pos",
        };

        const uint32_t m_ID = renderer::create_material(material_init_info, 0u);    

        std::vector<float> draw_data(16, 0);
        glm::mat4x4 model_mat { 1.0 };
        memcpy(draw_data.data(), &(model_mat[0][0]), 64);

        const renderer::RenderableInitInfo renderable_init_info {
            .mesh_ID = mesh_ID,
            .material_ID = m_ID,
            .draw_data_ptr = (uint8_t*)draw_data.data(),
            .draw_data_size = static_cast<uint32_t>(draw_data.size() * sizeof(float)),
            .default_sort_bin_name = "default_v3_pos",
        };

        const auto [r_ID, s_ID] = renderer::create_renderable(renderable_init_info, 0u);

        material_ID = m_ID;
        renderable_ID = r_ID;
        sort_bin_ID = s_ID;

        renderer::add_renderable_to_sortbin(renderable_ID, sort_bin_ID);
    }

    const VkCommandPool vk_handle_cmd_pool = vk_core::create_command_pool(0x0);
    const VkCommandBuffer vk_handle_cmd_buff = vk_core::allocate_command_buffer(vk_handle_cmd_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    const VkFence vk_handle_swapchain_image_acquire_fence = vk_core::create_fence(0x0);

    uint64_t frame_idx = 0;
    float rotation_angle = 0.0f;

    while (!glfwWindowShouldClose(glfw_window))
    {
        glfwPollEvents();

        uint32_t frame_resource_idx = frame_idx % frame_resource_count;

        glm::mat4x4 model_mat { 1.0 };
        std::vector<float> material_data { 0.0f, glm::cos(glm::radians(rotation_angle)), glm::sin(glm::radians(rotation_angle)) };
        model_mat = glm::rotate(model_mat, glm::radians(rotation_angle++), glm::vec3(0.0, 0.0, 1.0));
        renderer::update_uniform(renderer::BufferType::eMaterial, "color", material_data.data(), material_ID); 
        renderer::update_uniform(renderer::BufferType::eDraw, "model_mat", &(model_mat[0][0]), renderable_ID); 

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

            flush_uploads(vk_handle_cmd_buff, vk_handle_cmd_pool, frame_resource_idx, frame_idx == 0);

            renderer::record_render_pass("default", vk_handle_cmd_buff, {0, 0, window_width, window_height}, frame_resource_idx);

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

static std::pair<std::vector<float>, std::vector<uint32_t>> generate_triangle_data()
{
    const float x = 0.707;
    const float tan_60 = glm::tan(glm::radians(60.0f));
    const float height = x * tan_60;
    const float height_down = x / tan_60;
    const float height_up = height - height_down;

    const std::vector<float> vertex_data {
         0.0, -height_up  , 0.0,
           x,  height_down, 0.0,
          -x,  height_down, 0.0
    };

    const std::vector<uint32_t> index_data { 0, 1, 2 };

    return { vertex_data, index_data };
}

static void flush_uploads(const VkCommandBuffer vk_handle_cmd_buff, const VkCommandPool vk_handle_cmd_pool, const uint32_t frame_resource_idx, const bool first_frame)
{
    bool uploads_pending = false;
    renderer::flush_coherent_buffer_uploads(renderer::BufferType::eFrame, frame_resource_idx);
    uploads_pending |= renderer::flush_buffer_uploads_to_staging(renderer::BufferType::eGeometry, frame_resource_idx);
    uploads_pending |= renderer::flush_buffer_uploads_to_staging(renderer::BufferType::eMaterial, frame_resource_idx);
    uploads_pending |= renderer::flush_buffer_uploads_to_staging(renderer::BufferType::eDraw, frame_resource_idx);

    if (first_frame || uploads_pending)
    {
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

        vk_core::reset_command_pool(vk_handle_cmd_pool);

        const VkCommandBufferBeginInfo cmd_buff_begin_info {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = 0x0,
            .pInheritanceInfo = nullptr,
        };

        vkBeginCommandBuffer(vk_handle_cmd_buff, &cmd_buff_begin_info);
    }
}

static void blit(const uint32_t frame_resource_idx, const VkCommandBuffer vk_handle_cmd_buff)
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