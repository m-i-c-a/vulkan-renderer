#include "renderer.hpp"

#include "UploadInfo.hpp"
#include "Renderable.hpp"

#include "common/defines.hpp"
#include "common/loader_renderpass.hpp"
#include "common/loader_sortbin.hpp"
#include "common/RenderPass.hpp"
#include "common/GeometryBuffer.hpp"
#include "common/StagingBuffer.hpp"
#include "common/BufferPool_VariableBlock.hpp"
#include "common/UniformBuffer.hpp"

#include "vk_core.hpp"
#include "json.hpp"

#include <vector>
#include <array>
#include <inttypes.h>

static std::vector<RenderPass::Attachment> create_render_attachments(const renderer::InitInfo& init_info)
{
    const auto json_data = read_json_file(init_info.app_config_file);
    const JSONInfo_RenderAttachment render_attachment_info = json_data.at("render-attachments").get<JSONInfo_RenderAttachment>();

    std::vector<RenderPass::Attachment> render_attachment_list;
    std::vector<VkImageMemoryBarrier> image_memory_barriers;

    for (const JSONInfo_RenderAttachment::ImageState& render_attachment_image_state : render_attachment_info.image_state_list)
    {
        VkImageCreateInfo image_create_info {};
        image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_create_info.pNext = nullptr;
        image_create_info.flags = 0x0;
        image_create_info.imageType = VK_IMAGE_TYPE_2D;
        image_create_info.mipLevels = 1;
        image_create_info.arrayLayers = 1;
        image_create_info.queueFamilyIndexCount = 0;
        image_create_info.pQueueFamilyIndices = nullptr;
        image_create_info.extent = {init_info.window_width, init_info.window_height, 1};

        VkImageMemoryBarrier image_memory_barrier {};
        image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        image_memory_barrier.pNext = nullptr;
        image_memory_barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
        image_memory_barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
        image_memory_barrier.srcQueueFamilyIndex = vk_core::get_queue_family_idx();
        image_memory_barrier.dstQueueFamilyIndex = vk_core::get_queue_family_idx();
        image_memory_barrier.subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        };

        // Shared

        if (render_attachment_info.shared_image_state.format.has_value()) 
        {
            image_create_info.format = render_attachment_info.shared_image_state.format.value();
        }
        else
        {
            image_create_info.format = render_attachment_image_state.format.value();
        }

        if (render_attachment_info.shared_image_state.num_samples.has_value())
        {
            image_create_info.samples = render_attachment_info.shared_image_state.num_samples.value();
        }
        else
        {
            image_create_info.samples = render_attachment_image_state.num_samples.value();
        }

        if (render_attachment_info.shared_image_state.tiling.has_value())
        {
            image_create_info.tiling = render_attachment_info.shared_image_state.tiling.value();
        }
        else
        {
            image_create_info.tiling = render_attachment_image_state.tiling.value();
        }
        
        if (render_attachment_info.shared_image_state.sharing_mode.has_value())
        {
            image_create_info.sharingMode = render_attachment_info.shared_image_state.sharing_mode.value();
        }
        else
        {
            image_create_info.sharingMode = render_attachment_image_state.sharing_mode.value();
        }

        if (render_attachment_info.shared_image_state.initial_layout.has_value())
        {
            image_create_info.initialLayout = render_attachment_info.shared_image_state.initial_layout.value();
        }
        else
        {
            image_create_info.initialLayout = render_attachment_image_state.initial_layout.value();
        }

        // Unique

        image_create_info.format = render_attachment_image_state.format.value();
        image_create_info.usage = render_attachment_image_state.usage.value();

        VkImageViewCreateInfo image_view_create_info {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0x0,
            .image = VK_NULL_HANDLE,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = image_create_info.format,
            .components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
            .subresourceRange = {
                .aspectMask = (image_create_info.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) ? VK_IMAGE_ASPECT_COLOR_BIT : VK_IMAGE_ASPECT_DEPTH_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            }
        };

        render_attachment_list.emplace_back(image_create_info, image_view_create_info, init_info.frame_resource_count);


        image_memory_barrier.oldLayout = image_create_info.initialLayout;

        if (image_create_info.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
        {
            image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }
        else
        {
            ASSERT(image_create_info.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, "Render attachment must be of type color or depth (not both)!\n");
            image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
            image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        }

        for (uint32_t i = 0; i < init_info.frame_resource_count; i++)
        {
            image_memory_barrier.image = render_attachment_list.back().vk_handle_image_list[i];
            image_memory_barriers.push_back(image_memory_barrier);
        }
    }

    const VkCommandPool vk_handle_cmd_pool = vk_core::create_command_pool(0x0);
    const VkCommandBuffer vk_handle_cmd_buff = vk_core::allocate_command_buffer(vk_handle_cmd_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    const VkCommandBufferBeginInfo cmd_buff_begin_info {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = 0x0,
        .pInheritanceInfo = nullptr,
    };

    vkBeginCommandBuffer(vk_handle_cmd_buff, &cmd_buff_begin_info);

    vkCmdPipelineBarrier(vk_handle_cmd_buff,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0x0,
        0, nullptr,
        0, nullptr,
        static_cast<uint32_t>(image_memory_barriers.size()), image_memory_barriers.data());

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

    vk_core::destroy_command_pool(vk_handle_cmd_pool);

    return render_attachment_list;
}

static std::vector<RenderPass> create_render_passes(const renderer::InitInfo& init_info)
{
    const auto json_data = read_json_file(init_info.app_config_file);
    const JSONInfo_RenderPass render_pass_info = json_data.at("render-passes").get<JSONInfo_RenderPass>();
    const JSONInfo_AppSortBin sortbin_info = json_data.at("sortbins").get<JSONInfo_AppSortBin>();

    std::vector<RenderPass> render_pass_list;

    for (const JSONInfo_RenderPass::State state : render_pass_info.state_list)
    {
        const uint32_t render_pass_id = state.id;

        // Iter through all sortbins to find sortbins assigned to this render pass

        std::vector<uint32_t> sortbin_ids {};

        for (const JSONInfo_AppSortBin::State& sortbin_state : sortbin_info.sortbin_list)
        {
            if (sortbin_state.render_pass_id == render_pass_id)
            {
                sortbin_ids.push_back(sortbin_state.id);
            }
        }

        std::vector<RenderPass::AttachmentPassInfo> color_attachment_pass_info_list;

        for (const JSONInfo_RenderPass::AttachmentState& render_pass_attachment_state : state.color_attachment_list)
        {
            const RenderPass::AttachmentPassInfo attachment_info {
                .attachment_idx = render_pass_attachment_state.id,
                .image_layout = render_pass_attachment_state.image_layout,
                .load_op = render_pass_attachment_state.load_op,
                .store_op = render_pass_attachment_state.store_op,
                .clear_value = render_pass_attachment_state.clear_value
            };

            color_attachment_pass_info_list.push_back(attachment_info);
        }

        RenderPass::AttachmentPassInfo depth_attachment_pass_info {
            .attachment_idx = state.depth_attachment.id,
            .image_layout = state.depth_attachment.image_layout,
            .load_op = state.depth_attachment.load_op,
            .store_op = state.depth_attachment.store_op,
            .clear_value = state.depth_attachment.clear_value
        };

        render_pass_list.emplace_back(std::move(sortbin_ids), std::move(color_attachment_pass_info_list), std::move(depth_attachment_pass_info));
    }

    return render_pass_list;
}

static std::vector<SortBin> create_sortbins(const renderer::InitInfo& init_info, const std::vector<RenderPass>& render_pass_list, const std::vector<RenderPass::Attachment> render_attachment_list, const VkDescriptorSetLayout vk_handle_global_desc_set_layout )
{
    std::vector<SortBin> sortbin_list;

    const auto app_json_data = read_json_file(init_info.app_config_file);
    const auto sortbin_json_data = read_json_file(init_info.sortbin_config_file);
    const auto sortbin_reflection_json_data = read_json_file(init_info.sortbin_reflection_config_file);

    const JSONInfo_AppSortBin app_sortbin_data = app_json_data.at("sortbins").get<JSONInfo_AppSortBin>();
    const JSONInfo_SortBin sortbin_data = sortbin_json_data.at("sortbins").get<JSONInfo_SortBin>();
    const JSONInfo_SortBinReflection sortbin_reflection_data = sortbin_reflection_json_data.at("sortbin-reflections").get<JSONInfo_SortBinReflection>();

    for (const JSONInfo_AppSortBin::State& app_sortbin_state : app_sortbin_data.sortbin_list)
    {
        const JSONInfo_SortBin::State& sortbin_state = sortbin_data.state_umap.at(app_sortbin_state.name);

        // Pipeline State Processing

        PipelineInfo pipeline_info; 

        get_default_pipeline_states(init_info.window_width, init_info.window_height, pipeline_info);

        process_pipeline_state(sortbin_state, pipeline_info, init_info.shader_root_path);

        // Reflection Processing

        const JSONInfo_SortBinReflection::State& sortbin_reflection_state = sortbin_reflection_data.state_umap.at(sortbin_state.sortbin_name);

        process_reflection_state(sortbin_reflection_state, pipeline_info, vk_handle_global_desc_set_layout);

        // Rendering Info

        std::vector<VkFormat> color_attachment_format_list;

        const RenderPass& render_pass = render_pass_list[app_sortbin_state.render_pass_id];

        for (const RenderPass::AttachmentPassInfo& attachment_pass_info : render_pass.color_attachment_pass_info_list)
        {
            const RenderPass::Attachment& attachment = render_attachment_list[attachment_pass_info.attachment_idx];
            color_attachment_format_list.push_back(attachment.format);
        }

        const VkFormat depth_attachment_format = render_attachment_list[render_pass.depth_attachment_pass_info.attachment_idx].format;

        const VkPipelineRenderingCreateInfo rendering_create_info {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .pNext = nullptr,
            .viewMask = 0,
            .colorAttachmentCount = static_cast<uint32_t>(color_attachment_format_list.size()),
            .pColorAttachmentFormats = color_attachment_format_list.data(),
            .depthAttachmentFormat = depth_attachment_format,
            .stencilAttachmentFormat = VK_FORMAT_UNDEFINED,
        };

        // Blending Info

        const VkPipelineColorBlendAttachmentState blend_state_none {
            .blendEnable = VK_FALSE,
            .srcColorBlendFactor = VK_BLEND_FACTOR_ZERO,
            .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
            .colorBlendOp = VK_BLEND_OP_ADD,
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
            .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
            .alphaBlendOp = VK_BLEND_OP_ADD,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        };

        for (const RenderPass::AttachmentPassInfo& attachment_pass_info : render_pass.color_attachment_pass_info_list)
        {
            pipeline_info.color_blend_attachment_state_list.push_back(blend_state_none);
        }

        const VkPipelineColorBlendStateCreateInfo default_color_blend_state {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0x0,
            .logicOpEnable = VK_FALSE,
            .logicOp = VK_LOGIC_OP_COPY,
            .attachmentCount = static_cast<uint32_t>(pipeline_info.color_blend_attachment_state_list.size()),
            .pAttachments = pipeline_info.color_blend_attachment_state_list.data(),
            .blendConstants = {0, 0, 0, 0}
        };

        pipeline_info.color_blend_state_create_info = default_color_blend_state;

        // Pipeline Creation

        const VkGraphicsPipelineCreateInfo graphics_pipeline_create_info {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = &rendering_create_info,
            .flags = 0x0,
            .stageCount = static_cast<uint32_t>(pipeline_info.shader_stage_create_info_list.size()),
            .pStages = pipeline_info.shader_stage_create_info_list.data(),
            .pVertexInputState = &pipeline_info.vertex_input_create_info,
            .pInputAssemblyState = &pipeline_info.input_assembly_create_info,
            .pTessellationState = nullptr,
            .pViewportState = &pipeline_info.viewport_state_create_info,
            .pRasterizationState = &pipeline_info.rasterization_state_create_info,
            .pMultisampleState = &pipeline_info.multisample_state_create_info,
            .pDepthStencilState = &pipeline_info.depth_stencil_state_create_info,
            .pColorBlendState = &pipeline_info.color_blend_state_create_info,
            .pDynamicState = nullptr,
            .layout = pipeline_info.vk_handle_pipeline_layout,
            .renderPass = VK_NULL_HANDLE,
            .subpass = 0,
            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = 0,
        };

        SortBin sortbin {
            .vk_handle_pipeline = vk_core::create_graphics_pipeline(graphics_pipeline_create_info),
            .vk_handle_pipeline_layout = pipeline_info.vk_handle_pipeline_layout,
            .vertex_type = 0,
            .per_draw_data_type = 0,
            .vk_handle_desc_set_layout_list = pipeline_info.vk_handle_desc_set_layout_list,
        };

        sortbin_list.push_back(sortbin);


        for (const VkPipelineShaderStageCreateInfo& shader_stage_create_info : pipeline_info.shader_stage_create_info_list)
        {
            vk_core::destroy_shader_module(shader_stage_create_info.module);
        }
    }

    return sortbin_list;
}

namespace renderer
{

static std::vector<RenderPass::Attachment> global_render_attachment_list;
static std::vector<RenderPass> global_render_pass_list;
static std::vector<SortBin> global_sortbin_list;
static std::vector<Mesh> global_mesh_list; 


static GeometryBuffer* global_geometry_buffer;
static StagingBuffer* global_staging_buffer;

static UniformBuffer* global_frame_uniform_buffer;;
static BufferPool_VariableBlock* global_material_data_buffer;
static BufferPool_VariableBlock* global_draw_data_buffer;

static VkDescriptorSetLayout vk_handle_global_desc_set_layout;
static VkDescriptorPool vk_handle_global_desc_pool;
static std::vector<VkDescriptorSet> vk_handle_global_desc_set_list;

void init(const InitInfo& init_info)
{
    global_geometry_buffer = new GeometryBuffer({
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0x0,
        .size = 1024 * 1024, 
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr
    });

    global_staging_buffer = new StagingBuffer(1024 * 1024);

    global_material_data_buffer = new BufferPool_VariableBlock(init_info.frame_resource_count, 1024);
    global_draw_data_buffer = new BufferPool_VariableBlock(init_info.frame_resource_count, 1024);

    std::unordered_map<std::string, DescriptorVariable> frame_uniform_refl_set {{
        { "proj_mat", { "proj_mat", 0, 64 } },
        { "view_mat", { "view_mat", 64, 128 } },
    }};

    global_frame_uniform_buffer = new UniformBuffer(init_info.frame_resource_count, 128, std::move(frame_uniform_refl_set));

    // Need to have JSON with reflection info for all files in shared.glsl (the sortbin is not responsible for these resources)

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

    // Create desc set layout

    const std::array<VkDescriptorSetLayoutBinding, 3> desc_set_layout_binding_list {{
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS,
            .pImmutableSamplers = nullptr,
        },
        {
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS,
            .pImmutableSamplers = nullptr,
        },
        {
            .binding = 2,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS,
            .pImmutableSamplers = nullptr,
        }
    }};

    const VkDescriptorSetLayoutCreateInfo desc_set_layout_create_info {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0x0,
        .bindingCount = static_cast<uint32_t>(desc_set_layout_binding_list.size()),
        .pBindings = desc_set_layout_binding_list.data(),
    };

    vk_handle_global_desc_set_layout = vk_core::create_desc_set_layout(desc_set_layout_create_info);

    const std::array<VkDescriptorPoolSize, 2> desc_pool_sizes {{
        {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = init_info.frame_resource_count,
        },
        {
            .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = init_info.frame_resource_count * 2,
        },
    }};

    const VkDescriptorPoolCreateInfo desc_pool_create_info {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0x0,
        .maxSets = init_info.frame_resource_count,
        .poolSizeCount = static_cast<uint32_t>(desc_pool_sizes.size()),
        .pPoolSizes = desc_pool_sizes.data()
    };

    vk_handle_global_desc_pool = vk_core::create_desc_pool(desc_pool_create_info);

    const std::vector<VkDescriptorSetLayout> vk_handle_desc_set_layout_list(init_info.frame_resource_count, vk_handle_global_desc_set_layout);

    const VkDescriptorSetAllocateInfo desc_set_alloc_info {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = vk_handle_global_desc_pool,
        .descriptorSetCount = static_cast<uint32_t>(vk_handle_desc_set_layout_list.size()),
        .pSetLayouts = vk_handle_desc_set_layout_list.data()
    };

    vk_handle_global_desc_set_list = vk_core::allocate_desc_sets(desc_set_alloc_info);

    for (uint32_t i = 0; i < init_info.frame_resource_count; i++)
    {
        const VkDescriptorBufferInfo frame_ubo_desc_buffer_info = global_frame_uniform_buffer->get_descriptor_buffer_info(i);
        const VkDescriptorBufferInfo mat_ssbo_desc_buffer_info = global_material_data_buffer->get_descriptor_buffer_info(i);
        const VkDescriptorBufferInfo draw_ssbo_desc_buffer_info = global_draw_data_buffer->get_descriptor_buffer_info(i);

        std::array<VkWriteDescriptorSet, 3> write_desc_set_list {{
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = vk_handle_global_desc_set_list[i],
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
                .dstSet = vk_handle_global_desc_set_list[i],
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
                .dstSet = vk_handle_global_desc_set_list[i],
                .dstBinding = 2,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .pImageInfo = nullptr,
                .pBufferInfo = &draw_ssbo_desc_buffer_info,
                .pTexelBufferView = nullptr,
            }
        }};

        vk_core::update_desc_sets(static_cast<uint32_t>(write_desc_set_list.size()), write_desc_set_list.data(), 0, nullptr);
    }

    global_render_attachment_list = create_render_attachments(init_info);
    global_render_pass_list = create_render_passes(init_info);
    global_sortbin_list = create_sortbins(init_info, global_render_pass_list, global_render_attachment_list, vk_handle_global_desc_set_layout);
}

