#ifndef JSON_STRUCTURES_HPP
#define JSON_STRUCTURES_HPP

#include "vk_enum_to_string.hpp"

#include <vulkan/vulkan.h>

#include <string>
#include <vector>

#include "json.hpp"

struct JSONInfo_DescriptorVariable
{
    std::string name;
    uint32_t offset;
    uint32_t size;
    uint32_t count;
    std::vector<JSONInfo_DescriptorVariable> internal_structure;

    struct Hash
    {
        size_t operator()(const JSONInfo_DescriptorVariable& info) const
        {
            return std::hash<std::string>()(info.name);
        }
    };

    bool operator==(const JSONInfo_DescriptorVariable& other) const
    {
        return name == other.name &&
               offset == other.offset &&
               size == other.size &&
               count == other.count;
               internal_structure == other.internal_structure;
    }
};

void from_json(const nlohmann::json& json_data, JSONInfo_DescriptorVariable& info)
{
    info.name = json_data.at("name").get<std::string>();
    info.offset = json_data.at("offset").get<uint32_t>();
    info.size = json_data.at("size").get<uint32_t>();
    info.count = json_data.at("count").get<uint32_t>();
    info.internal_structure = json_data.at("internal-structure").get<std::vector<JSONInfo_DescriptorVariable>>();
}

struct JSONInfo_DescriptorBinding
{
    std::string name;
    uint32_t binding_ID;
    VkDescriptorType descriptor_type;
    uint32_t descriptor_count;
    VkShaderStageFlags stage_flags;
    std::vector<JSONInfo_DescriptorVariable> buffer_variables;
};

void from_json(const nlohmann::json& json_data, JSONInfo_DescriptorBinding& info)
{
    info.name = json_data.at("name").get<std::string>();
    info.binding_ID = json_data.at("binding-id").get<uint32_t>();
    info.descriptor_type = string_to_enum_VkDescriptorType(json_data.at("descriptor-type").get<std::string>());
    info.descriptor_count = json_data.at("descriptor-count").get<uint32_t>();

    info.stage_flags = 0x0;
    for (const std::string& shader_stage_str : json_data.at("stage-flags"))
    {
        info.stage_flags |= string_to_enum_VkShaderStageFlags(shader_stage_str);
    }

    info.buffer_variables = json_data.at("members").get<std::vector<JSONInfo_DescriptorVariable>>();
}

struct JSONInfo_RenderAttachment
{
    struct ImageState
    {
        std::string name;
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

            info.image_state_list.back().name = json_attachment_info.at("name").get<std::string>();

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
        std::string name;
        VkImageLayout image_layout;
    };

    struct WriteAttachmentState
    {
        std::string name;
        VkImageLayout image_layout;
        VkAttachmentLoadOp load_op;
        VkAttachmentStoreOp store_op;
        VkClearValue clear_value;
    };

    struct State
    {
        std::string name;
        std::vector<ReadAttachmentState> input_attachment_list;
        std::vector<WriteAttachmentState> color_attachment_list;
        std::optional<WriteAttachmentState> depth_attachment;
    };

    std::vector<State> state_list;
};

void from_json(const nlohmann::json& json_data, JSONInfo_RenderPass::ReadAttachmentState& info)
{
    info.name = json_data.at("name").get<std::string>();
    info.image_layout = string_to_enum_VkImageLayout(json_data.at("image-layout").get<std::string>());
}

void from_json(const nlohmann::json& json_data, JSONInfo_RenderPass::WriteAttachmentState& info)
{
    info.name = json_data.at("name").get<std::string>();
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
    info.name = json_data.at("name").get<std::string>();
    info.input_attachment_list = json_data.at("input-attachments");
    info.color_attachment_list = json_data.at("color-attachments");

    if (json_data.contains("depth-attachment"))
    {
        info.depth_attachment = json_data.at("depth-attachment");
    }
    else
    {
        info.depth_attachment = std::nullopt;
    }
}

void from_json(const nlohmann::json& json_data, JSONInfo_RenderPass& info)
{
    info.state_list = json_data;
}







struct JSONInfo_SortBinReflection
{
    struct BlockDefinition
    {
        uint32_t size;
        uint32_t end_padding;
        std::vector<JSONInfo_DescriptorVariable> members;
    };

    struct PushConstantState
    {
        VkShaderStageFlags shader_stage_flags;
        uint32_t offset;
        uint32_t size;
    };

