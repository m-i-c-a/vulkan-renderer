#ifndef RENDERER_LOADER_SHARED_HPP
#define RENDERER_LOADER_SHARED_HPP

#include "defines.hpp"

#include "json.hpp"

#include <fstream>

nlohmann::json read_json_file(const char* const filepath)
{
    std::ifstream file(filepath);
    ASSERT(file.is_open(), "Failed to vulkan init config file: %s\n", filepath);

    const nlohmann::json json_data = nlohmann::json::parse(file);

    file.close();

    return json_data;
}

#endif // RENDERER_LOADER_SHARED_HPP