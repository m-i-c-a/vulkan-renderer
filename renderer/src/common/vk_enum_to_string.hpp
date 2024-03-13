#ifndef RENDERER_VK_ENUM_TO_STRING_HPP
#define RENDERER_VK_ENUM_TO_STRING_HPP

#include <vulkan/vulkan.h>

#include <unordered_map>

VkImageType string_to_enum_VkImageType(const std::string& str)
{
    const std::unordered_map<std::string, VkImageType> mapping {
        { "VK_IMAGE_TYPE_1D"             , VK_IMAGE_TYPE_1D },
        { "VK_IMAGE_TYPE_2D"             , VK_IMAGE_TYPE_2D },
        { "VK_IMAGE_TYPE_3D"             , VK_IMAGE_TYPE_3D },
    };

    return mapping.at(str);
}

VkImageTiling string_to_enum_VkImageTiling(const std::string& str)
{
    const std::unordered_map<std::string, VkImageTiling> mapping {
        { "VK_IMAGE_TILING_OPTIMAL", VK_IMAGE_TILING_OPTIMAL },
        { "VK_IMAGE_TILING_LINEAR" , VK_IMAGE_TILING_LINEAR  },
    };

    return mapping.at(str);
}

VkSharingMode string_to_enum_VkSharingMode(const std::string& str)
{
    const std::unordered_map<std::string, VkSharingMode> mapping {
        { "VK_SHARING_MODE_EXCLUSIVE"  , VK_SHARING_MODE_EXCLUSIVE  },
        { "VK_SHARING_MODE_CONCURRENT" , VK_SHARING_MODE_CONCURRENT },
    };

    return mapping.at(str);
}

VkImageLayout string_to_enum_VkImageLayout(const std::string& str)
{
    const std::unordered_map<std::string, VkImageLayout> mapping {
        { "VK_IMAGE_LAYOUT_UNDEFINED"                                  , VK_IMAGE_LAYOUT_UNDEFINED                                  },
        { "VK_IMAGE_LAYOUT_GENERAL"                                    , VK_IMAGE_LAYOUT_GENERAL                                    },
        { "VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL"                   , VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL                   },
        { "VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL"           , VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL           },
        { "VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL"            , VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL            },
        { "VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL"                   , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL                   },
        { "VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL"                       , VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL                       },
        { "VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL"                       , VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL                       },
        { "VK_IMAGE_LAYOUT_PREINITIALIZED"                             , VK_IMAGE_LAYOUT_PREINITIALIZED                             },
        { "VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL" , VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL },
        { "VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL" , VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL },
        { "VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL"                   , VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL                   },
        { "VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL"                    , VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL                    },
        { "VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL"                 , VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL                 },
        { "VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL"                  , VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL                  },
        { "VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL"                          , VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL                          },
        { "VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL"                         , VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL                         },
        { "VK_IMAGE_LAYOUT_PRESENT_SRC_KHR"                            , VK_IMAGE_LAYOUT_PRESENT_SRC_KHR                            },
    };

    return mapping.at(str);
}

VkImageUsageFlags string_to_enum_VkImageUsageFlags(const std::string& str)
{
    const std::unordered_map<std::string, VkImageUsageFlags> mapping {
        { "VK_IMAGE_USAGE_TRANSFER_SRC_BIT"             , VK_IMAGE_USAGE_TRANSFER_SRC_BIT             }, 
        { "VK_IMAGE_USAGE_TRANSFER_DST_BIT"             , VK_IMAGE_USAGE_TRANSFER_DST_BIT             },
        { "VK_IMAGE_USAGE_SAMPLED_BIT"                  , VK_IMAGE_USAGE_SAMPLED_BIT                  },
        { "VK_IMAGE_USAGE_STORAGE_BIT"                  , VK_IMAGE_USAGE_STORAGE_BIT                  },
        { "VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT"         , VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT         },
        { "VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT" , VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT },
        { "VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT"     , VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT     },
        { "VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT"         , VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT         },
    };

    return mapping.at(str);
}

VkAttachmentLoadOp string_to_enum_VkAttachmentLoadOp(const std::string& str)
{
    const std::unordered_map<std::string, VkAttachmentLoadOp> mapping {
        { "VK_ATTACHMENT_LOAD_OP_LOAD"      , VK_ATTACHMENT_LOAD_OP_LOAD      },    
        { "VK_ATTACHMENT_LOAD_OP_CLEAR"     , VK_ATTACHMENT_LOAD_OP_CLEAR     },
        { "VK_ATTACHMENT_LOAD_OP_DONT_CARE" , VK_ATTACHMENT_LOAD_OP_DONT_CARE },
    };

    return mapping.at(str);
}

