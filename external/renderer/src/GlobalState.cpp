#include "GlobalState.hpp"
#include "internal/misc/logger.hpp"
#include "internal/misc/vk_enum_to_string.hpp"
#include "internal/misc/json_structures.hpp"
#include "internal/pod/DescriptorVariable.hpp"
#include "internal/buffers/UniformBuffer.hpp"
#include "internal/buffers/GeometryBuffer.hpp"
#include "internal/buffers/StagingBuffer.hpp"
#include "internal/buffers/BufferPool_VariableBlock.hpp"

#include "json.hpp"
#include <fstream>
#include <optional>

static nlohmann::json read_json_file(const char* const filepath);
static std::unordered_map<std::string, uint8_t> init_id_lut_render_attachment(const RendererState::CreateInfo& create_info);
static std::unordered_map<std::string, uint16_t> init_id_lut_render_pass(const RendererState::CreateInfo& create_info);
static std::unordered_map<std::string, uint16_t> init_id_lut_sort_bin(const RendererState::CreateInfo& create_info);
static std::vector<RenderPass::Attachment> init_vec_render_attachment(const RendererState::CreateInfo& create_info);
static std::vector<uint16_t> get_registered_sortbins(const JSONInfo_AppSortBin& sortbin_info, const JSONInfo_RenderPass::State& render_pass_state, const std::unordered_map<std::string, uint16_t>& sortbin_name_to_id_umap);
static std::vector<RenderPass::ReadAttachmentPassInfo> get_registered_inputs(const JSONInfo_RenderPass::State& render_pass_state, const std::unordered_map<std::string, uint8_t>& name_id_lut_render_attachment);
static std::vector<RenderPass::WriteAttachmentPassInfo> get_registered_color_outputs(const JSONInfo_RenderPass::State& render_pass_state, const std::unordered_map<std::string, uint8_t>& name_id_lut_render_attachment);
static std::optional<RenderPass::WriteAttachmentPassInfo> get_registered_depth_output(const JSONInfo_RenderPass::State& render_pass_state, const std::unordered_map<std::string, uint8_t>& name_id_lut_render_attachment);
static std::vector<RenderPass> init_vec_render_pass(const RendererState::CreateInfo& create_info, const std::unordered_map<std::string, uint8_t>& name_id_lut_render_attachment, const std::unordered_map<std::string, uint16_t>& name_id_lut_sort_bin);
static VkDescriptorPool init_desc_pool(const RendererState::CreateInfo& create_info, const std::vector<RenderPass>& render_pass_vec);
static std::vector<VkDescriptorSetLayoutBinding> create_desc_set_binding_list(const std::vector<JSONInfo_DescriptorBinding>& json_desc_set_binding_list);
static VkDescriptorSetLayout init_frame_desc_set_layout(const RendererState::CreateInfo& create_info);
static std::vector<VkDescriptorSet> init_vec_frame_desc_set(const RendererState::CreateInfo& create_info, const VkDescriptorPool vk_handle_desc_pool, const VkDescriptorSetLayout vk_handle_desc_set_layout);
static std::vector<uint16_t> init_vec_compatible_sortbin_ID(const RendererState::CreateInfo& create_info, const std::unordered_map<std::string, uint16_t>& name_id_lut_sort_bin);
static std::vector<VkViewport> create_viewport(const RendererState::CreateInfo& create_info);
static std::vector<VkRect2D> create_scissor(const RendererState::CreateInfo& create_info);
static VkPipelineViewportStateCreateInfo create_viewport_state(const std::vector<VkViewport>& viewport_vec, const std::vector<VkRect2D>& scissor_vec);
static VkPipelineMultisampleStateCreateInfo create_multisample_state();
static VkShaderModule create_shader_module(const std::string& shader_root_path, const std::string& shader_name);
static std::vector<VkPipelineShaderStageCreateInfo> create_shader_stage_vec(const std::vector<std::string>& shader_name_list, const std::string& shader_root_path);
static VkPipelineInputAssemblyStateCreateInfo create_input_assembly_state(const JSONInfo_SortBinPipelineState::State& sortbin_state);
static VkPipelineRasterizationStateCreateInfo create_rasterization_state(const JSONInfo_SortBinPipelineState::State& sortbin_state);
static VkPipelineDepthStencilStateCreateInfo create_depth_stencil_state(const JSONInfo_SortBinPipelineState::State& sortbin_state);
static std::vector<VkPushConstantRange> create_push_const_ranges(const JSONInfo_SortBinReflection::State& sortbin_state);
static VkPipelineLayout create_sort_bin_pipeline_layout(const VkDescriptorSetLayout vk_handle_frame_desc_set_layout, const std::optional<VkDescriptorSetLayout> vk_handle_render_pass_desc_set_layout, const std::vector<VkPushConstantRange>& push_const_range_vec);
static std::vector<VkFormat> get_sort_bin_color_attachment_format_vec(const RenderPass& render_pass, const std::vector<RenderPass::Attachment>& render_attachment_vec);
static VkPipelineRenderingCreateInfo create_rendering_create_info(const std::vector<VkFormat>& color_attachment_format_vec, const VkFormat depth_attachment_format);
static std::vector<VkPipelineColorBlendAttachmentState> create_color_blend_attachment_state_vec(const RenderPass& render_pass);
static VkPipelineColorBlendStateCreateInfo create_color_blend_state(const std::vector<VkPipelineColorBlendAttachmentState>& color_blend_attachment_state_vec);
static std::unordered_map<std::string, DescriptorVariable> create_desc_var_umap(const std::vector<JSONInfo_DescriptorVariable>& json_desc_var_list);
static std::vector<SortBin> init_vec_sort_bin(const RendererState::CreateInfo& create_info, const std::vector<RenderPass>& render_pass_vec, const std::vector<RenderPass::Attachment> render_attachment_list, const std::unordered_map<std::string, uint16_t>& name_id_lut_render_pass, const VkDescriptorSetLayout vk_handle_frame_desc_set_layout);
static std::unique_ptr<UniformBuffer> create_frame_ubo(const RendererState::CreateInfo& create_info, const std::string& ubo_name);
static void update_frame_desc_sets(const uint32_t frame_resource_count, const UniformBuffer* frame_uniform_buffer, const BufferPool_VariableBlock* material_data_buffer, const BufferPool_VariableBlock* draw_data_buffer, const UniformBuffer* frame_fwd_light_ubo, const std::vector<VkDescriptorSet>& vk_handle_desc_set_list);

