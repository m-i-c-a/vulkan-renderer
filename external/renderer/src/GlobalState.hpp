#ifndef RENDERER_GLOBAL_STATE_HPP
#define RENDERER_GLOBAL_STATE_HPP

#include "RenderPass.hpp"
#include "internal/pod/Material.hpp"
#include "internal/pod/Mesh.hpp"
#include "internal/pod/Renderable.hpp"

#include <inttypes.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

class UniformBuffer;
class GeometryBuffer;
class BufferPool_VariableBlock;
class StagingBuffer;

struct RendererState
{
    const std::unordered_map<std::string, uint8_t>  name_id_lut_render_attachment;
    const std::unordered_map<std::string, uint16_t> name_id_lut_render_pass;
    const std::unordered_map<std::string, uint16_t> name_id_lut_sort_bin;

    const std::vector<RenderPass::Attachment> render_attachment_vec;
    const std::vector<RenderPass> render_pass_vec;

    const VkDescriptorPool vk_handle_frame_desc_pool;
    const VkDescriptorSetLayout vk_handle_frame_desc_set_layout;
    const std::vector<VkDescriptorSet> vk_handle_frame_desc_set_vec;
    const std::vector<uint16_t> compatible_sortbin_ID_lut;

    std::vector<SortBin> sort_bin_vec;

    std::unordered_map<std::string, uint32_t> name_id_lut_material;

    std::vector<Renderable> renderable_vec;
    std::vector<Mesh> mesh_vec; 
    std::vector<Material> material_vec;

    std::unique_ptr<UniformBuffer>            frame_general_ubo;
    std::unique_ptr<UniformBuffer>            frame_fwd_light_ubo;
    std::unique_ptr<GeometryBuffer>           geometry_buffer;
    std::unique_ptr<BufferPool_VariableBlock> material_data_buffer;
    std::unique_ptr<BufferPool_VariableBlock> draw_data_buffer;
    std::unique_ptr<StagingBuffer>            staging_buffer;

    struct CreateInfo
    {
        const char* const refl_file_frame_desc_set_def;
        const char* const refl_file_sortbin_mat_draw_def;
        const char* const file_sortbin_pipeline_state;
        const char* const file_app_state;
        const char* const path_shader_root;

        uint8_t frame_resource_count;
        uint32_t window_x_dim;
        uint32_t window_y_dim;
    };

    explicit RendererState(const CreateInfo& create_info);
    ~RendererState();
};

#endif // RENDERER_GLOBAL_STATE_HPP