VkAttachmentStoreOp string_to_enum_VkAttachmentStoreOp(const std::string& str)
{
    const std::unordered_map<std::string, VkAttachmentStoreOp> mapping {
        { "VK_ATTACHMENT_STORE_OP_STORE"     , VK_ATTACHMENT_STORE_OP_STORE     },    
        { "VK_ATTACHMENT_STORE_OP_DONT_CARE" , VK_ATTACHMENT_STORE_OP_DONT_CARE },
    };

    return mapping.at(str);
}

VkFormat string_to_enum_VkFormat(const std::string& str)
{
    const std::unordered_map<std::string, VkFormat> mapping {
        { "VK_FORMAT_R8G8B8A8_UNORM"  , VK_FORMAT_R8G8B8A8_UNORM },
        { "VK_FORMAT_D32_SFLOAT"      , VK_FORMAT_D32_SFLOAT     },
        { "VK_FORMAT_R32G32B32_SFLOAT", VK_FORMAT_R32G32B32_SFLOAT },
        { "VK_FORMAT_R32G32_SFLOAT"   , VK_FORMAT_R32G32_SFLOAT },
    };

    return mapping.at(str);
}

VkSampleCountFlagBits uint32_to_enum_VkSampleCountFlagBits(const uint32_t val)
{
    const std::unordered_map<uint32_t, VkSampleCountFlagBits> mapping {
        { 1  , VK_SAMPLE_COUNT_1_BIT  },
        { 2  , VK_SAMPLE_COUNT_2_BIT  },
        { 4  , VK_SAMPLE_COUNT_4_BIT  },
        { 8  , VK_SAMPLE_COUNT_8_BIT  },
        { 16 , VK_SAMPLE_COUNT_16_BIT },
        { 32 , VK_SAMPLE_COUNT_32_BIT },
        { 64 , VK_SAMPLE_COUNT_64_BIT },
    };

    return mapping.at(val);
}

VkCompareOp string_to_enum_VkCompareOp(const std::string& str)
{
    const std::unordered_map<std::string, VkCompareOp> mapping {
        { "VK_COMPARE_OP_ALWAYS"          , VK_COMPARE_OP_ALWAYS },
        { "VK_COMPARE_OP_EQUAL"           , VK_COMPARE_OP_EQUAL },
        { "VK_COMPARE_OP_GREATER"         , VK_COMPARE_OP_GREATER },
        { "VK_COMPARE_OP_GREATER_OR_EQUAL", VK_COMPARE_OP_GREATER_OR_EQUAL },
        { "VK_COMPARE_OP_LESS"            , VK_COMPARE_OP_LESS },
        { "VK_COMPARE_OP_LESS_OR_EQUAL"   , VK_COMPARE_OP_LESS_OR_EQUAL },
        { "VK_COMPARE_OP_NEVER"           , VK_COMPARE_OP_NEVER },
        { "VK_COMPARE_OP_NOT_EQUAL"       , VK_COMPARE_OP_NOT_EQUAL },
    };

    return mapping.at(str);
}

VkPolygonMode string_to_enum_VkPolygonMode(const std::string& str)
{
    const std::unordered_map<std::string, VkPolygonMode> mapping {
        { "VK_POLYGON_MODE_FILL"             , VK_POLYGON_MODE_FILL },
        { "VK_POLYGON_MODE_FILL_RECTANGLE_NV", VK_POLYGON_MODE_FILL_RECTANGLE_NV },
        { "VK_POLYGON_MODE_LINE"             , VK_POLYGON_MODE_LINE },
        { "VK_POLYGON_MODE_POINT"            , VK_POLYGON_MODE_POINT },
    };

    return mapping.at(str);
}

VkCullModeFlags string_to_enum_VkCullModeFlagBits(const std::string& str)
{
    const std::unordered_map<std::string, VkCullModeFlags> mapping {

        { "VK_CULL_MODE_BACK_BIT"      , VK_CULL_MODE_BACK_BIT },
        { "VK_CULL_MODE_FRONT_AND_BACK", VK_CULL_MODE_FRONT_AND_BACK },
        { "VK_CULL_MODE_FRONT_BIT"     , VK_CULL_MODE_FRONT_BIT },
        { "VK_CULL_MODE_NONE"          , VK_CULL_MODE_NONE },
    };

    return mapping.at(str);
}

VkFrontFace string_to_enum_VkFrontFace(const std::string& str)
{
    const std::unordered_map<std::string, VkFrontFace> mapping {
        { "VK_FRONT_FACE_CLOCKWISE"        , VK_FRONT_FACE_CLOCKWISE },
        { "VK_FRONT_FACE_COUNTER_CLOCKWISE", VK_FRONT_FACE_COUNTER_CLOCKWISE },
    };

    return mapping.at(str);
}

