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

#include <vector>
#include <array>
#include <inttypes.h>

constexpr bool DEBUG = true;

#if 0

static void init_frame_desc_sets(const uint32_t frame_resource_count, const UniformBuffer* frame_uniform_buffer, const BufferPool_VariableBlock* material_data_buffer, const BufferPool_VariableBlock* draw_data_buffer, const std::vector<VkDescriptorSet>& vk_handle_desc_set_list)
{
    for (uint32_t i = 0; i < frame_resource_count; i++)
    {
        const VkDescriptorBufferInfo frame_ubo_desc_buffer_info = frame_uniform_buffer->get_descriptor_buffer_info(i);
        const VkDescriptorBufferInfo mat_ssbo_desc_buffer_info = material_data_buffer->get_descriptor_buffer_info(i);
        const VkDescriptorBufferInfo draw_ssbo_desc_buffer_info = draw_data_buffer->get_descriptor_buffer_info(i);

        const std::array<VkWriteDescriptorSet, 3> write_desc_set_list {{
            {
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
            },
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = vk_handle_desc_set_list[i],
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pImageInfo = nullptr,
                .pBufferInfo = &mat_ssbo_desc_buffer_info,
                .pTexelBufferView = nullptr,
            },
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = vk_handle_desc_set_list[i],
                .dstBinding = 2,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pImageInfo = nullptr,
                .pBufferInfo = &draw_ssbo_desc_buffer_info,
                .pTexelBufferView = nullptr,
            },
        }};

        vk_core::update_desc_sets(static_cast<uint32_t>(write_desc_set_list.size()), write_desc_set_list.data(), 0, nullptr);
    }
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


static bool queue_uploads_to_staging_buffer(GeometryBuffer* buffer, StagingBuffer* staging_buffer)
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
        LOG("Uniform queued upload to staging buffer (%lu, %lu)\n", upload_info.dst_offset, upload_info.size);
    }

    return !queued_uploads.empty();
}

#endif

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

// struct Material
// {
//     uint32_t ID;
//     uint32_t supported_sortbin_set_ID;
// };

// #include <memory>

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

#if 0


void record_render_pass(const std::string& render_pass_name, const VkCommandBuffer vk_handle_cmd_buff, const VkRect2D render_area, const uint32_t frame_resource_idx)
{
    if constexpr (DEBUG)
    {
        if (!global_state.render_pass_name_to_id_umap.contains(render_pass_name))
        {
            std::string available_render_pass_name_list = "";
            for (const auto& [name, id] : global_state.render_pass_name_to_id_umap)
            {
                available_render_pass_name_list += "\t" + name + "\n";
            }

            EXIT("ERROR: Render pass %s not found!\nAvailable RenderPasses are...\n%s",
                 render_pass_name.c_str(),
                 available_render_pass_name_list.c_str());
        }
    }

    const uint32_t render_pass_ID = global_state.render_pass_name_to_id_umap.at(render_pass_name);
    const RenderPass& render_pass = global_render_pass_list[render_pass_ID];

    const RenderPass::RecordInfo record_info {
        .frame_idx = frame_resource_idx,
        .vk_handle_cmd_buff = vk_handle_cmd_buff,
        .global_attachment_list = global_render_attachment_list,
        .global_sortbin_list = global_sortbin_list,
        .render_area = render_area,
        .vk_handle_index_buffer_list = { 
            global_state.geometry_buffer->get_vk_handle_buffer(),
            global_state.geometry_buffer->get_vk_handle_buffer(),
            global_state.geometry_buffer->get_vk_handle_buffer()
        },
        .vk_handle_global_desc_set = vk_handle_frame_desc_set_list[frame_resource_idx],
    };

    const VkBuffer vk_handle_geometry_buffer = global_state.geometry_buffer->get_vk_handle_buffer();
    const VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(vk_handle_cmd_buff, 0, 1, &vk_handle_geometry_buffer, &offset);

    render_pass.record(record_info);
}