RendererState::RendererState(const CreateInfo& create_info)
    : name_id_lut_render_attachment { init_id_lut_render_attachment(create_info) }
    , name_id_lut_render_pass { init_id_lut_render_pass(create_info) }
    , name_id_lut_sort_bin { init_id_lut_sort_bin(create_info) }
    , render_attachment_vec{ init_vec_render_attachment(create_info) }
    , render_pass_vec{ init_vec_render_pass(create_info, name_id_lut_render_attachment, name_id_lut_sort_bin) }
    , vk_handle_frame_desc_pool{ init_desc_pool(create_info, render_pass_vec) }
    , vk_handle_frame_desc_set_layout{ init_frame_desc_set_layout(create_info) }
    , vk_handle_frame_desc_set_vec{ init_vec_frame_desc_set(create_info, vk_handle_frame_desc_pool, vk_handle_frame_desc_set_layout) }
    , compatible_sortbin_ID_lut{ init_vec_compatible_sortbin_ID(create_info, name_id_lut_sort_bin) }
{
    sort_bin_vec = init_vec_sort_bin(create_info, render_pass_vec, render_attachment_vec, name_id_lut_render_pass, vk_handle_frame_desc_set_layout);
    geometry_buffer = std::make_unique<GeometryBuffer>(1 << 10);
    staging_buffer = std::make_unique<StagingBuffer>(1 << 10);
    material_data_buffer = std::make_unique<BufferPool_VariableBlock>(create_info.frame_resource_count, 1 << 10);
    draw_data_buffer = std::make_unique<BufferPool_VariableBlock>(create_info.frame_resource_count, 1 << 10);
    frame_general_ubo = create_frame_ubo(create_info, "Frame_UBO");
    frame_fwd_light_ubo = create_frame_ubo(create_info, "Frame_ForwardPointLightUBO");

    update_frame_desc_sets(create_info.frame_resource_count, frame_general_ubo.get(), material_data_buffer.get(), draw_data_buffer.get(), frame_fwd_light_ubo.get(), vk_handle_frame_desc_set_vec);
}

