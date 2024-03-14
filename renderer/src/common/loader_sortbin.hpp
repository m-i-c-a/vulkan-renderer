#ifndef RENDERER_LOADER_SORTBIN_HPP
#define RENDERER_LOADER_SORTBIN_HPP

#include "defines.hpp"
#include "vk_enum_to_string.hpp"
#include "loader_shared.hpp"
#include "RenderPass.hpp"

#include "vk_core.hpp"

#include <vulkan/vulkan.h>

#include <fstream>
#include <string>
#include <unordered_set>
#include <unordered_map>


#include "json.hpp"

struct JSONInfo_SortBin
{
    struct ShaderState
    {
        std::vector<std::string> shader_names; 
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
        InputAssemblyState input_assembly_state;
        RasterizationState rasterization_state;
        DepthStencilState depth_stencil_state;
    };

    struct State
    {
        std::string sortbin_name;
        PipelineState pipeline_state;

        bool operator==(const State& other) const
        {
            return sortbin_name == other.sortbin_name;
        };

        struct Hash
        {
            std::size_t operator()(const State& obj) const
            {
                return std::hash<std::string>()(obj.sortbin_name);
            }
        };
    };

    std::unordered_map<std::string, State> state_umap;
};

void from_json(const nlohmann::json& json_data, JSONInfo_SortBin::ShaderState& info)
{
    info.shader_names = json_data;
}

void from_json(const nlohmann::json& json_data, JSONInfo_SortBin::InputAssemblyState& info)
{
    info.topology = string_to_enum_VkPrimitiveTopology(json_data.at("topology").get<std::string>());
}

void from_json(const nlohmann::json& json_data, JSONInfo_SortBin::RasterizationState& info)
{
    info.polygon_mode = string_to_enum_VkPolygonMode(json_data.at("polygon-mode").get<std::string>());
    info.cull_mode = string_to_enum_VkCullModeFlagBits(json_data.at("cull-mode").get<std::string>());
    info.front_face = string_to_enum_VkFrontFace(json_data.at("front-face").get<std::string>());
}

void from_json(const nlohmann::json& json_data, JSONInfo_SortBin::DepthStencilState& info)
{
    info.depth_test_enable = json_data.at("depth-test-enable").get<bool>();
    info.depth_write_enable = json_data.at("depth-write-enable").get<bool>();
    info.depth_compare_op = string_to_enum_VkCompareOp(json_data.at("depth-compare-op").get<std::string>());
    info.stencil_test_enable = json_data.at("stencil-test-enable").get<bool>();
}

void from_json(const nlohmann::json& json_data, JSONInfo_SortBin::PipelineState& info)
{
    info.shader_state = json_data.at("shader-state");
    info.input_assembly_state = json_data.at("input-assembly-state");
    info.rasterization_state = json_data.at("rasterization-state");
    info.depth_stencil_state = json_data.at("depth-stencil-state");
}

void from_json(const nlohmann::json& json_data, JSONInfo_SortBin::State& info)
{
    info.sortbin_name = json_data.at("name").get<std::string>();
    info.pipeline_state = json_data.at("pipeline-state");
}

void from_json(const nlohmann::json& json_data, JSONInfo_SortBin& info)
{
    const std::vector<JSONInfo_SortBin::State>& state_list = json_data;

    info.state_umap = {};
    for (const JSONInfo_SortBin::State& state : state_list)
    {
        info.state_umap.insert({state.sortbin_name, state});
    }
}

struct JSONInfo_SortBinReflection
{
    struct VertexInputState
    {
        std::vector<VkVertexInputBindingDescription> binding_description_list;
        std::vector<VkVertexInputAttributeDescription> attribute_description_list;
    };

    struct DescriptorBindingState
    {
        std::string name;
        uint32_t binding_id;
        VkDescriptorType descriptor_type;
        uint32_t descriptor_count; 
        VkShaderStageFlags shader_stage_flags;
        // std::unordered_set<DescriptorVariable> variables;
    };

    struct DescriptorSetState
    {
        uint32_t set_id;
        std::vector<DescriptorBindingState> binding_list;
    };

    struct PushConstantState
    {
        VkShaderStageFlags shader_stage_flags;
        uint32_t offset;
        uint32_t size;
        // std::unordered_set<DescriptorVariable> variables;
    };

    struct State
    {
        std::string sortbin_name;
        VertexInputState vertex_input_state;
        std::vector<DescriptorSetState> desc_set_state_list;
        std::vector<PushConstantState> push_const_state_list;

        bool operator==(const State& other) const
        {
            return sortbin_name == other.sortbin_name;
        };

        struct Hash
        {
            std::size_t operator()(const State& obj) const
            {
                return std::hash<std::string>()(obj.sortbin_name);
            }
        };
    };