VkPrimitiveTopology string_to_enum_VkPrimitiveTopology(const std::string& str)
{
    const std::unordered_map<std::string, VkPrimitiveTopology> mapping {
        { "VK_PRIMITIVE_TOPOLOGY_POINT_LIST"                   , VK_PRIMITIVE_TOPOLOGY_POINT_LIST },
        { "VK_PRIMITIVE_TOPOLOGY_LINE_LIST"                    , VK_PRIMITIVE_TOPOLOGY_LINE_LIST },
        { "VK_PRIMITIVE_TOPOLOGY_LINE_STRIP"                   , VK_PRIMITIVE_TOPOLOGY_LINE_STRIP },
        { "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST"                , VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST },
        { "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP"               , VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP  },
        { "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN"                 , VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN },
        { "VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY"     , VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY  },
        { "VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY"    , VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY },
        { "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY" , VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY },
        { "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY", VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY },
        { "VK_PRIMITIVE_TOPOLOGY_PATCH_LIST"                   , VK_PRIMITIVE_TOPOLOGY_PATCH_LIST  },
        { "VK_PRIMITIVE_TOPOLOGY_MAX_ENUM"                     , VK_PRIMITIVE_TOPOLOGY_MAX_ENUM },
    };

    return mapping.at(str);
}

VkVertexInputRate string_to_enum_VkVertexInputRate(const std::string& str)
{
    const std::unordered_map<std::string, VkVertexInputRate> mapping {
        { "VK_VERTEX_INPUT_RATE_VERTEX"  , VK_VERTEX_INPUT_RATE_VERTEX },
        { "VK_VERTEX_INPUT_RATE_INSTANCE", VK_VERTEX_INPUT_RATE_INSTANCE },
    };

    return mapping.at(str);
}

VkDescriptorType string_to_enum_VkDescriptorType(const std::string& str)
{
    const std::unordered_map<std::string, VkDescriptorType> mapping {
        { "VK_DESCRIPTOR_TYPE_SAMPLER"                , VK_DESCRIPTOR_TYPE_SAMPLER                }, 
        { "VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER" , VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER }, 
        { "VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE"          , VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE          }, 
        { "VK_DESCRIPTOR_TYPE_STORAGE_IMAGE"          , VK_DESCRIPTOR_TYPE_STORAGE_IMAGE          }, 
        { "VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER"   , VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER   }, 
        { "VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER"   , VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER   }, 
        { "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER"         , VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER         }, 
        { "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER"         , VK_DESCRIPTOR_TYPE_STORAGE_BUFFER         }, 
        { "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC" , VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC }, 
        { "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC" , VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC }, 
        { "VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT"       , VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT       }, 
    };

    return mapping.at(str);
}

VkShaderStageFlags string_to_enum_VkShaderStageFlags(const std::string& str)
{
    const std::unordered_map<std::string, VkShaderStageFlags> mapping {
        { "VK_SHADER_STAGE_VERTEX_BIT"                  , VK_SHADER_STAGE_VERTEX_BIT                  }, 
        { "VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT"    , VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT    }, 
        { "VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT" , VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT }, 
        { "VK_SHADER_STAGE_GEOMETRY_BIT"                , VK_SHADER_STAGE_GEOMETRY_BIT                }, 
        { "VK_SHADER_STAGE_FRAGMENT_BIT"                , VK_SHADER_STAGE_FRAGMENT_BIT                }, 
        { "VK_SHADER_STAGE_COMPUTE_BIT"                 , VK_SHADER_STAGE_COMPUTE_BIT                 }, 
        { "VK_SHADER_STAGE_ALL_GRAPHICS"                , VK_SHADER_STAGE_ALL_GRAPHICS                }, 
        { "VK_SHADER_STAGE_ALL"                         , VK_SHADER_STAGE_ALL                         }, 
        { "VK_SHADER_STAGE_RAYGEN_BIT_KHR"              , VK_SHADER_STAGE_RAYGEN_BIT_KHR              }, 
        { "VK_SHADER_STAGE_ANY_HIT_BIT_KHR"             , VK_SHADER_STAGE_ANY_HIT_BIT_KHR             }, 
        { "VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR"         , VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR         }, 
        { "VK_SHADER_STAGE_MISS_BIT_KHR"                , VK_SHADER_STAGE_MISS_BIT_KHR                }, 
        { "VK_SHADER_STAGE_INTERSECTION_BIT_KHR"        , VK_SHADER_STAGE_INTERSECTION_BIT_KHR        }, 
        { "VK_SHADER_STAGE_CALLABLE_BIT_KHR"            , VK_SHADER_STAGE_CALLABLE_BIT_KHR            }, 
        { "VK_SHADER_STAGE_TASK_BIT_EXT"                , VK_SHADER_STAGE_TASK_BIT_EXT                }, 
        { "VK_SHADER_STAGE_MESH_BIT_EXT"                , VK_SHADER_STAGE_MESH_BIT_EXT                }, 
    };

    return mapping.at(str);
}

#endif // RENDERER_VK_ENUM_TO_STRING_HPP