RendererState::~RendererState()
{
    vk_core::destroy_desc_pool(vk_handle_frame_desc_pool);
    vk_core::destroy_desc_set_layout(vk_handle_frame_desc_set_layout);

    for (const SortBin& sortbin : sort_bin_vec)
    {
        vk_core::destroy_pipeline_layout(sortbin.vk_handle_pipeline_layout);
        vk_core::destroy_pipeline(sortbin.vk_handle_pipeline);
    }

    for (const RenderPass& render_pass : render_pass_vec)
    {
        if (render_pass.get_desc_set_layout() != VK_NULL_HANDLE)
        {
            vk_core::destroy_desc_set_layout(render_pass.get_desc_set_layout());
        }
    }

    for (const RenderPass::Attachment& attachment : render_attachment_vec)
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

static nlohmann::json read_json_file(const char* const filepath)
{
    std::ifstream file(filepath);
    ASSERT(file.is_open(), "Failed to vulkan init config file: %s\n", filepath);

    const nlohmann::json json_data = nlohmann::json::parse(file);

    file.close();

    return json_data;
}

static std::unordered_map<std::string, uint8_t> init_id_lut_render_attachment(const RendererState::CreateInfo& create_info)
{    
    const auto json_data = read_json_file(create_info.file_app_state);
    const JSONInfo_RenderAttachment render_attachment_info = json_data.at("render-attachments").get<JSONInfo_RenderAttachment>();

    uint16_t render_attachment_ID = 0u;
    std::unordered_map<std::string, uint8_t> umap;

    for (const JSONInfo_RenderAttachment::ImageState& render_attachment_state : render_attachment_info.image_state_list)
    {
        const auto iter = umap.find(render_attachment_state.name);
        
        if (iter == umap.end())
        {
            umap[render_attachment_state.name] = render_attachment_ID++;
        }
        else
        {
            EXIT("Render attachment %s already exists!\n", render_attachment_state.name.c_str());
        }
    }

    LOG("App Info - # of registered render attachments: %lu\n", umap.size());
    return umap;
}

static std::unordered_map<std::string, uint16_t> init_id_lut_render_pass(const RendererState::CreateInfo& create_info)
{
    const auto json_data = read_json_file(create_info.file_app_state);
    const JSONInfo_RenderPass render_pass_info = json_data.at("render-passes").get<JSONInfo_RenderPass>();

    uint16_t render_pass_ID = 0u;
    std::unordered_map<std::string, uint16_t> umap;

    for (const JSONInfo_RenderPass::State& render_pass_state : render_pass_info.state_list)
    {
        const auto iter = umap.find(render_pass_state.name);
        
        if (iter == umap.end())
        {
            umap[render_pass_state.name] = render_pass_ID++;
        }
        else
        {
            EXIT("Render pass %s already exists!\n", render_pass_state.name.c_str());
        }
    }

    LOG("App Info - # of registered render passes: %lu\n", umap.size());
    return umap;
}

static std::unordered_map<std::string, uint16_t> init_id_lut_sort_bin(const RendererState::CreateInfo& create_info)
{
    const auto json_data = read_json_file(create_info.file_app_state);
    const JSONInfo_AppSortBin sortbin_info = json_data.at("sortbins").get<JSONInfo_AppSortBin>();

    uint16_t sortbin_ID = 0u;
    std::unordered_map<std::string, uint16_t> umap;

    for (const JSONInfo_AppSortBin::State& sortbin_state : sortbin_info.sortbin_list)
    {
        const auto iter = umap.find(sortbin_state.name);
        
        if (iter == umap.end())
        {
            umap[sortbin_state.name] = sortbin_ID++;
        }
        else
        {
            EXIT("Sortbin %s already exists!\n", sortbin_state.name.c_str());
        }
    }

    LOG("App Info - # of registered sortbins: %lu\n", umap.size());
    return umap;
}

static std::vector<RenderPass::Attachment> init_vec_render_attachment(const RendererState::CreateInfo& create_info)
{
    const auto json_data = read_json_file(create_info.file_app_state);
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
        image_create_info.extent = {create_info.window_x_dim, create_info.window_y_dim, 1};

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

        render_attachment_list.emplace_back(image_create_info, image_view_create_info, create_info.frame_resource_count);


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

        for (uint32_t i = 0; i < create_info.frame_resource_count; i++)
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

static std::vector<uint16_t> get_registered_sortbins(const JSONInfo_AppSortBin& sortbin_info, const JSONInfo_RenderPass::State& render_pass_state, const std::unordered_map<std::string, uint16_t>& sortbin_name_to_id_umap)
{
    std::vector<uint16_t> registered_sortbin_ids {};

    for (const JSONInfo_AppSortBin::State& sortbin_state : sortbin_info.sortbin_list)
    {
        if (sortbin_state.render_pass_name == render_pass_state.name)
        {
            registered_sortbin_ids.push_back(sortbin_name_to_id_umap.at(sortbin_state.name));
        }
    }

    return registered_sortbin_ids;
}

static std::vector<RenderPass::ReadAttachmentPassInfo> get_registered_inputs(const JSONInfo_RenderPass::State& render_pass_state, const std::unordered_map<std::string, uint8_t>& name_id_lut_render_attachment)
{
    std::vector<RenderPass::ReadAttachmentPassInfo> input_attachment_pass_info_list;

    for (const JSONInfo_RenderPass::ReadAttachmentState& read_attachment_state : render_pass_state.input_attachment_list)
    {
        const RenderPass::ReadAttachmentPassInfo attachment_info {
            .attachment_idx = name_id_lut_render_attachment.at(read_attachment_state.name),
            .image_layout = read_attachment_state.image_layout
        };

        input_attachment_pass_info_list.push_back(attachment_info);
    }

    return input_attachment_pass_info_list;
}

static std::vector<RenderPass::WriteAttachmentPassInfo> get_registered_color_outputs(const JSONInfo_RenderPass::State& render_pass_state, const std::unordered_map<std::string, uint8_t>& name_id_lut_render_attachment)
{
    std::vector<RenderPass::WriteAttachmentPassInfo> color_attachment_pass_info_list;

    for (const JSONInfo_RenderPass::WriteAttachmentState& render_pass_attachment_state : render_pass_state.color_attachment_list)
    {
        const RenderPass::WriteAttachmentPassInfo attachment_info {
            .attachment_idx = name_id_lut_render_attachment.at(render_pass_attachment_state.name),
            .image_layout = render_pass_attachment_state.image_layout,
            .load_op = render_pass_attachment_state.load_op,
            .store_op = render_pass_attachment_state.store_op,
            .clear_value = render_pass_attachment_state.clear_value
        };

        color_attachment_pass_info_list.push_back(attachment_info);
    }

    return color_attachment_pass_info_list;
}

static std::optional<RenderPass::WriteAttachmentPassInfo> get_registered_depth_output(const JSONInfo_RenderPass::State& render_pass_state, const std::unordered_map<std::string, uint8_t>& name_id_lut_render_attachment)
{
    if (render_pass_state.depth_attachment.has_value())
    {
        const JSONInfo_RenderPass::WriteAttachmentState& depth_attachment_state = render_pass_state.depth_attachment.value();

        const RenderPass::WriteAttachmentPassInfo attachment_info {
            .attachment_idx = name_id_lut_render_attachment.at(depth_attachment_state.name),
            .image_layout = depth_attachment_state.image_layout,
            .load_op = depth_attachment_state.load_op,
            .store_op = depth_attachment_state.store_op,
            .clear_value = depth_attachment_state.clear_value
        };

        return attachment_info;
    }

    return std::nullopt;
}

static std::vector<RenderPass> init_vec_render_pass(const RendererState::CreateInfo& create_info, const std::unordered_map<std::string, uint8_t>& name_id_lut_render_attachment, const std::unordered_map<std::string, uint16_t>& name_id_lut_sort_bin)
{
    const auto json_data = read_json_file(create_info.file_app_state);
    const JSONInfo_RenderPass render_pass_info = json_data.at("render-passes").get<JSONInfo_RenderPass>();
    const JSONInfo_AppSortBin sortbin_info = json_data.at("sortbins").get<JSONInfo_AppSortBin>();

    std::vector<RenderPass> vec;

    for (const JSONInfo_RenderPass::State render_pass_state : render_pass_info.state_list)
    {
        const auto sortbin_IDs = get_registered_sortbins(sortbin_info, render_pass_state, name_id_lut_sort_bin);
        const auto input_info = get_registered_inputs(render_pass_state, name_id_lut_render_attachment);
        const auto color_info = get_registered_color_outputs(render_pass_state, name_id_lut_render_attachment);
        const auto depth_info = get_registered_depth_output(render_pass_state, name_id_lut_render_attachment);

        const RenderPass::InitInfo render_pass_init_info {
            .frame_resource_count = create_info.frame_resource_count,
            .supported_sortbin_id_list = std::move(sortbin_IDs),
            .read_attachment_pass_info_list = std::move(input_info),
            .write_color_attachment_pass_info_list = std::move(color_info),
            .write_depth_attachment_pass_info = std::move(depth_info)
        };

        RenderPass render_pass(std::move(render_pass_init_info));

        vec.push_back(render_pass);
    }

    return vec;
}

static VkDescriptorPool init_desc_pool(const RendererState::CreateInfo& create_info, const std::vector<RenderPass>& render_pass_vec)
{
    const auto json_data_frame_desc_refl = read_json_file(create_info.refl_file_frame_desc_set_def);
    const auto frame_desc_set_binding_vec = json_data_frame_desc_refl.at("bindings").get<std::vector<JSONInfo_DescriptorBinding>>();

    std::unordered_map<VkDescriptorType, uint32_t> desc_type_count_umap;

    for (const auto& binding_state : frame_desc_set_binding_vec)
    {
        desc_type_count_umap[binding_state.descriptor_type] += binding_state.descriptor_count * create_info.frame_resource_count;
    }

    uint32_t render_pass_input_attachment_count = 0u;
    uint32_t render_pass_set_count = 0u; 

    for (const RenderPass& render_pass : render_pass_vec)
    {
        render_pass_input_attachment_count += render_pass.read_attachment_pass_info_list.size();
        if (render_pass.read_attachment_pass_info_list.size() > 0)
        {
            render_pass_set_count++;
        }
    }

    if (render_pass_set_count > 0)
    {
        desc_type_count_umap[VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER] += render_pass_input_attachment_count * create_info.frame_resource_count;
    }


    std::vector<VkDescriptorPoolSize> desc_pool_size_vec;

    for (const auto& [desc_type, desc_count] : desc_type_count_umap)
    {
        desc_pool_size_vec.push_back({desc_type, desc_count});
    }

    const uint32_t max_desc_sets = 
        create_info.frame_resource_count * 1 +                     // 1 Frame Set per frame resource
        create_info.frame_resource_count * render_pass_set_count;  // 1 RenderPass Set (for each renderpass) per frame resource

    const VkDescriptorPoolCreateInfo desc_pool_create_info {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0x0,
        .maxSets = max_desc_sets,
        .poolSizeCount = static_cast<uint32_t>(desc_pool_size_vec.size()),
        .pPoolSizes = desc_pool_size_vec.data()
    };

    return vk_core::create_desc_pool(desc_pool_create_info);
}

static std::vector<VkDescriptorSetLayoutBinding> create_desc_set_binding_list(const std::vector<JSONInfo_DescriptorBinding>& json_desc_set_binding_list)
{
    std::vector<VkDescriptorSetLayoutBinding> desc_set_binding_list;

    for (const JSONInfo_DescriptorBinding& json_desc_binding : json_desc_set_binding_list)
    {
        const VkDescriptorSetLayoutBinding desc_set_layout_binding {
            .binding = json_desc_binding.binding_ID,
            .descriptorType = json_desc_binding.descriptor_type,
            .descriptorCount = json_desc_binding.descriptor_count,
            .stageFlags = json_desc_binding.stage_flags,
            .pImmutableSamplers = nullptr,
        }; 

        desc_set_binding_list.push_back(desc_set_layout_binding);
    }

    return desc_set_binding_list;
}

static VkDescriptorSetLayout init_frame_desc_set_layout(const RendererState::CreateInfo& create_info)
{
    const auto json_data_frame_desc_refl = read_json_file(create_info.refl_file_frame_desc_set_def).at("bindings").get<std::vector<JSONInfo_DescriptorBinding>>();
    const auto frame_desc_set_binding_list = create_desc_set_binding_list(json_data_frame_desc_refl);

    const VkDescriptorSetLayoutCreateInfo desc_set_layout_create_info {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0x0,
        .bindingCount = static_cast<uint32_t>(frame_desc_set_binding_list.size()),
        .pBindings = frame_desc_set_binding_list.data(),
    };

    return vk_core::create_desc_set_layout(desc_set_layout_create_info);
}

static std::vector<VkDescriptorSet> init_vec_frame_desc_set(const RendererState::CreateInfo& create_info, const VkDescriptorPool vk_handle_desc_pool, const VkDescriptorSetLayout vk_handle_desc_set_layout)
{
    const std::vector<VkDescriptorSetLayout> vk_handle_desc_set_layout_list(create_info.frame_resource_count, vk_handle_desc_set_layout);

    const VkDescriptorSetAllocateInfo desc_set_alloc_info {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = vk_handle_desc_pool,
        .descriptorSetCount = static_cast<uint32_t>(vk_handle_desc_set_layout_list.size()),
        .pSetLayouts = vk_handle_desc_set_layout_list.data()
    };

    return vk_core::allocate_desc_sets(desc_set_alloc_info);
}

static std::vector<uint16_t> init_vec_compatible_sortbin_ID(const RendererState::CreateInfo& create_info, const std::unordered_map<std::string, uint16_t>& name_id_lut_sort_bin)
{
    const auto json_data_sort_bin_refl_state = read_json_file(create_info.refl_file_sortbin_mat_draw_def);
    const auto sort_bin_reflection_umap = json_data_sort_bin_refl_state.at("sortbin-reflections").get<JSONInfo_SortBinReflection>().state_umap;
    const auto sort_bin_pipeline_state_umap = read_json_file(create_info.file_sortbin_pipeline_state).at("sortbins").get<JSONInfo_SortBinPipelineState>().state_umap;

    // compaitble if vertex input state "aligns" and iff material and draw definitions are the same or DNE

    struct SortBinAlikeState
    {
        JSONInfo_SortBinPipelineState::VertexInputState vertex_input_state;
        std::vector<JSONInfo_DescriptorVariable> definition_material_data;
        std::vector<JSONInfo_DescriptorVariable> definition_draw_data;

        bool operator==(const SortBinAlikeState& other) const
        {
            for (size_t i = 0; i < vertex_input_state.binding_description_list.size(); ++i)
            {
                if (vertex_input_state.binding_description_list[i].binding != other.vertex_input_state.binding_description_list[i].binding ||
                    vertex_input_state.binding_description_list[i].stride != other.vertex_input_state.binding_description_list[i].stride ||
                    vertex_input_state.binding_description_list[i].inputRate != other.vertex_input_state.binding_description_list[i].inputRate)
                {
                    return false;
                }
            }

            if (vertex_input_state.attribute_description_list.size() == other.vertex_input_state.attribute_description_list.size())
            {
                for (size_t i = 0; i < vertex_input_state.attribute_description_list.size(); ++i)
                {
                    if (vertex_input_state.attribute_description_list[i].usage != other.vertex_input_state.attribute_description_list[i].usage &&
                        vertex_input_state.attribute_description_list[i].attribute_desctiption.offset != other.vertex_input_state.attribute_description_list[i].attribute_desctiption.offset &&
                        vertex_input_state.attribute_description_list[i].attribute_desctiption.format != other.vertex_input_state.attribute_description_list[i].attribute_desctiption.format &&
                        vertex_input_state.attribute_description_list[i].attribute_desctiption.location != other.vertex_input_state.attribute_description_list[i].attribute_desctiption.location &&
                        vertex_input_state.attribute_description_list[i].attribute_desctiption.binding != other.vertex_input_state.attribute_description_list[i].attribute_desctiption.binding)
                    {
                        return false;
                    }
                }

                return true;
            }

            // Attributes must be a subset of the other
            std::vector<JSONInfo_SortBinPipelineState::VertexInputAttributeDescription> large_attrib_list;
            std::vector<JSONInfo_SortBinPipelineState::VertexInputAttributeDescription> small_attrib_list;

            if (vertex_input_state.attribute_description_list.size() < other.vertex_input_state.attribute_description_list.size())
            {
                // Other has more attributes
                large_attrib_list = other.vertex_input_state.attribute_description_list;
                small_attrib_list = vertex_input_state.attribute_description_list;
            }
            else
            {
                // Same # of attributes or I have more
                large_attrib_list = vertex_input_state.attribute_description_list;
                small_attrib_list = other.vertex_input_state.attribute_description_list;
            }

            uint32_t found_count = 0;

            for (const auto& large_attrib : large_attrib_list)
            {
                for (const auto& small_attrib : small_attrib_list)
                {
                    if (small_attrib.usage == large_attrib.usage)
                    {
                        if (small_attrib.attribute_desctiption.offset != large_attrib.attribute_desctiption.offset ||
                            small_attrib.attribute_desctiption.format != large_attrib.attribute_desctiption.format ||
                            small_attrib.attribute_desctiption.location != large_attrib.attribute_desctiption.location ||
                            small_attrib.attribute_desctiption.binding != large_attrib.attribute_desctiption.binding)
                        {
                            return false;
                        }

                        found_count++;
                    }
                }
            }

            if (found_count != small_attrib_list.size())
            {
                // Not all attributes in the smaller list were found in the larger list
                return false;
            }

            bool mat_def_equal = false;
            bool draw_def_equal = false;

            if (definition_material_data.empty() || other.definition_material_data.empty())
            {
                mat_def_equal = true;
            }

            if (definition_draw_data.empty() || other.definition_draw_data.empty())
            {
                draw_def_equal = true;
            }

            if (mat_def_equal && draw_def_equal)
            {
                return true;
            }

            return vertex_input_state == other.vertex_input_state &&
                   definition_material_data == other.definition_material_data &&
                   definition_draw_data == other.definition_draw_data;
        }
    };

    struct CustomHash
    {
        size_t operator()(const SortBinAlikeState& state) const
        {
            return 0lu;
        }
    };

    uint16_t uncompatible_sort_bin_count = 0u;
    std::vector<uint16_t> compatible_sort_bin_ID_vec(name_id_lut_sort_bin.size(), -1);

    std::unordered_map<SortBinAlikeState, uint16_t, CustomHash> alike_sort_bin_umap;

    for (const auto& [sort_bin_name, sort_bin_ID] : name_id_lut_sort_bin)
    {
        ASSERT(sort_bin_reflection_umap.contains(sort_bin_name), "Sortbin %s not found in sortbin reflection file!\n", sort_bin_name.c_str());
        ASSERT(sort_bin_pipeline_state_umap.contains(sort_bin_name), "Sortbin %s not found in sortbin pipeline state file!\n", sort_bin_name.c_str());
        const auto sort_bin_reflection_state = sort_bin_reflection_umap.at(sort_bin_name);
        const auto sort_bin_pipeline_state = sort_bin_pipeline_state_umap.at(sort_bin_name);

        const SortBinAlikeState alike_state {
            .vertex_input_state = sort_bin_pipeline_state.pipeline_state.vertex_input_state,
            .definition_material_data = sort_bin_reflection_state.definition_material_data.members,
            .definition_draw_data = sort_bin_reflection_state.definition_draw_data.members
        };

        const auto alike_iter = alike_sort_bin_umap.find(alike_state);

        if (alike_iter == alike_sort_bin_umap.end())
        {
           // I have ecnountered a new state that is not compatible with any previous states
           compatible_sort_bin_ID_vec[sort_bin_ID] = uncompatible_sort_bin_count;
           alike_sort_bin_umap[alike_state] = uncompatible_sort_bin_count++;
        }
        else
        {
            compatible_sort_bin_ID_vec[sort_bin_ID] = alike_iter->second;
        }
    }

    return compatible_sort_bin_ID_vec;
}

static std::vector<VkViewport> create_viewport(const RendererState::CreateInfo& create_info)
{
    const std::vector<VkViewport> viewport {{
        .x = 0,
        .y = 0,
        .width = static_cast<float>(create_info.window_x_dim),
        .height = static_cast<float>(create_info.window_y_dim),
        .minDepth = 0,
        .maxDepth = 1,
    }};

    return viewport;
}

static std::vector<VkRect2D> create_scissor(const RendererState::CreateInfo& create_info)
{
    const std::vector<VkRect2D> scissor {{
        .offset = {.x = 0, .y = 0},
        .extent = {create_info.window_x_dim, create_info.window_y_dim}
    }};

    return scissor;
}

static VkPipelineViewportStateCreateInfo create_viewport_state(const std::vector<VkViewport>& viewport_vec, const std::vector<VkRect2D>& scissor_vec)
{
    const VkPipelineViewportStateCreateInfo default_viewport_state {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0x0,
        .viewportCount = static_cast<uint32_t>(viewport_vec.size()),
        .pViewports = viewport_vec.data(),
        .scissorCount = static_cast<uint32_t>(scissor_vec.size()),
        .pScissors = scissor_vec.data(),
    };

    return default_viewport_state;
}

static VkPipelineMultisampleStateCreateInfo create_multisample_state()
{
    const VkPipelineMultisampleStateCreateInfo default_multisample_state {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 0,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };

    return default_multisample_state;
}

static VkShaderModule create_shader_module(const std::string& shader_root_path, const std::string& shader_name)
{
    const std::string complete_filepath = shader_root_path + shader_name + ".spv";

    FILE* f = fopen(complete_filepath.c_str(), "r");
    ASSERT(f != 0, "Failed to open file %s!\n", complete_filepath.c_str());

    fseek(f, 0, SEEK_END);
    const size_t nbytes_file_size = (size_t)ftell(f);
    rewind(f);

    uint32_t* buffer = (uint32_t*)malloc(nbytes_file_size);
    fread(buffer, nbytes_file_size, 1, f);
    fclose(f);

    VkShaderModuleCreateInfo create_info {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = nbytes_file_size,
        .pCode = buffer,
    };

    const VkShaderModule shader_module = vk_core::create_shader_module(create_info);

    free(buffer);

    return shader_module;
};

static std::vector<VkPipelineShaderStageCreateInfo> create_shader_stage_vec(const std::vector<std::string>& shader_name_list, const std::string& shader_root_path)
{    
    const auto get_shader_stage = [](const std::string& shader_name) {
        if (shader_name.ends_with(".vert"))
            return VK_SHADER_STAGE_VERTEX_BIT;
        if (shader_name.ends_with(".geom"))
            return VK_SHADER_STAGE_GEOMETRY_BIT;
        if (shader_name.ends_with(".frag"))
            return VK_SHADER_STAGE_FRAGMENT_BIT;

        EXIT("No shader stage associated with %s\n", shader_name.c_str());
    };

    std::vector<VkPipelineShaderStageCreateInfo> shader_stage_vec;

    for (const std::string& shader_name : shader_name_list)
    {
        const VkPipelineShaderStageCreateInfo create_info {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0x0,
            .stage = get_shader_stage(shader_name),
            .module = create_shader_module(shader_root_path, shader_name),
            .pName = "main",
            .pSpecializationInfo = nullptr,
        };

        shader_stage_vec.push_back(create_info);
    }

    return shader_stage_vec;
}

static VkPipelineInputAssemblyStateCreateInfo create_input_assembly_state(const JSONInfo_SortBinPipelineState::State& sortbin_state)
{
    const VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0x0,
        .topology = sortbin_state.pipeline_state.input_assembly_state.topology,
        .primitiveRestartEnable = VK_FALSE,
    };

    return input_assembly_create_info;
}

static VkPipelineRasterizationStateCreateInfo create_rasterization_state(const JSONInfo_SortBinPipelineState::State& sortbin_state)
{
    const VkPipelineRasterizationStateCreateInfo rasterization_create_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = sortbin_state.pipeline_state.rasterization_state.polygon_mode,
        .cullMode = sortbin_state.pipeline_state.rasterization_state.cull_mode,
        .frontFace = sortbin_state.pipeline_state.rasterization_state.front_face,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.f,
        .depthBiasClamp = 0.f,
        .depthBiasSlopeFactor = 0.f,
        .lineWidth = 1.f,
    };

    return rasterization_create_info; 
}

