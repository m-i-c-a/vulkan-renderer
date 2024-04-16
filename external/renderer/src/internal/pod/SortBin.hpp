#ifndef RENDERER_SORT_BIN_HPP
#define RENDERER_SORT_BIN_HPP

#include "DrawInfo.hpp"
#include "DescriptorVariable.hpp"

#include <vulkan/vulkan.h>

#include <string>
#include <vector>
#include <unordered_map>

struct SortBin
{
    // JSON-Derived
    const std::string name;
    const std::unordered_map<std::string, DescriptorVariable> descriptor_variable_material_umap;
    const std::unordered_map<std::string, DescriptorVariable> descriptor_variable_draw_umap;
    const uint64_t material_data_block_size;
    const uint64_t material_data_block_end_padding_size;
    const uint64_t draw_data_block_size;
    const uint64_t draw_data_block_end_padding_size;

    // Vulkan Handles
    const VkPipeline vk_handle_pipeline;
    const VkPipelineLayout vk_handle_pipeline_layout;
    const VkDescriptorSet vk_handle_desc_set;

    const uint8_t supported_sortbin_set;

    // Runtime
    std::vector<DrawInfo> draw_list_u32;
    std::vector<DrawInfo> draw_list_u16;
    std::vector<DrawInfo> draw_list_u8;
    std::vector<DrawInfo> draw_list;
};

#endif // RENDERER_SORT_BIN_HPP