#include "RenderPass.hpp"
#include "internal/pod/DrawInfo.hpp"
#include "vk_core.hpp"

static VkRenderingAttachmentInfo create_rendering_attachment_info(const uint32_t frame_idx, const std::vector<RenderPass::Attachment>& render_attachments, const RenderPass::WriteAttachmentPassInfo& attachment_pass_info);
static std::vector<VkRenderingAttachmentInfo> create_color_attachment_info_list(const uint32_t frame_idx, const std::vector<RenderPass::Attachment>& render_attachments, const std::vector<RenderPass::WriteAttachmentPassInfo> color_attachment_pass_info_list);
static void record_draws(const VkCommandBuffer vk_handle_cmd_buff, const std::vector<DrawInfo>& draw_list, const VkIndexType index_type);
static void record_sortbin_draws(const VkCommandBuffer vk_handle_cmd_buff, const std::vector<SortBin>& sortbins, const std::vector<uint16_t>& supported_sortbin_ids, const std::array<VkBuffer, 3>& vk_handle_index_buffer_list, const VkDescriptorSet vk_handle_frame_desc_set, const VkDescriptorSet vk_handle_render_pass_desc_set);

uint32_t RenderPass::s_input_attachment_count = 0u;
VkSampler RenderPass::s_vk_handle_input_attachment_sampler = VK_NULL_HANDLE;

uint32_t RenderPass::get_input_attachment_count() { return s_input_attachment_count; }

void RenderPass::create_input_attachment_sampler()
{
    const VkSamplerCreateInfo sampler_create_info {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0x0,
        .magFilter = VK_FILTER_NEAREST,
        .minFilter = VK_FILTER_NEAREST,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_FALSE,
        .maxAnisotropy = 1.0f,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .minLod = 0.0f,
        .maxLod = 0.0f,
        .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE
    };

    s_vk_handle_input_attachment_sampler = vk_core::create_sampler(sampler_create_info);
}

