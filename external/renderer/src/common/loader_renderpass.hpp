#ifndef RENDERER_LOADER_RENDERPASS_HPP
#define RENDERER_LOADER_RENDERPASS_HPP

#include "defines.hpp"
#include "vk_enum_to_string.hpp"
#include "loader_shared.hpp"
#include "RenderPass.hpp"

#include "json.hpp"

#include <vulkan/vulkan.h>

#include <optional>
#include <vector>

struct JSONInfo_RenderAttachment
{
    struct ImageState
    {
        std::optional<VkFormat> format;
        std::optional<VkSampleCountFlagBits> num_samples;
        std::optional<VkImageTiling> tiling;
        std::optional<VkImageUsageFlags> usage;
        std::optional<VkSharingMode> sharing_mode;
        std::optional<VkImageLayout> initial_layout;
    };

    ImageState shared_image_state;

    std::vector<ImageState> image_state_list;
};

void from_json(const nlohmann::json& json_data, JSONInfo_RenderAttachment& info)
{
    if (json_data.contains("shared-state"))
    {
        const auto& json_data_shared_state = json_data.at("shared-state"); 

        if (json_data_shared_state.contains("num-samples"))
        {
            info.shared_image_state.num_samples = uint32_to_enum_VkSampleCountFlagBits(json_data_shared_state.at("num-samples").get<uint32_t>());
        }

        if (json_data_shared_state.contains("tiling"))
        {
            info.shared_image_state.tiling = string_to_enum_VkImageTiling(json_data_shared_state.at("tiling").get<std::string>());
        }

        if (json_data_shared_state.contains("sharing-mode"))
        {
            info.shared_image_state.sharing_mode = string_to_enum_VkSharingMode(json_data_shared_state.at("sharing-mode").get<std::string>());
        }

        if (json_data_shared_state.contains("initial-layout"))
        {
            info.shared_image_state.initial_layout = string_to_enum_VkImageLayout(json_data_shared_state.at("initial-layout").get<std::string>());
        }
    }
    else
    {
        EXIT("'shared-state' missing from 'render-attachments' object in json config file!\n");
    }

    if (json_data.contains("render-attachment-list"))
    {
        const auto& json_data_render_attachment_list = json_data.at("render-attachment-list"); 

        for (const auto& json_attachment_info : json_data_render_attachment_list)
        {
            info.image_state_list.push_back({});

            // Shared

            if (!info.shared_image_state.num_samples.has_value())
            {
                if (json_attachment_info.contains("num-samples"))
                {
                    info.image_state_list.back().num_samples = uint32_to_enum_VkSampleCountFlagBits(json_attachment_info.at("num-samples").get<uint32_t>());
                }
                else
                {
                    EXIT("'num-samples' missing from 'render-attachments-list' image!\n");
                }
            }

            if (!info.shared_image_state.tiling.has_value())
            {
                if (json_attachment_info.contains("tiling"))
                {
                    info.image_state_list.back().tiling = string_to_enum_VkImageTiling(json_attachment_info.at("tiling").get<std::string>());
                }
                else
                {
                    EXIT("'tiling' missing from 'render-attachments-list' image!\n");
                }
            }

            if (!info.shared_image_state.sharing_mode.has_value())
            {
                if (json_attachment_info.contains("sharing-mode"))
                {
                    info.image_state_list.back().sharing_mode = string_to_enum_VkSharingMode(json_attachment_info.at("sharing-mode").get<std::string>());
                }
                else
                {
                    EXIT("'sharing-mode' missing from 'render-attachments-list' image!\n");
                }
            }

            if (!info.shared_image_state.initial_layout.has_value())
            {
                if (json_attachment_info.contains("initial-layout"))
                {
                    info.image_state_list.back().initial_layout = string_to_enum_VkImageLayout(json_attachment_info.at("initial-layout").get<std::string>());
                }
                else
                {
                    EXIT("'sharing-mode' missing from 'render-attachments-list' image!\n");
                }
            }

            // Unique 

            if (json_attachment_info.contains("format"))
            {
                info.image_state_list.back().format = string_to_enum_VkFormat(json_attachment_info.at("format").get<std::string>());
            }
            else
            {
                EXIT("'format' missing from 'render-attachments-list' image!\n");
            }

            if (json_attachment_info.contains("usage"))
            {
                const auto& usage_vec = json_attachment_info.at("usage");

                VkImageUsageFlags flags = 0x0;

                for (const std::string& str_usage : usage_vec)
                {
                    flags |= string_to_enum_VkImageUsageFlags(str_usage);
                }

                info.image_state_list.back().usage = flags;
            }
        }
    }
    else
    {
        EXIT("'render-attachment-list' missing from 'render-attachments' object in json config file!\n");
    }
};


