#ifndef RENDERER_MATERIAL_HPP
#define RENDERER_MATERIAL_HPP

#include <inttypes.h>

struct Material
{
    uint32_t ID;
    uint32_t default_sort_bin_ID;
};

#endif // RENDERER_MATERIAL_HPP