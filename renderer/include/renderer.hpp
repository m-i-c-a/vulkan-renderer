#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "Renderable.hpp"

#include <vulkan/vulkan.h>

#include <stdint.h>

// Could all be spec consts built into program

// Every sortbin has unique shader (sortbins do NOT share shaders)
// Enforcing this uniqueness allows us to...
// -- deduce per material data type based on reflection && JSON data 
// -- deduce per draw data type based on reflection
// -- deduce vertex attrib based on reflection && JSON data

// The Unknown / app-specific sortbin state
// -- renderpass 
// -- color attachment format list
// -- depth attachment format
// -- color attachment blend state list

namespace renderer
{
    struct InitInfo
    {
        const uint32_t window_width; 
        const uint32_t window_height; 
        const uint32_t frame_resource_count; 

        const char* const app_config_file; 
        const char* const sortbin_config_file;
        const char* const sortbin_reflection_config_file;
        const char* const shader_root_path;
    };

    struct MeshInitInfo
    {
        uint32_t sortbin_id;
        uint32_t vertex_stride;
        uint32_t vertex_count;
        const uint8_t* vertex_data;
        uint32_t index_count;
        const uint8_t* index_data;
        uint32_t index_stride;
        std::vector<uint8_t> material_data;
        const std::vector<uint8_t> draw_data;
    };

    struct Renderable
    {
        const uint32_t mesh_id = 0u;
        const uint32_t draw_id = 0u;

        float bsphere_origin[3] = { 0.0f, 0.0f, 0.0f };
        float bsphere_radius = 0.0f;
    };

    void init(const InitInfo& init_info);

    void terminate();

    void add_renderable_to_sortbin(const Renderable& renderable);

    void clear_renderables_in_sortbin();

    void record_render_pass(const uint32_t renderpass_id, const VkCommandBuffer vk_handle_cmd_buff, const VkRect2D render_area, const uint32_t frame_resource_idx);

    VkImage get_attachment_image(const uint32_t attachment_id, const uint32_t frame_resource_idx);

    Renderable load_mesh(const MeshInitInfo& mesh_init_info);

    void flush_geometry_uploads(const VkCommandBuffer vk_handle_cmd_buff);
    void flush_draw_data_uploads(const VkCommandBuffer vk_handle_cmd_buff);

}; // renderer

#endif // RENDERER_HPP