void terminate()
{
    delete global_geometry_buffer;

    for (const SortBin& sortbin : global_sortbin_list)
    {
        for (const VkDescriptorSetLayout vk_handle_desc_set_layout : sortbin.vk_handle_desc_set_layout_list)
        {
            vk_core::destroy_desc_set_layout(vk_handle_desc_set_layout);
        }

        vk_core::destroy_pipeline_layout(sortbin.vk_handle_pipeline_layout);
        vk_core::destroy_pipeline(sortbin.vk_handle_pipeline);
    }

    for (const RenderPass::Attachment& attachment : global_render_attachment_list)
    {
        for (const VkImage vk_handle_image : attachment.vk_handle_image_list)
        {
            vk_core::destroy_image(vk_handle_image);
        }

        for (const VkImageView vk_handle_image_view : attachment.vk_handle_image_view_list)
        {
            vk_core::destroy_image_view(vk_handle_image_view);
        }

        for (const VkDeviceMemory vk_handle_image_memory : attachment.vk_handle_image_memory_list)
        {
            vk_core::free_memory(vk_handle_image_memory);
        }
    }
}

void record_render_pass(const uint32_t renderpass_id, const VkCommandBuffer vk_handle_cmd_buff, const VkRect2D render_area, const uint32_t frame_resource_idx)
{
    const RenderPass& render_pass = global_render_pass_list[renderpass_id];

    const RenderPass::RecordInfo record_info {
        .frame_idx = frame_resource_idx,
        .vk_handle_cmd_buff = vk_handle_cmd_buff,
        .global_attachment_list = global_render_attachment_list,
        .global_sortbin_list = global_sortbin_list,
        .render_area = render_area,
        .vk_handle_index_buffer_list = { 
            global_geometry_buffer->get_vk_handle_buffer(),
            global_geometry_buffer->get_vk_handle_buffer(),
            global_geometry_buffer->get_vk_handle_buffer()
        },
        .vk_handle_global_desc_set = vk_handle_global_desc_set_list[frame_resource_idx],
    };

    const VkBuffer vk_handle_geometry_buffer = global_geometry_buffer->get_vk_handle_buffer();
    const VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(vk_handle_cmd_buff, 0, 1, &vk_handle_geometry_buffer, &offset);

    render_pass.record(record_info);
}

