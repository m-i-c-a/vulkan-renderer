#ifndef RENDERER_MESH_HPP
#define RENDERER_MESH_HPP

#include <inttypes.h>

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

#endif // RENDERER_MESH_HPP