VkImage get_attachment_image(const uint32_t attachment_id, const uint32_t frame_resource_idx)
{
    return global_render_attachment_list[attachment_id].vk_handle_image_list[frame_resource_idx];
}

uint16_t get_sortbin_ID(const std::string& sortbin_name)
{
    return global_state.sortbin_name_to_id_umap.at(sortbin_name);
}


std::pair<uint32_t, uint16_t> copy_renderable(const uint32_t copy_from_renderable_id) { return {}; }
bool renderable_supports_sortbin(const uint32_t renderable_id, const uint16_t sortbin_id) { return false; }
void delete_renderable(const uint32_t renderable_id) {}


void add_renderable_to_sortbin(const uint32_t renderable_id, const uint16_t sortbin_id)
{
    const Renderable& renderable = global_renderable_list[renderable_id];
    const Mesh& mesh = global_mesh_list[renderable.mesh_id];

    if (global_state.supported_sortbin_set_ID_list[renderable.default_sortbin_id] != global_state.supported_sortbin_set_ID_list[sortbin_id])
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
            global_sortbin_list[sortbin_id].draw_list_u32.push_back(draw_info);
            break;
        }
        case 2:
        {
            global_sortbin_list[sortbin_id].draw_list_u16.push_back(draw_info);
            break;
        }
        case 1:
        {
            global_sortbin_list[sortbin_id].draw_list_u8.push_back(draw_info);
            break;
        }
        default:
        {
            global_sortbin_list[sortbin_id].draw_list.push_back(draw_info);
            break;
        }
    };
}


uint32_t create_mesh(const MeshInitInfo& init_info)
{
    ASSERT(global_state.geometry_buffer != nullptr, "Global geometry buffer not initialized!\n");

    const int32_t vertex_offset = global_state.geometry_buffer->queue_upload(
        init_info.vertex_stride,
        init_info.vertex_count,
        std::vector<uint8_t>(init_info.vertex_data, init_info.vertex_data + init_info.vertex_count * init_info.vertex_stride));

    const uint32_t first_index = static_cast<uint32_t>(global_state.geometry_buffer->queue_upload(
        init_info.index_stride,
        init_info.index_count,
        std::vector<uint8_t>(init_info.index_data, init_info.index_data + init_info.index_count * init_info.index_stride)));

    const uint32_t mesh_ID = static_cast<uint32_t>(global_mesh_list.size());

    const Mesh mesh {
        .index_count = init_info.index_count,
        .vertex_count = init_info.vertex_count,
        .first_vertex = 0,
        .first_index = first_index,
        .vertex_offset = vertex_offset,
        .index_stride = init_info.index_stride,
    };

    global_mesh_list.push_back(mesh);

    queue_uploads_to_staging_buffer(global_state.geometry_buffer, global_state.staging_buffer);

    return mesh_ID;
}

uint32_t create_material(const MaterialInitInfo& init_info, const uint32_t frame_resource_idx)
{
    const auto iter = global_state.material_umap.find(init_info.name);

    if (iter != global_state.material_umap.end())
    {
        LOG("Attempting to create already existing material %s!\n", init_info.name.c_str());
        return iter->second.ID;
    }

    ASSERT(global_sortbin_list.size() > init_info.default_sortbin_ID, "Default sortbin ID out of range!\n");

    const SortBin& sortbin = global_sortbin_list[init_info.default_sortbin_ID];

    ASSERT(sortbin.material_data_block_size - sortbin.material_data_block_end_padding_size == init_info.material_data_size, "Material data size mismatch!\n");

    const uint32_t mat_ID = upload_block(global_state.material_data_buffer, sortbin.material_data_block_size, init_info.material_data_size, init_info.material_data_ptr);

    queue_uploads_to_staging_buffer(global_state.material_data_buffer, global_state.staging_buffer, frame_resource_idx);

    const Material mat {
        .ID = mat_ID,
        .supported_sortbin_set_ID = 0
    };

    global_state.material_umap.emplace_hint(iter, init_info.name, mat);

    return mat_ID;
}