    struct State
    {
        std::string sortbin_name;
        BlockDefinition definition_material_data;
        BlockDefinition definition_draw_data;
        std::vector<PushConstantState> push_const_state_list;
    };

    std::unordered_map<std::string, State> state_umap;
};

void from_json(const nlohmann::json& json_data, JSONInfo_SortBinReflection::PushConstantState& info)
{

}

void from_json(const nlohmann::json& json_data, JSONInfo_SortBinReflection::BlockDefinition& info)
{
    info.size = json_data.at("block-size").get<uint32_t>();
    info.end_padding = json_data.at("end-padding").get<uint32_t>();
    info.members = json_data.at("members").get<std::vector<JSONInfo_DescriptorVariable>>();
}

void from_json(const nlohmann::json& json_data, JSONInfo_SortBinReflection::State& info)
{
    info.sortbin_name = json_data.at("name").get<std::string>();
    info.definition_material_data = json_data.at("definition-material-data");
    info.definition_draw_data = json_data.at("definition-draw-data");
    info.push_const_state_list = json_data.at("definition-push-const-data");
}

void from_json(const nlohmann::json& json_data, JSONInfo_SortBinReflection& info)
{
    const std::vector<JSONInfo_SortBinReflection::State>& state_list = json_data;

    info.state_umap = {};
    for (const JSONInfo_SortBinReflection::State& state : state_list)
    {
        const auto iter = info.state_umap.insert({state.sortbin_name, state});

        if (!iter.second)
        {
            EXIT("Sortbin %s already exists in sortbin reflection config file!\n", state.sortbin_name.c_str());
        }
    }
}





















struct JSONInfo_AppSortBin
{
    struct State
    {
        std::string name;
        std::string render_pass_name;
    };

    std::vector<State> sortbin_list;
};

void from_json(const nlohmann::json& json_data, JSONInfo_AppSortBin::State& info)
{
    info.name = json_data.at("name").get<std::string>();
    info.render_pass_name = json_data.at("render-pass-name").get<std::string>();
}

void from_json(const nlohmann::json& json_data, JSONInfo_AppSortBin& info)
{
    info.sortbin_list = json_data;
}



struct JSONInfo_SortBinPipelineState
{
    struct ShaderState
    {
        std::vector<std::string> shader_names; 
    };

    struct VertexInputAttributeDescription
    {
        std::string usage;
        VkVertexInputAttributeDescription attribute_desctiption;
    };

    struct VertexInputState
    {
        std::vector<VkVertexInputBindingDescription> binding_description_list;
        std::vector<VertexInputAttributeDescription> attribute_description_list;

        bool operator==(const VertexInputState& other) const
        {
            if (binding_description_list.size() != other.binding_description_list.size() ||
                attribute_description_list.size() != other.attribute_description_list.size())
            {
                return false;
            }

            for (size_t i = 0; i < binding_description_list.size(); ++i)
            {
                if (binding_description_list[i].binding != other.binding_description_list[i].binding ||
                    binding_description_list[i].stride != other.binding_description_list[i].stride ||
                    binding_description_list[i].inputRate != other.binding_description_list[i].inputRate)
                {
                    return false;
                }
            }

            for (size_t i = 0; i < attribute_description_list.size(); ++i)
            {
                if (attribute_description_list[i].usage != other.attribute_description_list[i].usage ||
                    attribute_description_list[i].attribute_desctiption.location != other.attribute_description_list[i].attribute_desctiption.location ||
                    attribute_description_list[i].attribute_desctiption.binding != other.attribute_description_list[i].attribute_desctiption.binding ||
                    attribute_description_list[i].attribute_desctiption.format != other.attribute_description_list[i].attribute_desctiption.format ||
                    attribute_description_list[i].attribute_desctiption.offset != other.attribute_description_list[i].attribute_desctiption.offset)
                {
                    return false;
                }
            }

            return true;
        }
    };

    struct InputAssemblyState
    {
        VkPrimitiveTopology topology;
    };

    struct RasterizationState
    {
        VkPolygonMode polygon_mode;
        VkCullModeFlags cull_mode;
        VkFrontFace front_face;
    };

    struct DepthStencilState
    {
        bool depth_test_enable;
        bool depth_write_enable;
        VkCompareOp depth_compare_op;
        bool stencil_test_enable;
    };
    