struct JSONInfo_RenderPass
{
    struct ReadAttachmentState
    {
        uint32_t id;
        VkImageLayout image_layout;
    };

    struct WriteAttachmentState
    {
        uint32_t id;
        VkImageLayout image_layout;
        VkAttachmentLoadOp load_op;
        VkAttachmentStoreOp store_op;
        VkClearValue clear_value;
    };

    struct State
    {
        uint32_t id;
        std::string name;
        std::vector<ReadAttachmentState> input_attachment_list;
        std::vector<WriteAttachmentState> color_attachment_list;
        WriteAttachmentState depth_attachment;
    };

    std::vector<State> state_list;
};

void from_json(const nlohmann::json& json_data, JSONInfo_RenderPass::ReadAttachmentState& info)
{
    info.id = json_data.at("id").get<uint32_t>();
    info.image_layout = string_to_enum_VkImageLayout(json_data.at("image-layout").get<std::string>());
}

void from_json(const nlohmann::json& json_data, JSONInfo_RenderPass::WriteAttachmentState& info)
{
    info.id = json_data.at("id").get<uint32_t>();
    info.image_layout = string_to_enum_VkImageLayout(json_data.at("image-layout").get<std::string>());
    info.load_op = string_to_enum_VkAttachmentLoadOp(json_data.at("load-op").get<std::string>());
    info.store_op = string_to_enum_VkAttachmentStoreOp(json_data.at("store-op").get<std::string>());

    const auto& json_clear_value = json_data.at("clear-value");

    if (json_clear_value.contains("color"))
    {
        info.clear_value.color.float32[0] = json_clear_value.at("color")[0];
        info.clear_value.color.float32[1] = json_clear_value.at("color")[1];
        info.clear_value.color.float32[2] = json_clear_value.at("color")[2];
        info.clear_value.color.float32[3] = json_clear_value.at("color")[3];
    }
    else
    {
        info.clear_value.depthStencil.depth = json_clear_value.at("depth").get<float>();
    }
}

void from_json(const nlohmann::json& json_data, JSONInfo_RenderPass::State& info)
{
    info.id = json_data.at("id").get<uint32_t>();
    info.name = json_data.at("name").get<std::string>();
    info.input_attachment_list = json_data.at("input-attachments");
    info.color_attachment_list = json_data.at("color-attachments");
    info.depth_attachment = json_data.at("depth-attachment");
}

void from_json(const nlohmann::json& json_data, JSONInfo_RenderPass& info)
{
    info.state_list = json_data;
}

struct JSONInfo_AppSortBin
{
    struct State
    {
        uint32_t id;
        std::string name;
        uint32_t render_pass_id;
    };

    std::vector<State> sortbin_list;
};

void from_json(const nlohmann::json& json_data, JSONInfo_AppSortBin::State& info)
{
    info.id = json_data.at("id").get<uint32_t>();
    info.name = json_data.at("name").get<std::string>();
    info.render_pass_id = json_data.at("renderpass-id").get<uint32_t>();
}

void from_json(const nlohmann::json& json_data, JSONInfo_AppSortBin& info)
{
    info.sortbin_list = json_data;
}

#endif // RENDERER_LOADER_RENDERPASS_HPP