#ifndef RENDERER_DEFINES_HPP
#define RENDERER_DEFINES_HPP

#include <fstream>
#include <stdio.h>
#include <assert.h>

#include <vulkan/vulkan.h>

#define LOG(fmt, ...)                    \
    fprintf(stdout, fmt, ##__VA_ARGS__); \
    fflush(stdout);

#define ASSERT(val, fmt, ...)                    \
    do                                           \
    {                                            \
        if (!(val))                                \
        {                                        \
            fprintf(stdout, fmt, ##__VA_ARGS__); \
            fflush(stdout);                      \
            assert(false);                       \
        }                                        \
    } while(0)                                   \

#define EXIT(fmt, ...)                       \
    do                                       \
    {                                        \
        fprintf(stderr, fmt, ##__VA_ARGS__); \
        fflush(stderr);                      \
        assert(false);                       \
    } while (0)

#define VK_CHECK(val)                  \
    do                                 \
    {                                  \
        if (val != VK_SUCCESS)         \
        {                              \
            assert(val == VK_SUCCESS); \
        }                              \
    } while (false)


#endif // RENDERER_DEFINES_HPP