    struct PipelineState
    {
        ShaderState shader_state;
        VertexInputState vertex_input_state;
        InputAssemblyState input_assembly_state;
        RasterizationState rasterization_state;
        DepthStencilState depth_stencil_state;
    };

    struct State
    {
        std::string sortbin_name;
        PipelineState pipeline_state;
    };

    std::unordered_map<std::string, State> state_umap;
};

void from_json(const nlohmann::json& json_data, JSONInfo_SortBinPipelineState::ShaderState& info)
{
    info.shader_names = json_data;
}

void from_json(const nlohmann::json& json_data, VkVertexInputBindingDescription& info)
{
    info.binding = json_data.at("binding").get<uint32_t>();
    info.stride = json_data.at("stride").get<uint32_t>();
    info.inputRate = string_to_enum_VkVertexInputRate(json_data.at("input-rate").get<std::string>());
}

void from_json(const nlohmann::json& json_data, VkVertexInputAttributeDescription& info)
{
    info.location = json_data.at("location").get<uint32_t>();
    info.binding = json_data.at("binding").get<uint32_t>();
    info.format = string_to_enum_VkFormat(json_data.at("format").get<std::string>());
    info.offset = json_data.at("offset").get<uint32_t>();
}

void from_json(const nlohmann::json& json_data, JSONInfo_SortBinPipelineState::VertexInputState& info)
{
    info.binding_description_list = json_data.at("vertex-input-binding-desc");

    for (const auto& json_attribute_desc : json_data.at("vertex-input-attrib-desc"))
    {
        info.attribute_description_list.push_back({});
        info.attribute_description_list.back().usage = json_attribute_desc.at("usage").get<std::string>();
        info.attribute_description_list.back().attribute_desctiption.location = json_attribute_desc.at("location").get<uint32_t>();
        info.attribute_description_list.back().attribute_desctiption.binding = json_attribute_desc.at("binding").get<uint32_t>();
        info.attribute_description_list.back().attribute_desctiption.format = string_to_enum_VkFormat(json_attribute_desc.at("format").get<std::string>());
        info.attribute_description_list.back().attribute_desctiption.offset = json_attribute_desc.at("offset").get<uint32_t>();
    }
}

void from_json(const nlohmann::json& json_data, JSONInfo_SortBinPipelineState::InputAssemblyState& info)
{
    info.topology = string_to_enum_VkPrimitiveTopology(json_data.at("topology").get<std::string>());
}

void from_json(const nlohmann::json& json_data, JSONInfo_SortBinPipelineState::RasterizationState& info)
{
    info.polygon_mode = string_to_enum_VkPolygonMode(json_data.at("polygon-mode").get<std::string>());
    info.cull_mode = string_to_enum_VkCullModeFlagBits(json_data.at("cull-mode").get<std::string>());
    info.front_face = string_to_enum_VkFrontFace(json_data.at("front-face").get<std::string>());
}

void from_json(const nlohmann::json& json_data, JSONInfo_SortBinPipelineState::DepthStencilState& info)
{
    info.depth_test_enable = json_data.at("depth-test-enable").get<bool>();
    info.depth_write_enable = json_data.at("depth-write-enable").get<bool>();
    info.depth_compare_op = string_to_enum_VkCompareOp(json_data.at("depth-compare-op").get<std::string>());
    info.stencil_test_enable = json_data.at("stencil-test-enable").get<bool>();
}

void from_json(const nlohmann::json& json_data, JSONInfo_SortBinPipelineState::PipelineState& info)
{
    info.shader_state = json_data.at("shader-state");
    info.vertex_input_state = json_data.at("vertex-input-state");
    info.input_assembly_state = json_data.at("input-assembly-state");
    info.rasterization_state = json_data.at("rasterization-state");
    info.depth_stencil_state = json_data.at("depth-stencil-state");
}

void from_json(const nlohmann::json& json_data, JSONInfo_SortBinPipelineState::State& info)
{
    info.sortbin_name = json_data.at("name").get<std::string>();
    info.pipeline_state = json_data.at("pipeline-state");
}

void from_json(const nlohmann::json& json_data, JSONInfo_SortBinPipelineState& info)
{
    const std::vector<JSONInfo_SortBinPipelineState::State>& state_list = json_data;

    info.state_umap = {};
    for (const JSONInfo_SortBinPipelineState::State& state : state_list)
    {
        info.state_umap.insert({state.sortbin_name, state});
    }
}




#endif // JSON_STRUCTURES_HPP