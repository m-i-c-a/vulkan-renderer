#ifndef RENDERER_HPP
#define RENDERER_HPP

// #include "Renderable.hpp"

#include <vulkan/vulkan.h>

#include <stdint.h>
#include <string>

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
        const uint8_t frame_resource_count; 

        const char* const refl_file_frame_desc_set_def;
        const char* const refl_file_sortbin_mat_draw_def;
        const char* const file_sortbin_pipeline_state;
        const char* const file_app_state;
        const char* const path_shader_root;
    };

#if 0
    struct MeshInitInfo
    {
        uint32_t             vertex_stride;
        uint32_t             vertex_count;
        const uint8_t*       vertex_data;
        uint32_t             index_count;
        const uint8_t*       index_data;
        uint32_t             index_stride;
    };

    struct MaterialInitInfo
    {
        std::string    name;
        const uint8_t* material_data_ptr;
        uint32_t       material_data_size;
        uint16_t       default_sortbin_ID;
    };

    struct RenderableInitInfo
    {
        uint32_t       mesh_ID;
        uint32_t       material_ID;
        const uint8_t* draw_data_ptr;
        uint32_t       draw_data_size;
        uint16_t       default_sortbin_id;
    };

    enum class BufferType
    {
        eGeometry,
        eFrame,
        eSortbin,
        eMaterial,
        eDraw,
    };
#endif
#if 0
    uint16_t get_sortbin_ID(const std::string& sortbin_name);

    uint32_t get_material_ID(const std::string& material_name);

    uint32_t create_mesh(const MeshInitInfo& init_info);
    uint32_t create_material(const MaterialInitInfo& init_info, const uint32_t frame_resource_idx);
    std::pair<uint32_t, uint16_t> create_renderable(const RenderableInitInfo& init_info, const uint32_t frame_resource_idx);

    void add_renderable_to_sortbin(const uint32_t renderable_id, const uint16_t sortbin_id);

    void flush_coherent_buffer_uploads(const BufferType buffer_type, const uint32_t frame_resource_idx);
    bool flush_buffer_uploads_to_staging(const BufferType buffer_type, const uint32_t frame_resource_idx);
    void flush_staging_to_device(const VkCommandBuffer vk_handle_cmd_buff);

    void update_uniform(const BufferType buffer_type, const std::string& uniform_name, const void* const value, const uint32_t data_id = UINT32_MAX);
#endif

    void init(const InitInfo& init_info);
    void terminate();

#if 0

    void clear_renderables_in_sortbin();

    void update_frame_uniform(const std::string& uniform_name, const void* const value);
    void update_material_uniform(const uint32_t renderable_id, const std::string& member_name, const void* const data);
    void update_draw_uniform(const uint32_t renderable_id, const std::string& member_name, const void* const data);

    void record_render_pass(const std::string& render_pass_name, const VkCommandBuffer vk_handle_cmd_buff, const VkRect2D render_area, const uint32_t frame_resource_idx);

    void flush_staging_buffer(const VkCommandBuffer vk_handle_cmd_buff);
    void flush_frame_data_uploads(const uint32_t frame_resource_idx);

    VkImage get_attachment_image(const uint32_t attachment_id, const uint32_t frame_resource_idx);
#endif
}; // renderer

#endif // RENDERER_HPP