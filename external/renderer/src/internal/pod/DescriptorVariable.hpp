#ifndef RENDERER_DESCRIPTOR_VARIABLE_HPP
#define RENDERER_DESCRIPTOR_VARIABLE_HPP

#include <string>
#include <inttypes.h>
#include <unordered_map>
#include <memory>

struct InternalDesctriptorVariable
{
    std::string name;
    uint32_t offset;
    uint32_t size;
    uint32_t count;

    bool operator==(const InternalDesctriptorVariable& other) const
    {
        return name == other.name && 
               offset == other.offset && 
               size == other.size &&
               count == other.count;
    };
};

struct DescriptorVariable
{
    std::string name;
    uint32_t offset;
    uint32_t size;
    uint32_t count;
    std::unordered_map<std::string, InternalDesctriptorVariable> internal_structure;

    bool operator==(const DescriptorVariable &other) const
    {
        return name == other.name && 
               offset == other.offset && 
               size == other.size &&
               count == other.count &&
               internal_structure == other.internal_structure;
    };

    struct Hash
    {
        std::size_t operator()(const DescriptorVariable &obj) const
        {
            return std::hash<std::string>()(obj.name);
        }
    };
};

#endif // RENDERER_DESCRIPTOR_VARIABLE_HPP