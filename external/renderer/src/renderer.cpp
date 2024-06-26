#include "renderer.hpp"

// #include "UploadInfo.hpp"
// #include "Renderable.hpp"

// #include "common/defines.hpp"
// #include "common/loader_renderpass.hpp"
// #include "common/loader_sortbin.hpp"
// #include "common/RenderPass.hpp"
// #include "common/GeometryBuffer.hpp"
// #include "common/StagingBuffer.hpp"
// #include "common/BufferPool_VariableBlock.hpp"
// #include "common/UniformBuffer.hpp"

// #include "common/json_structures.hpp"

// #include "vk_core.hpp"
// #include "json.hpp"

#include "GlobalState.hpp"
#include "internal/misc/logger.hpp"
#include "internal/buffers/BufferPool_VariableBlock.hpp"
#include "internal/buffers/GeometryBuffer.hpp"
#include "internal/buffers/StagingBuffer.hpp"
#include "internal/buffers/UniformBuffer.hpp"

#include <vector>
#include <array>
#include <inttypes.h>

constexpr bool DEBUG = true;

static bool queue_uploads_to_staging_buffer(GeometryBuffer* const buffer, StagingBuffer* const staging_buffer)
{
    const std::vector<UploadInfo> queued_uploads = buffer->get_queued_uploads();

    for (const UploadInfo& upload_info : queued_uploads)
    {
        const void* const data_ptr = upload_info.data_pointer ? upload_info.data_pointer : (void*)upload_info.data_vector.data();
        staging_buffer->queue_upload(buffer->get_vk_handle_buffer(), upload_info.dst_offset, upload_info.size, data_ptr);
        // LOG("Geometry queued upload to staging buffer (%lu, %lu)\n", upload_info.dst_offset, upload_info.size);
    }

    return !queued_uploads.empty();
}

static bool queue_uploads_to_staging_buffer(BufferPool_VariableBlock* buffer, StagingBuffer* staging_buffer, const uint32_t frame_resource_idx)
{
    const std::vector<UploadInfo> queued_uploads = buffer->get_queued_uploads(frame_resource_idx);

    for (const UploadInfo& upload_info : queued_uploads)
    {
        const void* const data_ptr = upload_info.data_pointer ? upload_info.data_pointer : (void*)upload_info.data_vector.data();
        staging_buffer->queue_upload(buffer->get_vk_handle_buffer(), upload_info.dst_offset, upload_info.size, data_ptr);
        // LOG("Uniform queued upload to staging buffer (%lu, %lu)\n", upload_info.dst_offset, upload_info.size);
    }

    return !queued_uploads.empty();
}

static uint32_t upload_block(BufferPool_VariableBlock* buffer, const uint32_t block_size, const std::vector<uint8_t>& data, const uint32_t mat_ID = UINT32_MAX)
{
    uint32_t block_ID = buffer->acquire_block(block_size); 
    void* block_ptr = buffer->get_writable_block(block_size, block_ID);

    if ( mat_ID == UINT32_MAX )
    {
        memcpy(block_ptr, data.data(), block_size);
    }
    else
    {
        memcpy(static_cast<uint8_t*>(block_ptr), data.data(), block_size - sizeof(uint32_t));
        memcpy(static_cast<uint8_t*>(block_ptr) + (block_size - sizeof(uint32_t)), &mat_ID, sizeof(uint32_t));
    }

    return block_ID;
}

static uint32_t upload_block(BufferPool_VariableBlock* buffer, const uint32_t block_size, const uint32_t data_size, const uint8_t* data_ptr, const uint32_t mat_ID = UINT32_MAX)
{
    uint32_t block_ID = buffer->acquire_block(block_size); 
    void* block_ptr = buffer->get_writable_block(block_size, block_ID);

    if ( mat_ID == UINT32_MAX )
    {
        memcpy(block_ptr, data_ptr, block_size);
    }
    else
    {
        memcpy(static_cast<uint8_t*>(block_ptr), data_ptr, data_size);
        memcpy(static_cast<uint8_t*>(block_ptr) + data_size, &mat_ID, sizeof(uint32_t));
    }

    return block_ID;
}


