#ifndef RENDERER_RENDER_PASS_HPP
#define RENDERER_RENDER_PASS_HPP

#include "Renderable.hpp"
#include "DescriptorVariable.hpp"
#include "vk_core.hpp"

#include  <vulkan/vulkan.h>

#include <vector>
#include <unordered_set>
#include <bitset>
#include <memory>

struct SortBin
{
    const VkPipeline vk_handle_pipeline;
    const VkPipelineLayout vk_handle_pipeline_layout;
    const uint8_t vertex_type;
    const uint8_t per_draw_data_type;

    VkDescriptorSet vk_handle_desc_set;
    
    std::vector<renderer::DrawInfo> draw_list_u32;
    std::vector<renderer::DrawInfo> draw_list_u16;
    std::vector<renderer::DrawInfo> draw_list_u8;
    std::vector<renderer::DrawInfo> draw_list;

    std::vector<VkDescriptorSetLayout> vk_handle_desc_set_layout_list;
    std::unordered_set<DescriptorVariable, DescriptorVariable::Hash> descriptor_variable_material_set;
    std::unordered_set<DescriptorVariable, DescriptorVariable::Hash> descriptor_variable_draw_set;

    uint64_t material_data_block_size = 12; // should be 0, change back
    uint64_t draw_data_block_size = 4; // By default always constains material ID
};

struct RenderPass
{
public:

    struct Attachment
    {
        explicit Attachment(const VkImageCreateInfo& image_create_info, VkImageViewCreateInfo image_view_create_info, const uint32_t frame_resource_count);

        VkExtent3D extent = { 0, 0, 0 };
        VkFormat format = VK_FORMAT_UNDEFINED;
        uint32_t mip_count = 0u;
        uint32_t layer_count = 0u;

        std::vector<VkImage> vk_handle_image_list;
        std::vector<VkImageView> vk_handle_image_view_list;
        std::vector<VkDeviceMemory> vk_handle_image_memory_list;
    };

    struct AttachmentPassInfo
    {
        uint32_t attachment_idx = 0u;
        VkImageLayout image_layout = VK_IMAGE_LAYOUT_UNDEFINED;
        VkAttachmentLoadOp load_op = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        VkAttachmentStoreOp store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        VkClearValue clear_value = { .color = { 0, 0, 0, 0 } };
    };

    struct RecordInfo
    {
        const uint32_t frame_idx;
        const VkCommandBuffer vk_handle_cmd_buff;
        const std::vector<Attachment> global_attachment_list;
        const std::vector<SortBin>& global_sortbin_list;
        const VkRect2D render_area;
        const std::array<VkBuffer, 3> vk_handle_index_buffer_list;
        const VkDescriptorSet vk_handle_global_desc_set;
    };

    RenderPass(const std::vector<uint32_t>&& _supported_sortbin_id_list,
        const std::vector<AttachmentPassInfo>&& _color_attachment_pass_info_list,
        const AttachmentPassInfo&& _depth_attachment_pass_info);

    void register_depth_attachment();
    void register_color_attachment();

    void record(const RecordInfo& record_info) const;

    const std::vector<uint32_t> supported_sortbin_id_list;
    const std::vector<AttachmentPassInfo> color_attachment_pass_info_list;
    const AttachmentPassInfo depth_attachment_pass_info;
};

static VkRenderingAttachmentInfo create_rendering_attachment_info(const uint32_t frame_idx, 
    const std::vector<RenderPass::Attachment>& render_attachments, 
    const RenderPass::AttachmentPassInfo& attachment_pass_info)
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
    const std::vector<RenderPass::AttachmentPassInfo> color_attachment_pass_info_list)
{
    std::vector<VkRenderingAttachmentInfo> color_rendering_attachment_info_list;

    for (const RenderPass::AttachmentPassInfo& attachment_pass_info : color_attachment_pass_info_list)
    {
        const VkRenderingAttachmentInfo attachment_info = create_rendering_attachment_info(frame_idx, render_attachments, attachment_pass_info);
        color_rendering_attachment_info_list.push_back(std::move(attachment_info));
    }

    return color_rendering_attachment_info_list;
}


namespace
{

static void record_draws(const VkCommandBuffer vk_handle_cmd_buff, 
    const std::vector<renderer::DrawInfo>& draw_list,
    const VkIndexType index_type)
{
    if (index_type == VK_INDEX_TYPE_MAX_ENUM)
    {
        for (const renderer::DrawInfo& draw_info : draw_list)
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
        for (const renderer::DrawInfo& draw_info : draw_list)
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
    const std::vector<uint32_t>& supported_sortbin_ids,
    const std::array<VkBuffer, 3>& vk_handle_index_buffer_list,
    const VkDescriptorSet vk_handle_global_desc_set)
{
    if (sortbins.empty() || supported_sortbin_ids.empty())
    {
        return;
    }

        
    for (const uint32_t sortbin_id : supported_sortbin_ids)
    {
        const SortBin& sortbin = sortbins[sortbin_id];

        vkCmdBindDescriptorSets(vk_handle_cmd_buff,
            VK_PIPELINE_BIND_POINT_GRAPHICS, 
            sortbins[supported_sortbin_ids[0]].vk_handle_pipeline_layout,
            0, 1, 
            &vk_handle_global_desc_set,
            0, nullptr);

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

}; // end anonymous namespace

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

RenderPass::RenderPass(const std::vector<uint32_t>&& _supported_sortbin_id_list,
    const std::vector<AttachmentPassInfo>&& _color_attachment_pass_info_list,
    const AttachmentPassInfo&& _depth_attachment_pass_info)
    : supported_sortbin_id_list { std::move(_supported_sortbin_id_list) }
    , color_attachment_pass_info_list { std::move(_color_attachment_pass_info_list) }
    , depth_attachment_pass_info { std::move(_depth_attachment_pass_info) }
{
}

void RenderPass::record(const RenderPass::RecordInfo& record_info) const 
{
    const VkRenderingAttachmentInfo depth_rendering_attachment_info = 
        create_rendering_attachment_info(record_info.frame_idx, 
        record_info.global_attachment_list, 
        depth_attachment_pass_info);

    const std::vector<VkRenderingAttachmentInfo> color_rendering_attachment_infos = 
        create_color_attachment_info_list(record_info.frame_idx,
        record_info.global_attachment_list, 
        color_attachment_pass_info_list);

    const VkRenderingInfo rendering_info {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .pNext = nullptr,
        .flags = 0x0,
        .renderArea = record_info.render_area,
        .layerCount = 1,
        .viewMask = 0x0,
        .colorAttachmentCount = static_cast<uint32_t>(color_rendering_attachment_infos.size()),
        .pColorAttachments = color_rendering_attachment_infos.data(),
        .pDepthAttachment = &depth_rendering_attachment_info,
        .pStencilAttachment = nullptr,
    };

    vkCmdBeginRendering(record_info.vk_handle_cmd_buff, &rendering_info);

    record_sortbin_draws(record_info.vk_handle_cmd_buff, record_info.global_sortbin_list, supported_sortbin_id_list, record_info.vk_handle_index_buffer_list, record_info.vk_handle_global_desc_set);

    vkCmdEndRendering(record_info.vk_handle_cmd_buff);
}

#endif // RENDERER_RENDER_PASS_HPP