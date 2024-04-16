#ifndef RENDERER_DESCRIPTOR_VARIABLE_HPP
#define RENDERER_DESCRIPTOR_VARIABLE_HPP

#include <string>
#include <inttypes.h>
// #include <unordered_map>

struct DescriptorVariable 
{
    std::string name;
    uint32_t offset;
    uint32_t size;
    uint32_t count;
    // std::unordered_map<std::string, DescriptorVariable> internal_structure;

    bool operator==(const DescriptorVariable& other) const
    {
        return name == other.name && offset == other.offset && size == other.size;
    };

    struct Hash
    {
        std::size_t operator()(const DescriptorVariable& obj) const
        {
            return std::hash<std::string>()(obj.name);
        }
    };
};

#endif // RENDERER_DESCRIPTOR_VARIABLE_HPP