static VkPipelineDepthStencilStateCreateInfo create_depth_stencil_state(const JSONInfo_SortBinPipelineState::State& sortbin_state)
{
    const VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = sortbin_state.pipeline_state.depth_stencil_state.depth_test_enable,
        .depthWriteEnable = sortbin_state.pipeline_state.depth_stencil_state.depth_write_enable,
        .depthCompareOp = sortbin_state.pipeline_state.depth_stencil_state.depth_compare_op,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = sortbin_state.pipeline_state.depth_stencil_state.stencil_test_enable
    };

    return depth_stencil_create_info;
}

static std::vector<VkVertexInputAttributeDescription> create_attrib_binding_vec(const JSONInfo_SortBinPipelineState::State& sortbin_state)
{
    std::vector<VkVertexInputAttributeDescription> attribute_description_vec;

    for (const auto& attrib : sortbin_state.pipeline_state.vertex_input_state.attribute_description_list)
    {
        attribute_description_vec.push_back(attrib.attribute_desctiption);
    }

    return attribute_description_vec;
}

static VkPipelineVertexInputStateCreateInfo create_vertex_input_state(const JSONInfo_SortBinPipelineState::State& sortbin_state, const std::vector<VkVertexInputAttributeDescription>& attrib_description_vec)
{
    const VkPipelineVertexInputStateCreateInfo vertex_input_create_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0x0,
        .vertexBindingDescriptionCount = static_cast<uint32_t>(sortbin_state.pipeline_state.vertex_input_state.binding_description_list.size()),
        .pVertexBindingDescriptions = sortbin_state.pipeline_state.vertex_input_state.binding_description_list.data(),
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attrib_description_vec.size()),
        .pVertexAttributeDescriptions = attrib_description_vec.data(),
    };

    return vertex_input_create_info;
}

