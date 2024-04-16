#ifndef RENDERER_DRAW_INFO_HPP
#define RENDERER_DRAW_INFO_HPP

#include <inttypes.h>

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

#endif // RENDERER_DRAW_INFO_HPP