std::pair<uint32_t, uint16_t> create_renderable(const RenderableInitInfo& init_info, const uint32_t frame_resource_idx)
{
    ASSERT(global_sortbin_list.size() > init_info.default_sortbin_id, "Default sortbin ID out of range!\n");

    const SortBin& sortbin = global_sortbin_list[init_info.default_sortbin_id];

    ASSERT(sortbin.draw_data_block_size - sortbin.draw_data_block_end_padding_size == init_info.draw_data_size + sizeof(uint32_t), "Draw data size mismatch!\n");

    const uint32_t draw_ID = upload_block(global_state.draw_data_buffer, sortbin.draw_data_block_size, init_info.draw_data_size, init_info.draw_data_ptr, init_info.material_ID);
    const uint32_t renderable_ID = static_cast<uint32_t>(global_renderable_list.size());

    const Renderable renderable {
        .mesh_id = init_info.mesh_ID,
        .material_id = init_info.material_ID,
        .draw_id = draw_ID,
        .default_sortbin_id = init_info.default_sortbin_id,
        .supported_sortbin_set_id = 0,
    };

    global_renderable_list.push_back(renderable);

    queue_uploads_to_staging_buffer(global_state.draw_data_buffer, global_state.staging_buffer, frame_resource_idx);

    return {renderable_ID, renderable.default_sortbin_id};
}




void flush_coherent_buffer_uploads(const BufferType buffer_type, const uint32_t frame_resource_idx)
{
    switch (buffer_type)
    {
        case BufferType::eFrame:
        {
            global_state.frame_uniform_buffer->flush_updates(frame_resource_idx);
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
            has_uploads = queue_uploads_to_staging_buffer(global_state.geometry_buffer, global_state.staging_buffer);
            break;
        }
        case BufferType::eMaterial:
        {
            has_uploads = queue_uploads_to_staging_buffer(global_state.material_data_buffer, global_state.staging_buffer, frame_resource_idx);
            break;
        }
        case BufferType::eDraw:
        {
            has_uploads = queue_uploads_to_staging_buffer(global_state.draw_data_buffer, global_state.staging_buffer, frame_resource_idx);
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
    global_state.staging_buffer->flush(vk_handle_cmd_buff);
}

void update_uniform(const BufferType buffer_type, const std::string& uniform_name, const void* const value, const uint32_t data_id)
{
    switch (buffer_type)
    {
        case BufferType::eFrame:
        {
            global_state.frame_uniform_buffer->update_member(uniform_name, value);
            break;
        }
        case BufferType::eDraw:
        {
            ASSERT(global_renderable_list.size() > data_id, "Renderable ID out of range!\n");
            const Renderable& renderable = global_renderable_list[data_id]; 

            ASSERT(global_sortbin_list.size() > renderable.default_sortbin_id, "Renderable ID out of range!\n");
            const SortBin& sortbin = global_sortbin_list[renderable.default_sortbin_id];

            const auto it = sortbin.descriptor_variable_draw_umap.find(uniform_name);
            ASSERT(it != sortbin.descriptor_variable_draw_umap.end(), "Member name `%s` not found in sortbin descriptor variable list!\n", uniform_name.c_str());

            void* draw_data_ptr = global_state.draw_data_buffer->get_writable_block(sortbin.draw_data_block_size, renderable.draw_id);
            memcpy(static_cast<uint8_t*>(draw_data_ptr) + it->second.offset, value, it->second.size);

            // LOG("Updating uniform member (%u, %s, %u, %u)\n", data_id, uniform_name.c_str(), it->second.offset, it->second.size);

            break;
        }
        case BufferType::eSortbin:
        case BufferType::eMaterial:
        default:
        {
            LOG("Warning - Buffer type %d does not support uniform updates!\n", (int)buffer_type);
            break;
        }
    };
}

#endif

}; // renderer