RenderPass::Attachment::Attachment(const VkImageCreateInfo& image_create_info, VkImageViewCreateInfo image_view_create_info, const uint32_t frame_resource_count)
    : extent { image_create_info.extent }
    , format { image_create_info.format }
    , mip_count { image_create_info.mipLevels }
    , layer_count { image_create_info.arrayLayers }
{
    for ( uint32_t i = 0; i < frame_resource_count; i++)
    {
        const VkImage vk_handle_image = vk_core::create_image(image_create_info);
        const VkDeviceMemory vk_handle_image_memory = vk_core::allocate_image_memory(vk_handle_image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        vk_core::bind_image_memory(vk_handle_image, vk_handle_image_memory);

        image_view_create_info.image = vk_handle_image;

        const VkImageView vk_handle_image_view = vk_core::create_image_view(image_view_create_info);

        vk_handle_image_list.push_back(vk_handle_image);
        vk_handle_image_view_list.push_back(vk_handle_image_view);
        vk_handle_image_memory_list.push_back(vk_handle_image_memory);
    }
}

RenderPass::RenderPass(const InitInfo&& init_info)
    : supported_sortbin_id_list { std::move(init_info.supported_sortbin_id_list) }
    , read_attachment_pass_info_list { std::move(init_info.read_attachment_pass_info_list) }
    , write_color_attachment_pass_info_list { std::move(init_info.write_color_attachment_pass_info_list) }
    , write_depth_attachment_pass_info { std::move(init_info.write_depth_attachment_pass_info) }
{
    if (!init_info.read_attachment_pass_info_list.empty())
    {
        const VkDescriptorSetLayoutBinding desc_set_layout_binding = {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = static_cast<uint32_t>(init_info.read_attachment_pass_info_list.size()),
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr
        };

        const VkDescriptorSetLayoutCreateInfo desc_set_layout_create_info {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0x0,
            .bindingCount = 1,
            .pBindings = &desc_set_layout_binding 
        };

        m_vk_handle_desc_set_layout = vk_core::create_desc_set_layout(desc_set_layout_create_info);

        s_input_attachment_count += static_cast<uint32_t>(init_info.read_attachment_pass_info_list.size());
    }
}

void RenderPass::init_desc_sets(const uint32_t frame_resource_count, const VkDescriptorPool vk_handle_desc_pool, const std::vector<Attachment>& render_attachments)
{
    if ( m_vk_handle_desc_set_layout == VK_NULL_HANDLE )
    {
        return;
    } 

    const VkDescriptorSetAllocateInfo desc_set_alloc_info {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = vk_handle_desc_pool, 
        .descriptorSetCount = frame_resource_count,
        .pSetLayouts = &m_vk_handle_desc_set_layout
    };

    m_vk_handle_desc_set_list = vk_core::allocate_desc_sets(desc_set_alloc_info);

    for (uint32_t i = 0; i < frame_resource_count; i++)
    {
        std::vector<VkDescriptorImageInfo> image_info_list;

        for (const ReadAttachmentPassInfo& read_attachment_pass_info : read_attachment_pass_info_list)
        {
            const Attachment& attachment = render_attachments[read_attachment_pass_info.attachment_idx];

            image_info_list.push_back({
                .sampler = s_vk_handle_input_attachment_sampler,
                .imageView = attachment.vk_handle_image_view_list[i],
                .imageLayout = read_attachment_pass_info.image_layout
            });
        }

        const VkWriteDescriptorSet write_desc_set {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = m_vk_handle_desc_set_list[i],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = static_cast<uint32_t>(read_attachment_pass_info_list.size()),
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = image_info_list.data(),
            .pBufferInfo = nullptr,
            .pTexelBufferView = nullptr,
        };

        vk_core::update_desc_sets(1, &write_desc_set, 0, nullptr);
    }
}

void RenderPass::record(const RenderPass::RecordInfo& record_info) const 
{
    VkRenderingAttachmentInfo depth_rendering_attachment_info = {};
    if (write_depth_attachment_pass_info.has_value())
    {
        depth_rendering_attachment_info = 
            create_rendering_attachment_info(record_info.frame_idx, 
            record_info.global_attachment_list, 
            write_depth_attachment_pass_info.value());
    }

    const std::vector<VkRenderingAttachmentInfo> color_rendering_attachment_infos = 
        create_color_attachment_info_list(record_info.frame_idx,
        record_info.global_attachment_list, 
        write_color_attachment_pass_info_list);

    const VkRenderingInfo rendering_info {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .pNext = nullptr,
        .flags = 0x0,
        .renderArea = record_info.render_area,
        .layerCount = 1,
        .viewMask = 0x0,
        .colorAttachmentCount = static_cast<uint32_t>(color_rendering_attachment_infos.size()),
        .pColorAttachments = color_rendering_attachment_infos.data(),
        .pDepthAttachment = write_depth_attachment_pass_info.has_value() ? &depth_rendering_attachment_info : nullptr,
        .pStencilAttachment = nullptr,
    };

    vkCmdBeginRendering(record_info.vk_handle_cmd_buff, &rendering_info);

    record_sortbin_draws(
        record_info.vk_handle_cmd_buff, 
        record_info.global_sortbin_list,
        supported_sortbin_id_list,
        record_info.vk_handle_index_buffer_list,
        record_info.vk_handle_global_desc_set,
        m_vk_handle_desc_set_layout != VK_NULL_HANDLE ? m_vk_handle_desc_set_list[record_info.frame_idx] : VK_NULL_HANDLE);

    vkCmdEndRendering(record_info.vk_handle_cmd_buff);
}

// Helper functions

static VkRenderingAttachmentInfo create_rendering_attachment_info(const uint32_t frame_idx, 
    const std::vector<RenderPass::Attachment>& render_attachments, 
    const RenderPass::WriteAttachmentPassInfo& attachment_pass_info)
{
    const auto& attachment = render_attachments[attachment_pass_info.attachment_idx];

    const VkRenderingAttachmentInfo attachment_info {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .pNext = nullptr,
        .imageView = attachment.vk_handle_image_view_list[frame_idx],
        .imageLayout = attachment_pass_info.image_layout,
        .resolveMode = VK_RESOLVE_MODE_NONE,
        .resolveImageView = VK_NULL_HANDLE,
        .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .loadOp = attachment_pass_info.load_op,
        .storeOp = attachment_pass_info.store_op,
        .clearValue = attachment_pass_info.clear_value
    };

    return attachment_info;
}

static std::vector<VkRenderingAttachmentInfo> create_color_attachment_info_list(const uint32_t frame_idx,
    const std::vector<RenderPass::Attachment>& render_attachments, 
    const std::vector<RenderPass::WriteAttachmentPassInfo> color_attachment_pass_info_list)
{
    std::vector<VkRenderingAttachmentInfo> color_rendering_attachment_info_list;

    for (const RenderPass::WriteAttachmentPassInfo& attachment_pass_info : color_attachment_pass_info_list)
    {
        const VkRenderingAttachmentInfo attachment_info = create_rendering_attachment_info(frame_idx, render_attachments, attachment_pass_info);
        color_rendering_attachment_info_list.push_back(std::move(attachment_info));
    }

    return color_rendering_attachment_info_list;
}

static void record_draws(const VkCommandBuffer vk_handle_cmd_buff, 
    const std::vector<DrawInfo>& draw_list,
    const VkIndexType index_type)
{
    if (index_type == VK_INDEX_TYPE_MAX_ENUM)
    {
        for (const DrawInfo& draw_info : draw_list)
        {
            vkCmdDraw(vk_handle_cmd_buff, 
                draw_info.vertex_count,
                draw_info.instance_count,
                draw_info.first_vertex,
                draw_info.first_instance);
        }
    }
    else
    {
        for (const DrawInfo& draw_info : draw_list)
        {
            vkCmdDrawIndexed(vk_handle_cmd_buff,
                draw_info.index_count,
                draw_info.instance_count,
                draw_info.first_index,
                draw_info.vertex_offset,
                draw_info.first_instance);
        }
    }
}

static void record_sortbin_draws(const VkCommandBuffer vk_handle_cmd_buff,
    const std::vector<SortBin>& sortbins,
    const std::vector<uint16_t>& supported_sortbin_ids,
    const std::array<VkBuffer, 3>& vk_handle_index_buffer_list,
    const VkDescriptorSet vk_handle_frame_desc_set,
    const VkDescriptorSet vk_handle_render_pass_desc_set)
{
    if (sortbins.empty() || supported_sortbin_ids.empty())
    {
        return;
    }

    std::vector<VkDescriptorSet> vk_handle_desc_set_list { vk_handle_frame_desc_set };
    if ( vk_handle_render_pass_desc_set != VK_NULL_HANDLE )
    {
        vk_handle_desc_set_list.push_back(vk_handle_render_pass_desc_set);
    }

    vkCmdBindDescriptorSets(vk_handle_cmd_buff,
        VK_PIPELINE_BIND_POINT_GRAPHICS, 
        sortbins[supported_sortbin_ids[0]].vk_handle_pipeline_layout,
        0, 
        static_cast<uint32_t>(vk_handle_desc_set_list.size()), vk_handle_desc_set_list.data(),
        0, nullptr);

    for (const uint32_t sortbin_id : supported_sortbin_ids)
    {
        const SortBin& sortbin = sortbins[sortbin_id];

        vkCmdBindPipeline(vk_handle_cmd_buff, 
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            sortbin.vk_handle_pipeline);

        if (!sortbin.draw_list_u32.empty())
        {
            vkCmdBindIndexBuffer(vk_handle_cmd_buff, vk_handle_index_buffer_list[0], 0, VK_INDEX_TYPE_UINT32);
            record_draws(vk_handle_cmd_buff, sortbin.draw_list_u32, VK_INDEX_TYPE_UINT32);
        }

        if (!sortbin.draw_list_u16.empty())
        {
            vkCmdBindIndexBuffer(vk_handle_cmd_buff, vk_handle_index_buffer_list[1], 0, VK_INDEX_TYPE_UINT16);
            record_draws(vk_handle_cmd_buff, sortbin.draw_list_u16, VK_INDEX_TYPE_UINT16);
        }

        if (!sortbin.draw_list_u8.empty())
        {
            vkCmdBindIndexBuffer(vk_handle_cmd_buff, vk_handle_index_buffer_list[2], 0, VK_INDEX_TYPE_UINT8_EXT);
            record_draws(vk_handle_cmd_buff, sortbin.draw_list_u8, VK_INDEX_TYPE_UINT8_EXT);
        }

        if (!sortbin.draw_list.empty())
        {
            record_draws(vk_handle_cmd_buff, sortbin.draw_list, VK_INDEX_TYPE_MAX_ENUM);
        }
    }
}

