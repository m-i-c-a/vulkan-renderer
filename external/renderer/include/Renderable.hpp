#ifndef RENDERER_RENDERABLE_HPP
#define RENDERER_RENDERABLE_HPP

#include <inttypes.h>
#include <vector>

namespace renderer
{

struct Mesh
{
    uint32_t sortbin_id;
    uint32_t index_count;
    uint32_t vertex_count;
    uint32_t first_vertex;
    uint32_t first_index;
    int32_t  vertex_offset;
    uint32_t index_stride;
};

// Internal to the renderer
// Cull just fills up RenderableBatch.draw_ids

struct RenderableBatch
{
    const uint32_t mesh_id;
    std::vector<uint32_t> draw_ids;
};

struct DrawInfo
{   
    uint32_t index_count = 0; // doubles (first_vertex, first_index) for non-indexed and indexed draws
    uint32_t vertex_count = 0; // doubles (first_vertex, first_index) for non-indexed and indexed draws
    uint32_t instance_count = 0;
    uint32_t first_index = 0; // doubles (first_vertex, first_index) for non-indexed and indexed draws
    uint32_t first_vertex = 0; // doubles (first_vertex, first_index) for non-indexed and indexed draws
    int32_t vertex_offset = 0;
    uint32_t first_instance = 0; // doubles as draw id
};


}; // renderer

#endif // RENDERER_RENDERABLE_HPP