static std::vector<VkPushConstantRange> create_push_const_ranges(const JSONInfo_SortBinReflection::State& sortbin_state)
{
    std::vector<VkPushConstantRange> push_const_range_vec;

    for (const JSONInfo_SortBinReflection::PushConstantState& push_const_state : sortbin_state.push_const_state_list)
    {
        const VkPushConstantRange push_const_range {
            .stageFlags = push_const_state.shader_stage_flags,
            .offset = push_const_state.offset,
            .size = push_const_state.size,
        };

        push_const_range_vec.push_back(push_const_range);
    }

    return push_const_range_vec;
}

static VkPipelineLayout create_sort_bin_pipeline_layout(const VkDescriptorSetLayout vk_handle_frame_desc_set_layout, const VkDescriptorSetLayout vk_handle_render_pass_desc_set_layout, const std::vector<VkPushConstantRange>& push_const_range_vec)
{
    std::vector<VkDescriptorSetLayout> desc_set_layout_vec { vk_handle_frame_desc_set_layout };

    if (vk_handle_render_pass_desc_set_layout != VK_NULL_HANDLE)
    {
        desc_set_layout_vec.push_back(vk_handle_render_pass_desc_set_layout);
    }

    const VkPipelineLayoutCreateInfo pipeline_layout_create_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0x0,
        .setLayoutCount = static_cast<uint32_t>(desc_set_layout_vec.size()),
        .pSetLayouts = desc_set_layout_vec.data(),
        .pushConstantRangeCount = static_cast<uint32_t>(push_const_range_vec.size()),
        .pPushConstantRanges = push_const_range_vec.data(),
    };

    return vk_core::create_pipeline_layout(pipeline_layout_create_info);
}

