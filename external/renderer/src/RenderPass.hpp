#ifndef RENDERER_RENDER_PASS_HPP
#define RENDERER_RENDER_PASS_HPP

#include "internal/pod/SortBin.hpp"

#include  <vulkan/vulkan.h>

#include <vector>
#include <unordered_map>
#include <string>
#include <memory>
#include <optional>

struct RenderPass
{
private:
    static uint32_t s_input_attachment_count;
    static VkSampler s_vk_handle_input_attachment_sampler;

    VkDescriptorSetLayout m_vk_handle_desc_set_layout = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> m_vk_handle_desc_set_list; // size = N frame resources
public:

    static uint32_t get_input_attachment_count();
    static void create_input_attachment_sampler();

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

    struct ReadAttachmentPassInfo
    {
        uint32_t attachment_idx = 0u;
        VkImageLayout image_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    };

    struct WriteAttachmentPassInfo
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

    struct InitInfo {
        uint32_t frame_resource_count;
        std::vector<uint16_t> supported_sortbin_id_list;
        std::vector<ReadAttachmentPassInfo> read_attachment_pass_info_list;
        std::vector<WriteAttachmentPassInfo> write_color_attachment_pass_info_list;
        std::optional<WriteAttachmentPassInfo> write_depth_attachment_pass_info;
    };

    explicit RenderPass(const InitInfo&& init_info);

    void init_desc_sets(const uint32_t frame_resource_count, const VkDescriptorPool vk_handle_desc_pool, const std::vector<Attachment>& render_attachments);

    void record(const RecordInfo& record_info) const;

    VkDescriptorSetLayout get_desc_set_layout() const { return m_vk_handle_desc_set_layout; }

    const std::vector<uint16_t> supported_sortbin_id_list;
    const std::vector<ReadAttachmentPassInfo> read_attachment_pass_info_list;
    const std::vector<WriteAttachmentPassInfo> write_color_attachment_pass_info_list;
    const std::optional<WriteAttachmentPassInfo> write_depth_attachment_pass_info;
};

#endif // RENDERER_RENDER_PASS_HPP