VkImage get_attachment_image(const uint32_t attachment_id, const uint32_t frame_resource_idx)
{
    return global_render_attachment_list[attachment_id].vk_handle_image_list[frame_resource_idx];
}


void add_renderable_to_sortbin(const Renderable& renderable)
{
    const Mesh& mesh = global_mesh_list[renderable.mesh_id];

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
            global_sortbin_list[mesh.sortbin_id].draw_list_u32.push_back(draw_info);
            break;
        }
        case 2:
        {
            global_sortbin_list[mesh.sortbin_id].draw_list_u16.push_back(draw_info);
            break;
        }
        case 1:
        {
            global_sortbin_list[mesh.sortbin_id].draw_list_u8.push_back(draw_info);
            break;
        }
        default:
        {
            global_sortbin_list[mesh.sortbin_id].draw_list.push_back(draw_info);
            break;
        }
    };
}

Renderable load_mesh(const MeshInitInfo& mesh_init_info)
{
    const Mesh mesh {
        .sortbin_id = mesh_init_info.sortbin_id,
        .index_count = mesh_init_info.index_count,
        .vertex_count = mesh_init_info.vertex_count,
        .first_vertex = 0,
        .first_index = static_cast<uint32_t>(global_geometry_buffer->queue_upload(mesh_init_info.index_stride, mesh_init_info.index_count, std::vector<uint8_t>(mesh_init_info.index_data, mesh_init_info.index_data + mesh_init_info.index_count * mesh_init_info.index_stride))),
        .vertex_offset = global_geometry_buffer->queue_upload(mesh_init_info.vertex_stride, mesh_init_info.vertex_count, std::vector<uint8_t>(mesh_init_info.vertex_data, mesh_init_info.vertex_data + mesh_init_info.vertex_count * mesh_init_info.vertex_stride)),
        .index_stride = mesh_init_info.index_stride
    };

    const SortBin& sortbin = global_sortbin_list[mesh.sortbin_id];
    ASSERT(sortbin.material_data_block_size == mesh_init_info.material_data.size(), "Material data size mismatch!\n");
    ASSERT(sortbin.draw_data_block_size == mesh_init_info.draw_data.size() + sizeof(uint32_t), "Draw data size mismatch!\n");

    const uint32_t mat_ID = global_material_data_buffer->acquire_block(sortbin.material_data_block_size);
    void* mat_data_ptr = global_material_data_buffer->get_writable_block(sortbin.material_data_block_size, mat_ID);
    memcpy(mat_data_ptr, mesh_init_info.material_data.data(), sortbin.material_data_block_size);

    const uint32_t draw_ID = global_draw_data_buffer->acquire_block(sortbin.draw_data_block_size);
    void* draw_data_ptr = global_draw_data_buffer->get_writable_block(sortbin.draw_data_block_size, draw_ID);
    memcpy(draw_data_ptr, &mat_ID, sizeof(uint32_t));
    memcpy(static_cast<uint8_t*>(draw_data_ptr) + sizeof(uint32_t), mesh_init_info.draw_data.data(), sortbin.draw_data_block_size - sizeof(uint32_t));

    const std::vector<UploadInfo> queued_geometry_upload_info_list = global_geometry_buffer->get_queued_uploads();
    const std::vector<UploadInfo> queued_material_data_upload_info_list = global_material_data_buffer->get_queued_uploads(0);
    const std::vector<UploadInfo> queued_draw_data_upload_info_list = global_draw_data_buffer->get_queued_uploads(0);

    for (const UploadInfo& upload_info : queued_geometry_upload_info_list)
    {
        const void* const data_ptr = upload_info.data_pointer ? upload_info.data_pointer : (void*)upload_info.data_vector.data();
        global_staging_buffer->queue_upload(global_geometry_buffer->get_vk_handle_buffer(), upload_info.dst_offset, upload_info.size, data_ptr);
    }

    for (const UploadInfo& upload_info : queued_material_data_upload_info_list)
    {
        const void* const data_ptr = upload_info.data_pointer ? upload_info.data_pointer : (void*)upload_info.data_vector.data();
        global_staging_buffer->queue_upload(global_material_data_buffer->get_vk_handle_buffer(), upload_info.dst_offset, upload_info.size, data_ptr);
    }

    for (const UploadInfo& upload_info : queued_draw_data_upload_info_list)
    {
        const void* const data_ptr = upload_info.data_pointer ? upload_info.data_pointer : (void*)upload_info.data_vector.data();
        global_staging_buffer->queue_upload(global_draw_data_buffer->get_vk_handle_buffer(), upload_info.dst_offset, upload_info.size, data_ptr);
    }

    const Renderable renderable {
        .mesh_id = static_cast<uint32_t>(global_mesh_list.size()),
        .draw_id = draw_ID,
        .bsphere_origin = {0.0f, 0.0f, 0.0f},
        .bsphere_radius = 0.0f,
    };

    global_mesh_list.push_back(mesh);

    return renderable;
}


void flush_geometry_uploads(const VkCommandBuffer vk_handle_cmd_buff)
{
   global_staging_buffer->flush(vk_handle_cmd_buff); 
}

void flush_draw_data_uploads(const VkCommandBuffer vk_handle_cmd_buff)
{
   global_staging_buffer->flush(vk_handle_cmd_buff); 
}

}; // renderer