static std::vector<VkFormat> get_sort_bin_color_attachment_format_vec(const RenderPass& render_pass, const std::vector<RenderPass::Attachment>& render_attachment_vec)
{
    std::vector<VkFormat> color_attachment_format_vec;

    for (const RenderPass::WriteAttachmentPassInfo& attachment_pass_info : render_pass.write_color_attachment_pass_info_list)
    {
        const RenderPass::Attachment& attachment = render_attachment_vec[attachment_pass_info.attachment_idx];
        color_attachment_format_vec.push_back(attachment.format);
    }

    return color_attachment_format_vec;
}

static VkPipelineRenderingCreateInfo create_rendering_create_info(const std::vector<VkFormat>& color_attachment_format_vec, const VkFormat depth_attachment_format)
{
    const VkPipelineRenderingCreateInfo rendering_create_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .pNext = nullptr,
        .viewMask = 0,
        .colorAttachmentCount = static_cast<uint32_t>(color_attachment_format_vec.size()),
        .pColorAttachmentFormats = color_attachment_format_vec.data(),
        .depthAttachmentFormat = depth_attachment_format,
        .stencilAttachmentFormat = VK_FORMAT_UNDEFINED,
    };

    return rendering_create_info;
}

static std::vector<VkPipelineColorBlendAttachmentState> create_color_blend_attachment_state_vec(const RenderPass& render_pass)
{
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

    std::vector<VkPipelineColorBlendAttachmentState> color_blend_attachment_state_vec;

    for (const RenderPass::WriteAttachmentPassInfo& attachment_pass_info : render_pass.write_color_attachment_pass_info_list)
    {
        color_blend_attachment_state_vec.push_back(blend_state_none);
    }

    return color_blend_attachment_state_vec;
}

