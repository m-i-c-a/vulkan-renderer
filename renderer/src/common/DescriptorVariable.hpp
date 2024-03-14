#ifndef RENDERER_DESCRIPTOR_VARIABLE_HPP
#define RENDERER_DESCRIPTOR_VARIABLE_HPP

#include <string>
#include <inttypes.h>

struct DescriptorVariable 
{
    std::string name;
    uint32_t offset;
    uint32_t size;

    bool operator==(const DescriptorVariable& other) const
    {
        return name == other.name;
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