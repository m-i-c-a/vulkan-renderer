#ifndef RENDERER_RENDERABLE_HPP
#define RENDERER_RENDERABLE_HPP

#include <inttypes.h>

struct Renderable
{
    uint32_t mesh_id;
    uint32_t material_id;
    uint32_t draw_id;
    uint16_t default_sortbin_id;
    uint16_t supported_sortbin_set_id;
};

#endif // RENDERER_RENDERABLE_HPP