static VkPipelineColorBlendStateCreateInfo create_color_blend_state(const std::vector<VkPipelineColorBlendAttachmentState>& color_blend_attachment_state_vec)
{
    const VkPipelineColorBlendStateCreateInfo color_blend_state_create_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0x0,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = static_cast<uint32_t>(color_blend_attachment_state_vec.size()),
        .pAttachments = color_blend_attachment_state_vec.data(),
        .blendConstants = {0, 0, 0, 0},
    };

    return color_blend_state_create_info;
}

static std::unordered_map<std::string, DescriptorVariable> create_desc_var_umap(const std::vector<JSONInfo_DescriptorVariable>& json_desc_var_list)
{
    std::unordered_map<std::string, DescriptorVariable> desc_var_umap;

    for (const JSONInfo_DescriptorVariable& json_desc_var : json_desc_var_list)
    {
        std::unordered_map<std::string, InternalDesctriptorVariable> internal_desc_var_umap;

        for (const JSONInfo_DescriptorVariable& internal_json_desc_var : json_desc_var.internal_structure)
        {
            ASSERT(internal_json_desc_var.internal_structure.empty(), "Double internal descriptor structures are not supported!\n");

            const InternalDesctriptorVariable internal_desc_var {
                .name = internal_json_desc_var.name,
                .offset = internal_json_desc_var.offset,
                .size = internal_json_desc_var.size,
                .count = internal_json_desc_var.count,
            };

            internal_desc_var_umap.emplace(internal_json_desc_var.name, internal_desc_var);
        }

        DescriptorVariable desc_var {
            .name = json_desc_var.name,
            .offset = json_desc_var.offset,
            .size   = json_desc_var.size,
            .count = json_desc_var.count,
            .internal_structure = std::move(internal_desc_var_umap)
        };

        desc_var_umap.emplace(json_desc_var.name, desc_var);
    }

    return desc_var_umap;
}