    std::unordered_map<std::string, State> state_umap;
};

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

void from_json(const nlohmann::json& json_data, JSONInfo_SortBinReflection::VertexInputState& info)
{
    info.binding_description_list = json_data.at("vertex-input-binding-desc");
    info.attribute_description_list = json_data.at("vertex-input-attrib-desc");
}

void from_json(const nlohmann::json& json_data, JSONInfo_SortBinReflection::DescriptorBindingState& info)
{
    info.binding_id = json_data.at("binding-id").get<uint32_t>();
    info.descriptor_type = string_to_enum_VkDescriptorType(json_data.at("descriptor-type").get<std::string>());
    info.descriptor_count = json_data.at("descriptor-count").get<uint32_t>();

    info.shader_stage_flags = 0x0;
    for (const std::string& shader_stage_str : json_data.at("stage-flags"))
    {
        info.shader_stage_flags |= string_to_enum_VkShaderStageFlags(shader_stage_str);
    }
}

void from_json(const nlohmann::json& json_data, JSONInfo_SortBinReflection::DescriptorSetState& info)
{
    info.set_id = json_data.at("set-id");
    info.binding_list = json_data.at("binding-list");
}

void from_json(const nlohmann::json& json_data, JSONInfo_SortBinReflection::State& info)
{
    info.sortbin_name = json_data.at("name").get<std::string>();
    info.vertex_input_state = json_data.at("vertex-input-state");
    info.desc_set_state_list = json_data.at("desc-set-state");
    // info.push_const_state_list = 
}

void from_json(const nlohmann::json& json_data, JSONInfo_SortBinReflection& info)
{
    const std::vector<JSONInfo_SortBinReflection::State>& state_list = json_data;

    info.state_umap = {};
    for (const JSONInfo_SortBinReflection::State& state : state_list)
    {
        info.state_umap.insert({state.sortbin_name, state});
    }
}

struct PipelineInfo
{
    std::vector<VkVertexInputBindingDescription> vertex_input_binding_desc_list;
    std::vector<VkVertexInputAttributeDescription> vertex_input_attrib_desc_list;
    std::vector<VkViewport> viewport_list;
    std::vector<VkRect2D> scissor_list;
    std::vector<VkPipelineColorBlendAttachmentState> color_blend_attachment_state_list;

    std::vector<VkPipelineShaderStageCreateInfo> shader_stage_create_info_list;
    VkPipelineVertexInputStateCreateInfo vertex_input_create_info;
    VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info;
    VkPipelineViewportStateCreateInfo viewport_state_create_info;
    VkPipelineRasterizationStateCreateInfo rasterization_state_create_info;
    VkPipelineMultisampleStateCreateInfo multisample_state_create_info;
    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info;
    VkPipelineColorBlendStateCreateInfo color_blend_state_create_info;

    std::vector<VkDescriptorSetLayout> vk_handle_desc_set_layout_list;
    VkPipelineLayout vk_handle_pipeline_layout;
};


void get_default_pipeline_states(const uint32_t window_width, const uint32_t window_height, PipelineInfo& pipeline_info)
{
    // Viewport / Scissor

    const std::vector<VkViewport> defualt_viewports {{
        .x = 0,
        .y = 0,
        .width = static_cast<float>(window_width),
        .height = static_cast<float>(window_height),
        .minDepth = 0,
        .maxDepth = 1,
    }};

    pipeline_info.viewport_list = defualt_viewports;

    const std::vector<VkRect2D> default_scissors {{
        .offset = {.x = 0, .y = 0},
        .extent = {window_width, window_height}
    }};

    pipeline_info.scissor_list = default_scissors;

    const VkPipelineViewportStateCreateInfo default_viewport_state {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0x0,
        .viewportCount = static_cast<uint32_t>(pipeline_info.viewport_list.size()),
        .pViewports = pipeline_info.viewport_list.data(),
        .scissorCount = static_cast<uint32_t>(pipeline_info.scissor_list.size()),
        .pScissors = pipeline_info.scissor_list.data(),
    };

    pipeline_info.viewport_state_create_info = default_viewport_state;

    // Multisample

    const VkPipelineMultisampleStateCreateInfo default_multisample_state {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 0,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };

    pipeline_info.multisample_state_create_info = default_multisample_state;
}