namespace renderer
{
// There is only one descriptor set.
// All sortbins MUST match the template for set 0 below.
// Sortbin json files only need to have member descriptions for block in the template.

// Set 0
// -- Frame UBO
// -- -- For runtime frame info
// -- -- Located in shared.glsl
// -- Sortbin SSBO
// -- -- For runtime sortbin info
// -- -- Indexed by push_consts.sortbin_ID
// -- -- Located in <sortbin> shader file
// -- Material SSBO
// -- -- For model matertial info
// -- -- Indexed by draw_data.mat_ID
// -- -- Located in <sortbin> shader file
// -- Draw SSBO
// -- -- For model material info
// -- -- Indexed by instance ID
// -- -- Located in <sortbin> shader file

// A Renderable is the combination of a mesh_id, material_id, draw_id, and a supported_sortbin_set_idx. It is the summation
// if all the data needed to render an object. 

// A Entity is the combination of a renderable_id and a sortbin_id. We distinguish an Entitiy from a Renderable so that
// sortbins can be changed at runtime if needed. A Renderable can be added to various sortbins, as long as the 
// vertex input / mateiral / draw states match. The separation allows a toggling to wireframe or different shaders at runtime.

// Startup
// -- Group sortbins into sets based on vertex input / material / draw states

// Load a GLTF model (contains sortbin_id)
// -- Create renderable_id
// -- Create mesh_id
// -- Create material_id
// -- Create draw_id
// -- Add supported_sortbin_set_id to renderable

std::unique_ptr<RendererState> global_state = nullptr;

void init(const InitInfo& init_info)
{
    const RendererState::CreateInfo renderer_internal_create_info {
        .refl_file_frame_desc_set_def = init_info.refl_file_frame_desc_set_def,
        .refl_file_sortbin_mat_draw_def = init_info.refl_file_sortbin_mat_draw_def,
        .file_sortbin_pipeline_state = init_info.file_sortbin_pipeline_state,
        .file_app_state = init_info.file_app_state,
        .path_shader_root = init_info.path_shader_root,
        .frame_resource_count = init_info.frame_resource_count,
        .window_x_dim = init_info.window_width,
        .window_y_dim = init_info.window_height
    };

    global_state = std::make_unique<RendererState>(renderer_internal_create_info);
}

void terminate()
{
    global_state.reset();
}

uint32_t create_mesh(const MeshInitInfo& init_info)
{
    const int32_t vertex_offset = global_state->geometry_buffer->queue_upload(
        init_info.vertex_stride,
        init_info.vertex_count,
        std::vector<uint8_t>(init_info.vertex_data, init_info.vertex_data + init_info.vertex_count * init_info.vertex_stride));

    const uint32_t first_index = static_cast<uint32_t>(global_state->geometry_buffer->queue_upload(
        init_info.index_stride,
        init_info.index_count,
        std::vector<uint8_t>(init_info.index_data, init_info.index_data + init_info.index_count * init_info.index_stride)));

    const uint32_t mesh_ID = static_cast<uint32_t>(global_state->mesh_vec.size());

    const Mesh mesh {
        .index_count = init_info.index_count,
        .vertex_count = init_info.vertex_count,
        .first_vertex = 0,
        .first_index = first_index,
        .vertex_offset = vertex_offset,
        .index_stride = init_info.index_stride,
    };

    global_state->mesh_vec.push_back(mesh);

    queue_uploads_to_staging_buffer(global_state->geometry_buffer.get(), global_state->staging_buffer.get());

    return mesh_ID;
}

uint32_t create_material(const MaterialInitInfo& init_info, const uint32_t frame_resource_idx)
{
    const auto iter = global_state->name_id_lut_material.find(init_info.name);

    if (iter != global_state->name_id_lut_material.end())
    {
        LOG("create_material - Attempting to create already existing material %s!\n", init_info.name.c_str());
        return iter->second;
    }

    const auto sort_bin_iter = global_state->name_id_lut_sort_bin.find(init_info.default_sort_bin_name);
    ASSERT(sort_bin_iter != global_state->name_id_lut_sort_bin.end(), "create_material - Default SortBin %s not found!\n", init_info.default_sort_bin_name.c_str());
    const uint16_t sort_bin_ID = sort_bin_iter->second;

    ASSERT(global_state->sort_bin_vec.size() > sort_bin_ID, "Default sortbin ID out of range!\n");
    const SortBin& sort_bin = global_state->sort_bin_vec[sort_bin_ID];

    ASSERT(sort_bin.material_data_block_size - sort_bin.material_data_block_end_padding_size == init_info.material_data_size, "Material data size mismatch!\n");
    const uint32_t mat_ID = upload_block(global_state->material_data_buffer.get(), sort_bin.material_data_block_size, init_info.material_data_size, init_info.material_data_ptr);

    queue_uploads_to_staging_buffer(global_state->material_data_buffer.get(), global_state->staging_buffer.get(), frame_resource_idx);

    const Material mat {
        .ID = mat_ID,
        .default_sort_bin_ID = sort_bin_ID
    };

    global_state->name_id_lut_material.emplace_hint(iter, init_info.name, mat_ID);
    global_state->material_vec.push_back(mat);

    return mat_ID;
}

std::pair<uint32_t, uint16_t> create_renderable(const RenderableInitInfo& init_info, const uint32_t frame_resource_idx)
{
    const auto sort_bin_lut_iter = global_state->name_id_lut_sort_bin.find(init_info.default_sort_bin_name);
    ASSERT(sort_bin_lut_iter != global_state->name_id_lut_sort_bin.end(), "create_renderable - Default SortBin %s not found!\n", init_info.default_sort_bin_name.c_str());
    const uint16_t sort_bin_ID = sort_bin_lut_iter->second;

    ASSERT(init_info.material_ID < global_state->material_vec.size(), "create_renderable - Material ID %u out of range!\n", init_info.material_ID);
    const Material& material = global_state->material_vec[init_info.material_ID];
    const SortBin& material_sort_bin = global_state->sort_bin_vec[material.default_sort_bin_ID]; 
    const SortBin& sort_bin = global_state->sort_bin_vec[sort_bin_ID];
    ASSERT(sort_bin.compatible_sort_bin_set_ID == material_sort_bin.compatible_sort_bin_set_ID, "create_renderable - SortBin %s not supported by Material %u!\n", init_info.default_sort_bin_name.c_str(), init_info.material_ID);
    ASSERT(sort_bin.draw_data_block_size - sort_bin.draw_data_block_end_padding_size == init_info.draw_data_size + sizeof(uint32_t), "Draw data size mismatch!\n");

    const uint32_t draw_ID = upload_block(global_state->draw_data_buffer.get(), sort_bin.draw_data_block_size, init_info.draw_data_size, init_info.draw_data_ptr, init_info.material_ID);
    const uint32_t renderable_ID = static_cast<uint32_t>(global_state->renderable_vec.size());

    const Renderable renderable {
        .mesh_id = init_info.mesh_ID,
        .material_id = init_info.material_ID,
        .draw_id = draw_ID,
        .default_sortbin_id = sort_bin_ID,
    };

    global_state->renderable_vec.push_back(renderable);

    queue_uploads_to_staging_buffer(global_state->draw_data_buffer.get(), global_state->staging_buffer.get(), frame_resource_idx);

    return {renderable_ID, renderable.default_sortbin_id};
}

void update_uniform(const BufferType buffer_type, const std::string& uniform_name, const void* const value, const uint32_t data_id)
{
    switch (buffer_type)
    {
        case BufferType::eFrame:
        {
            global_state->frame_general_ubo->update_member(uniform_name, value);
            break;
        }
        case BufferType::eMaterial:
        {
            ASSERT(global_state->material_vec.size() > data_id, "update_uniform - Material ID out of range!\n");
            const Material& material = global_state->material_vec[data_id];
            const SortBin& sort_bin = global_state->sort_bin_vec[material.default_sort_bin_ID];

            const auto it = sort_bin.descriptor_variable_material_umap.find(uniform_name);
            ASSERT(it != sort_bin.descriptor_variable_material_umap.end(), "Member name `%s` not found in sortbin descriptor variable list!\n", uniform_name.c_str());

            void* mat_data_ptr = global_state->material_data_buffer->get_writable_block(sort_bin.material_data_block_size, material.ID);
            memcpy(static_cast<uint8_t*>(mat_data_ptr) + it->second.offset, value, it->second.size);
            // LOG("Updating material uniform member (%u, %s, %u, %u)\n", data_id, uniform_name.c_str(), it->second.offset, it->second.size);
            break;
        }
        case BufferType::eDraw:
        {
            ASSERT(global_state->renderable_vec.size() > data_id, "update_uniform - Renderable ID out of range!\n");
            const Renderable& renderable = global_state->renderable_vec[data_id];
            const SortBin& sort_bin = global_state->sort_bin_vec[renderable.default_sortbin_id];

            const auto it = sort_bin.descriptor_variable_draw_umap.find(uniform_name);
            ASSERT(it != sort_bin.descriptor_variable_draw_umap.end(), "Member name `%s` not found in sortbin descriptor variable list!\n", uniform_name.c_str());

            void* draw_data_ptr = global_state->draw_data_buffer->get_writable_block(sort_bin.draw_data_block_size, renderable.draw_id);
            memcpy(static_cast<uint8_t*>(draw_data_ptr) + it->second.offset, value, it->second.size);
            // LOG("Updating draw uniform member (%u, %s, %u, %u)\n", data_id, uniform_name.c_str(), it->second.offset, it->second.size);
            break;
        }
        case BufferType::eSortbin:
        default:
        {
            LOG("Warning - Buffer type %d does not support uniform updates!\n", (int)buffer_type);
            break;
        }
    };
}

void flush_coherent_buffer_uploads(const BufferType buffer_type, const uint32_t frame_resource_idx)
{
    switch (buffer_type)
    {
        case BufferType::eFrame:
        {
            global_state->frame_general_ubo->flush_updates(frame_resource_idx);
            break;
        }
        default:
        {
            LOG("Warning - Buffer type %d is not coherent!\n", (int)buffer_type);
            break;
        }
    };
}

bool flush_buffer_uploads_to_staging(const BufferType buffer_type, const uint32_t frame_resource_idx)
{
    bool has_uploads = false;
    switch (buffer_type)
    {
        case BufferType::eGeometry:
        {
            has_uploads = queue_uploads_to_staging_buffer(global_state->geometry_buffer.get(), global_state->staging_buffer.get());
            break;
        }
        case BufferType::eMaterial:
        {
            has_uploads = queue_uploads_to_staging_buffer(global_state->material_data_buffer.get(), global_state->staging_buffer.get(), frame_resource_idx);
            break;
        }
        case BufferType::eDraw:
        {
            has_uploads = queue_uploads_to_staging_buffer(global_state->draw_data_buffer.get(), global_state->staging_buffer.get(), frame_resource_idx);
            break;
        }
        case BufferType::eSortbin:
        default:
        {
            LOG("Warning - Buffer type %d does not support staging buffer upload!\n", (int)buffer_type);
            break;
        }
    }

    return has_uploads;
}

void flush_staging_to_device(const VkCommandBuffer vk_handle_cmd_buff)
{
    global_state->staging_buffer->flush(vk_handle_cmd_buff);
}

void add_renderable_to_sortbin(const uint32_t renderable_id, const uint16_t sortbin_id)
{
    const Renderable& renderable = global_state->renderable_vec[renderable_id];
    const Mesh& mesh = global_state->mesh_vec[renderable.mesh_id];

    if (global_state->compatible_sortbin_ID_lut[renderable.default_sortbin_id] != global_state->compatible_sortbin_ID_lut[sortbin_id])
    {
        LOG("Sortbin %d not supported by renderable %d!\n", sortbin_id, renderable_id);
        return;
    }

    const DrawInfo draw_info {
        .index_count = mesh.index_count,
        .vertex_count = mesh.vertex_count,
        .instance_count = 1,
        .first_index = mesh.first_index,
        .first_vertex = mesh.first_vertex,
        .vertex_offset = mesh.vertex_offset,
        .first_instance = renderable.draw_id
    };

    switch (mesh.index_stride)
    {
        case 4:
        {
            global_state->sort_bin_vec[sortbin_id].draw_list_u32.push_back(draw_info);
            break;
        }
        case 2:
        {
            global_state->sort_bin_vec[sortbin_id].draw_list_u16.push_back(draw_info);
            break;
        }
        case 1:
        {
            global_state->sort_bin_vec[sortbin_id].draw_list_u8.push_back(draw_info);
            break;
        }
        default:
        {
            global_state->sort_bin_vec[sortbin_id].draw_list.push_back(draw_info);
            break;
        }
    };
}

void record_render_pass(const std::string& render_pass_name, const VkCommandBuffer vk_handle_cmd_buff, const VkRect2D render_area, const uint32_t frame_resource_idx)
{
    if constexpr (DEBUG)
    {
        if (!global_state->name_id_lut_render_pass.contains(render_pass_name))
        {
            std::string available_render_pass_name_list = "";
            for (const auto& [name, id] : global_state->name_id_lut_render_pass)
            {
                available_render_pass_name_list += "\t" + name + "\n";
            }

            EXIT("ERROR: Render pass %s not found!\nAvailable RenderPasses are...\n%s",
                 render_pass_name.c_str(),
                 available_render_pass_name_list.c_str());
        }
    }

    const uint32_t render_pass_ID = global_state->name_id_lut_render_pass.at(render_pass_name);
    const RenderPass& render_pass = global_state->render_pass_vec[render_pass_ID];

    const RenderPass::RecordInfo record_info {
        .frame_idx = frame_resource_idx,
        .vk_handle_cmd_buff = vk_handle_cmd_buff,
        .global_attachment_list = global_state->render_attachment_vec,
        .global_sortbin_list = global_state->sort_bin_vec,
        .render_area = render_area,
        .vk_handle_index_buffer_list = { 
            global_state->geometry_buffer->get_vk_handle_buffer(),
            global_state->geometry_buffer->get_vk_handle_buffer(),
            global_state->geometry_buffer->get_vk_handle_buffer()
        },
        .vk_handle_global_desc_set = global_state->vk_handle_frame_desc_set_vec[frame_resource_idx],
    };

    const VkBuffer vk_handle_geometry_buffer = global_state->geometry_buffer->get_vk_handle_buffer();
    const VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(vk_handle_cmd_buff, 0, 1, &vk_handle_geometry_buffer, &offset);

    render_pass.record(record_info);
}

VkImage get_attachment_image(const uint32_t attachment_id, const uint32_t frame_resource_idx)
{
    return global_state->render_attachment_vec[attachment_id].vk_handle_image_list[frame_resource_idx];
}

uint16_t get_sortbin_ID(const std::string& sortbin_name)
{
    return global_state->name_id_lut_sort_bin.at(sortbin_name);
}

}; // renderer