static std::vector<SortBin> init_vec_sort_bin(
    const RendererState::CreateInfo& create_info,
    const std::vector<RenderPass>& render_pass_vec,
    const std::vector<RenderPass::Attachment> render_attachment_list,
    const std::unordered_map<std::string, uint16_t>& name_id_lut_render_pass,
    const VkDescriptorSetLayout vk_handle_frame_desc_set_layout)
{
    const auto json_data_app_state = read_json_file(create_info.file_app_state);
    const auto json_data_sort_bin_pipeline_state = read_json_file(create_info.file_sortbin_pipeline_state);
    const auto json_data_sort_bin_refl_state = read_json_file(create_info.refl_file_sortbin_mat_draw_def);

    const auto sort_bin_app_state_vec = json_data_app_state.at("sortbins").get<JSONInfo_AppSortBin>().sortbin_list;
    const auto sort_bin_pipeline_state_umap = json_data_sort_bin_pipeline_state.at("sortbins").get<JSONInfo_SortBinPipelineState>().state_umap;
    const auto sort_bin_reflection_state_umap = json_data_sort_bin_refl_state.at("sortbin-reflections").get<JSONInfo_SortBinReflection>().state_umap;

    std::vector<SortBin> sortbin_list;

    for (const auto& sort_bin_app_state : sort_bin_app_state_vec)
    {
        ASSERT(sort_bin_pipeline_state_umap.contains(sort_bin_app_state.name), "Sortbin %s not found in sortbin pipeline state file!\n", sort_bin_app_state.name.c_str());
        ASSERT(sort_bin_reflection_state_umap.contains(sort_bin_app_state.name), "Sortbin %s not found in sortbin reflection file!\n", sort_bin_app_state.name.c_str());
        ASSERT(name_id_lut_render_pass.contains(sort_bin_app_state.render_pass_name), "Render pass %s not found in render pass list!\n", sort_bin_app_state.render_pass_name.c_str());

        const uint16_t render_pass_ID = name_id_lut_render_pass.at(sort_bin_app_state.render_pass_name);

        ASSERT(render_pass_ID < render_pass_vec.size(), "Render pass ID %u out of bounds!\n", render_pass_ID);
        const RenderPass& render_pass = render_pass_vec[render_pass_ID];

        const auto& sort_bin_pipeline_state = sort_bin_pipeline_state_umap.at(sort_bin_app_state.name);
        const auto& sort_bin_reflection_state = sort_bin_reflection_state_umap.at(sort_bin_app_state.name);

        // Default States
        const auto viewport_vec = create_viewport(create_info);
        const auto scissors_vec = create_scissor(create_info);
        const auto attrib_binding_vec = create_attrib_binding_vec(sort_bin_pipeline_state);
        const auto viewport_state = create_viewport_state(viewport_vec, scissors_vec);
        const auto multisample_state = create_multisample_state();

        // User-Specified States
        const auto shader_stage_vec = create_shader_stage_vec(sort_bin_pipeline_state.pipeline_state.shader_state.shader_names, create_info.path_shader_root);
        const auto input_assembly_state = create_input_assembly_state(sort_bin_pipeline_state);
        const auto rasterization_state = create_rasterization_state(sort_bin_pipeline_state);
        const auto depth_stencil_state = create_depth_stencil_state(sort_bin_pipeline_state);
        const auto vertex_input_state = create_vertex_input_state(sort_bin_pipeline_state, attrib_binding_vec);
        const auto push_const_range_vec = create_push_const_ranges(sort_bin_reflection_state);
        const auto vk_handle_pipeline_layout = create_sort_bin_pipeline_layout(vk_handle_frame_desc_set_layout, render_pass.get_desc_set_layout(), push_const_range_vec);

        // Rendering Info
        const auto color_attachment_format_vec = get_sort_bin_color_attachment_format_vec(render_pass, render_attachment_list);
        const VkFormat depth_attachment_format = render_pass.write_depth_attachment_pass_info.has_value() ? render_attachment_list[render_pass.write_depth_attachment_pass_info->attachment_idx].format : VK_FORMAT_UNDEFINED;
        const auto rendering_create_info = create_rendering_create_info(color_attachment_format_vec, depth_attachment_format);

        // Blending Info
        const auto color_blend_attachment_state_vec = create_color_blend_attachment_state_vec(render_pass);
        const auto color_blend_state = create_color_blend_state(color_blend_attachment_state_vec);
        
        // Pipeline Creation
        const VkGraphicsPipelineCreateInfo graphics_pipeline_create_info {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = &rendering_create_info,
            .flags = 0x0,
            .stageCount = static_cast<uint32_t>(shader_stage_vec.size()),
            .pStages = shader_stage_vec.data(),
            .pVertexInputState = &vertex_input_state,
            .pInputAssemblyState = &input_assembly_state,
            .pTessellationState = nullptr,
            .pViewportState = &viewport_state,
            .pRasterizationState = &rasterization_state,
            .pMultisampleState = &multisample_state,
            .pDepthStencilState = &depth_stencil_state,
            .pColorBlendState = &color_blend_state,
            .pDynamicState = nullptr,
            .layout = vk_handle_pipeline_layout,
            .renderPass = VK_NULL_HANDLE,
            .subpass = 0,
            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = 0,
        };

        SortBin sortbin {
            .name = sort_bin_app_state.name,
            .descriptor_variable_material_umap = create_desc_var_umap(sort_bin_reflection_state.definition_material_data.members),
            .descriptor_variable_draw_umap = create_desc_var_umap(sort_bin_reflection_state.definition_draw_data.members),
            .material_data_block_size = sort_bin_reflection_state.definition_material_data.size,
            .material_data_block_end_padding_size = sort_bin_reflection_state.definition_material_data.end_padding,
            .draw_data_block_size = sort_bin_reflection_state.definition_draw_data.size,
            .draw_data_block_end_padding_size = sort_bin_reflection_state.definition_draw_data.end_padding,
            .vk_handle_pipeline = vk_core::create_graphics_pipeline(graphics_pipeline_create_info),
            .vk_handle_pipeline_layout = vk_handle_pipeline_layout
        };

        sortbin_list.push_back(std::move(sortbin));

        for (const VkPipelineShaderStageCreateInfo& shader_stage_create_info : shader_stage_vec)
        {
            vk_core::destroy_shader_module(shader_stage_create_info.module);
        }
    }

    return sortbin_list;
}

static std::unique_ptr<UniformBuffer> create_frame_ubo(const RendererState::CreateInfo& create_info, const std::string& ubo_name)
{
    const auto json_data_frame_desc_refl = read_json_file(create_info.refl_file_frame_desc_set_def);
    const auto frame_desc_set_binding_vec = json_data_frame_desc_refl.at("bindings").get<std::vector<JSONInfo_DescriptorBinding>>();

    uint64_t size = 0lu;
    std::unordered_map<std::string, DescriptorVariable> desc_var_umap;

    for (const auto& binding_state : frame_desc_set_binding_vec)
    {
        if (binding_state.name == ubo_name)
        {
            for (const auto& desc_var : binding_state.buffer_variables)
            {
                size += desc_var.size;
            }
            desc_var_umap = create_desc_var_umap(binding_state.buffer_variables);
            break;
        }
    }

    ASSERT(size != 0lu, "Uniform Buffer %s not found in frame descriptor reflection file!\n", ubo_name.c_str());

    return std::make_unique<UniformBuffer>(create_info.frame_resource_count, size, std::move(desc_var_umap));
}

static void update_frame_desc_sets(const uint32_t frame_resource_count, const UniformBuffer* frame_uniform_buffer, const BufferPool_VariableBlock* material_data_buffer, const BufferPool_VariableBlock* draw_data_buffer, const UniformBuffer* frame_fwd_light_ubo, const std::vector<VkDescriptorSet>& vk_handle_desc_set_list)
{
    for (uint32_t i = 0; i < frame_resource_count; i++)
    {
        const VkDescriptorBufferInfo frame_ubo_desc_buffer_info = frame_uniform_buffer->get_descriptor_buffer_info(i);
        const VkDescriptorBufferInfo mat_ssbo_desc_buffer_info = material_data_buffer->get_descriptor_buffer_info(i);
        const VkDescriptorBufferInfo draw_ssbo_desc_buffer_info = draw_data_buffer->get_descriptor_buffer_info(i);
        const VkDescriptorBufferInfo frame_fwd_light_ubo_desc_buffer_info = frame_fwd_light_ubo->get_descriptor_buffer_info(i);

        const std::array<VkWriteDescriptorSet, 4> write_desc_set_list {{
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
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = vk_handle_desc_set_list[i],
                .dstBinding = 3,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pImageInfo = nullptr,
                .pBufferInfo = &frame_fwd_light_ubo_desc_buffer_info,
                .pTexelBufferView = nullptr,
            },
        }};

        vk_core::update_desc_sets(static_cast<uint32_t>(write_desc_set_list.size()), write_desc_set_list.data(), 0, nullptr);
    }
}