void process_pipeline_state(const JSONInfo_SortBin::State& sortbin_state, PipelineInfo& pipeline_info, const std::string& shader_root_path)
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

    const auto create_shader_module = [](const std::string& shader_root_path, const std::string& shader_name) {
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

    // Shader Stage

    for (const std::string shader_name : sortbin_state.pipeline_state.shader_state.shader_names)
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

        pipeline_info.shader_stage_create_info_list.push_back(create_info);
    }

    // Input Assembly

    const VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info 
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0x0,
        .topology = sortbin_state.pipeline_state.input_assembly_state.topology,
        .primitiveRestartEnable = VK_FALSE,
    };

    pipeline_info.input_assembly_create_info = input_assembly_create_info;

    // Rasterization

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

    pipeline_info.rasterization_state_create_info = rasterization_create_info;

    // Depth Stehcil

    const VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = sortbin_state.pipeline_state.depth_stencil_state.depth_test_enable,
        .depthWriteEnable = sortbin_state.pipeline_state.depth_stencil_state.depth_write_enable,
        .depthCompareOp = sortbin_state.pipeline_state.depth_stencil_state.depth_compare_op,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = sortbin_state.pipeline_state.depth_stencil_state.stencil_test_enable
    };

    pipeline_info.depth_stencil_state_create_info = depth_stencil_create_info;
}

void process_reflection_state(const JSONInfo_SortBinReflection::State& sortbin_reflection_state, PipelineInfo& pipeline_info, const VkDescriptorSetLayout vk_handle_global_desc_set_layout)
{
    pipeline_info.vk_handle_desc_set_layout_list.clear();

    // Vertex Input

    pipeline_info.vertex_input_binding_desc_list = sortbin_reflection_state.vertex_input_state.binding_description_list;
    pipeline_info.vertex_input_attrib_desc_list = sortbin_reflection_state.vertex_input_state.attribute_description_list;

    const VkPipelineVertexInputStateCreateInfo vertex_input_create_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0x0,
        .vertexBindingDescriptionCount = static_cast<uint32_t>(pipeline_info.vertex_input_binding_desc_list.size()),
        .pVertexBindingDescriptions = pipeline_info.vertex_input_binding_desc_list.data(),
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(pipeline_info.vertex_input_attrib_desc_list.size()),
        .pVertexAttributeDescriptions = pipeline_info.vertex_input_attrib_desc_list.data(),
    };

    pipeline_info.vertex_input_create_info = vertex_input_create_info;

    // Descriptors

    for (const JSONInfo_SortBinReflection::DescriptorSetState& desc_set_reflection_state : sortbin_reflection_state.desc_set_state_list)
    {
        std::vector<VkDescriptorSetLayoutBinding> desc_set_layout_bindings;

        for (const JSONInfo_SortBinReflection::DescriptorBindingState& desc_set_binding_reflection_state : desc_set_reflection_state.binding_list)
        {
            const VkDescriptorSetLayoutBinding desc_set_layout_binding {
                .binding = desc_set_binding_reflection_state.binding_id,
                .descriptorType = desc_set_binding_reflection_state.descriptor_type,
                .descriptorCount = desc_set_binding_reflection_state.descriptor_count,
                .stageFlags = desc_set_binding_reflection_state.shader_stage_flags,
                .pImmutableSamplers = nullptr,
            };


            desc_set_layout_bindings.push_back(desc_set_layout_binding);
        }

        const VkDescriptorSetLayoutCreateInfo desc_set_layout_create_info {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0x0,
            .bindingCount = static_cast<uint32_t>(desc_set_layout_bindings.size()),
            .pBindings = desc_set_layout_bindings.data(),
        };

        pipeline_info.vk_handle_desc_set_layout_list.push_back(vk_core::create_desc_set_layout(desc_set_layout_create_info));
    }

    // Push Consts

    std::vector<VkPushConstantRange> push_const_ranges;

    for (const JSONInfo_SortBinReflection::PushConstantState& push_const_state : sortbin_reflection_state.push_const_state_list)
    {
        const VkPushConstantRange push_const_range {
            .stageFlags = push_const_state.shader_stage_flags,
            .offset = push_const_state.offset,
            .size = push_const_state.size,
        };

        push_const_ranges.push_back(push_const_range);
    }

    const VkPipelineLayoutCreateInfo pipeline_layout_create_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0x0,
        .setLayoutCount = (vk_handle_global_desc_set_layout == VK_NULL_HANDLE) ? static_cast<uint32_t>(pipeline_info.vk_handle_desc_set_layout_list.size()) : 1,
        .pSetLayouts = (vk_handle_global_desc_set_layout == VK_NULL_HANDLE) ? pipeline_info.vk_handle_desc_set_layout_list.data() : &vk_handle_global_desc_set_layout,
        .pushConstantRangeCount = static_cast<uint32_t>(push_const_ranges.size()),
        .pPushConstantRanges = push_const_ranges.data(),
    };

    pipeline_info.vk_handle_pipeline_layout = vk_core::create_pipeline_layout(pipeline_layout_create_info);
}

#endif // RENDERER_LOADER_SORTBIN_HPP