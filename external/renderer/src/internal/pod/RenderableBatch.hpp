#ifndef RENDERER_RENDERABLE_BATCH_HPP
#define RENDERER_RENDERABLE_BATCH_HPP

#include <inttypes.h>
#include <vector>

struct RenderableBatch
{
    const uint32_t mesh_id;
    std::vector<uint32_t> draw_ids;
};

#endif // RENDERER_RENDERABLE_BATCH_HPP