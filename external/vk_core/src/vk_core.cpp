#include "vk_core.hpp"

#include <vulkan/vulkan.h>

#include <GLFW/glfw3.h>
#include "json.hpp"

#include <stdio.h>
#include <assert.h>
#include <fstream>
#include <vector>

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


struct ConfigInfoInstance
{
    std::string application_name; 
    uint32_t application_version[3];
    std::string engine_name;
    uint32_t engine_version[3];
    uint32_t api_version[2];
    std::vector<std::string> layers;
    std::vector<std::string> extensions;
};

void from_json(const nlohmann::json& j, ConfigInfoInstance& c)
{
    j.at("application_name").get_to(c.application_name);
    j.at("application_version").get_to(c.application_version);
    j.at("engine_name").get_to(c.engine_name);
    j.at("engine_version").get_to(c.engine_version);
    j.at("api_version").get_to(c.api_version);
    j.at("layers").get_to(c.layers);
    j.at("extensions").get_to(c.extensions);
}

struct ConfigInfoDevice
{
    std::vector<std::string> layers;
    std::vector<std::string> extensions;
};

void from_json(const nlohmann::json& j, ConfigInfoDevice& c)
{
    j.at("layers").get_to(c.layers);
    j.at("extensions").get_to(c.extensions);
}

struct ConfigInfoSwapchain
{
    uint32_t min_image_count = 0;
    std::string present_mode;
};

void from_json(const nlohmann::json& j, ConfigInfoSwapchain& c)
{
    j.at("min_image_count").get_to(c.min_image_count);
    j.at("present_mode").get_to(c.present_mode);
}

static VkInstance create_instance(const nlohmann::json& json_data)
{
    const ConfigInfoInstance config_info = json_data.at("instance").get<ConfigInfoInstance>();

    const uint32_t api_version = [](const uint32_t versions[2]){
        switch (versions[0])
        {
            case 1:
            {
                switch (versions[1])
                {
                    case 0:
                        return VK_API_VERSION_1_0;
                    case 1:
                        return VK_API_VERSION_1_1;
                    case 2:
                        return VK_API_VERSION_1_2;
                    case 3:
                        return VK_API_VERSION_1_3;
                    default:
                        EXIT("Invalid VK_VERSION specified in config file.\n");
                }
                break;
            }
            default:
            {
                EXIT("Invalid VK_VERSION specified in config file.\n");
            }
        }
        return 0u;
    }(config_info.api_version);

    std::vector<const char*> layers (config_info.layers.size(), "");
    for (uint32_t i = 0; i < layers.size(); ++i)
        layers[i] = config_info.layers.at(i).c_str();

    std::vector<const char*> extensions (config_info.extensions.size(), "");
    for (uint32_t i = 0; i < extensions.size(); ++i)
        extensions[i] = config_info.extensions.at(i).c_str();

    const VkApplicationInfo app_info {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = config_info.application_name.c_str(),
        .applicationVersion = VK_MAKE_VERSION(config_info.application_version[0], config_info.application_version[1], config_info.application_version[2]),
        .pEngineName = config_info.engine_name.c_str(),
        .engineVersion = VK_MAKE_VERSION(config_info.engine_version[0], config_info.engine_version[1], config_info.engine_version[2]),
        .apiVersion = api_version
    };

    const VkInstanceCreateInfo create_info {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = static_cast<uint32_t>(layers.size()),
        .ppEnabledLayerNames = layers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
    };

    VkInstance instance = VK_NULL_HANDLE;
    VK_CHECK(vkCreateInstance(&create_info, nullptr, &instance));
    return instance;
}

static VkSurfaceKHR create_surface(VkInstance instance, GLFWwindow* window)
{
    VkSurfaceKHR surface;
    VK_CHECK(glfwCreateWindowSurface(instance, window, nullptr, &surface));
    return surface;
}

static VkPhysicalDevice select_physical_device(VkInstance instance)
{
    uint32_t numPhysicalDevices = 0;
    vkEnumeratePhysicalDevices(instance, &numPhysicalDevices, nullptr);
    VkPhysicalDevice* physicalDevices = new VkPhysicalDevice[numPhysicalDevices];
    vkEnumeratePhysicalDevices(instance, &numPhysicalDevices, physicalDevices);

    VkPhysicalDevice physicalDevice = physicalDevices[0];
    delete [] physicalDevices;
    return physicalDevice;
}

static uint32_t select_queue_family_index(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
    uint32_t numQueueFamilyProperties = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &numQueueFamilyProperties, nullptr);
    VkQueueFamilyProperties* queueFamilyProperties = new VkQueueFamilyProperties[numQueueFamilyProperties];
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &numQueueFamilyProperties, queueFamilyProperties);

    uint32_t graphicsQueueFamilyIndex = UINT32_MAX;
    uint32_t presentQueueFamilyIndex = UINT32_MAX;
    for (uint32_t i = 0; i < numQueueFamilyProperties; ++i)
    {
        VkBool32 q_fam_supports_present = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &q_fam_supports_present);

        if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && q_fam_supports_present == VK_TRUE)
        {
            graphicsQueueFamilyIndex = i;
            presentQueueFamilyIndex  = i;
            break;
        }
    }

    delete [] queueFamilyProperties;

    assert(graphicsQueueFamilyIndex < UINT32_MAX && "no supported graphics queue family index");
    assert(presentQueueFamilyIndex  < UINT32_MAX && "no supported present queue family index");
    assert(graphicsQueueFamilyIndex == presentQueueFamilyIndex && "queue families (graphics/present) do not match");

    return graphicsQueueFamilyIndex;
}

static VkDevice create_device(const nlohmann::json& json_data, const VkPhysicalDevice physical_device, const uint32_t q_fam_idx)
{
    const ConfigInfoDevice config_info = json_data.at("device").get<ConfigInfoDevice>();

    std::vector<const char*> layers (config_info.layers.size(), "");
    for (uint32_t i = 0; i < layers.size(); ++i)
        layers[i] = config_info.layers.at(i).c_str();

    std::vector<const char*> extensions(config_info.extensions.size(), "");
    for (uint32_t i = 0; i < extensions.size(); ++i)
        extensions[i] = config_info.extensions.at(i).c_str();

    const float q_priority = 1.0f;

    const VkDeviceQueueCreateInfo queue_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = q_fam_idx,
        .queueCount = 1,
        .pQueuePriorities = &q_priority
    };

    const VkPhysicalDeviceVulkan13Features vk_physicalDeviceFeatures13 {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext = nullptr,
        .dynamicRendering = VK_TRUE
    };

    const VkDeviceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext =  &vk_physicalDeviceFeatures13,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_create_info,
        .enabledLayerCount = static_cast<uint32_t>(layers.size()),
        .ppEnabledLayerNames = (layers.size() == 0) ? nullptr : layers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
        .pEnabledFeatures = nullptr
    };

    VkDevice device = VK_NULL_HANDLE;
    VK_CHECK(vkCreateDevice(physical_device, &create_info, nullptr, &device));
    return device;
}

static VkQueue get_queue(const VkDevice device, const uint32_t queue_family_index)
{
   VkQueue queue = VK_NULL_HANDLE;
   vkGetDeviceQueue(device, queue_family_index, 0, &queue);
   return queue;
}

static uint32_t getSwapchainMinImageCount(const VkSurfaceCapabilitiesKHR& vk_surfaceCapabilities, const uint32_t requestedImageCount)
{
    assert(requestedImageCount > 0 && "Invalid requested image count for swapchain!");

    uint32_t minImageCount = UINT32_MAX;

    // If the maxImageCount is 0, then there is not a limit on the number of images the swapchain
    // can support (ignoring memory constraints). See the Vulkan Spec for more information.

    if (vk_surfaceCapabilities.maxImageCount == 0)
    {
        if (requestedImageCount >= vk_surfaceCapabilities.minImageCount)
        {
            minImageCount = requestedImageCount;
        }
        else
        {
            EXIT("Failed to create Swapchain. The requested number of images %u does not meet the minimum requirement of %u.\n", requestedImageCount, vk_surfaceCapabilities.minImageCount);
        }
    }
    else if (requestedImageCount >= vk_surfaceCapabilities.minImageCount && requestedImageCount <= vk_surfaceCapabilities.maxImageCount)
    {
        minImageCount = requestedImageCount;
    }
    else
    {
        EXIT("The number of requested Swapchain images %u is not supported. Min: %u Max: %u.\n", requestedImageCount, vk_surfaceCapabilities.minImageCount, vk_surfaceCapabilities.maxImageCount);
    }

    return minImageCount;
}

static void getSwapchainImageFormatAndColorSpace(const VkPhysicalDevice vk_physicalDevice, const VkSurfaceKHR vk_surface, const VkFormat requestedFormat, VkFormat& chosenFormat, VkColorSpaceKHR& chosenColorSpace)
{
    uint32_t numSupportedSurfaceFormats = 0;
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(vk_physicalDevice, vk_surface, &numSupportedSurfaceFormats, nullptr));
    VkSurfaceFormatKHR* supportedSurfaceFormats = new VkSurfaceFormatKHR[numSupportedSurfaceFormats];
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(vk_physicalDevice, vk_surface, &numSupportedSurfaceFormats, supportedSurfaceFormats));

    bool requestedFormatFound = false;
    for (uint32_t i = 0; i < numSupportedSurfaceFormats; ++i)
    {
        if (supportedSurfaceFormats[i].format == requestedFormat)
        {
            chosenFormat = supportedSurfaceFormats[i].format;
            chosenColorSpace = supportedSurfaceFormats[i].colorSpace;
            requestedFormatFound = true;
            break;
        }
    }

    if (!requestedFormatFound)
    {
        chosenFormat = supportedSurfaceFormats[0].format;
        chosenColorSpace = supportedSurfaceFormats[0].colorSpace;
    }

    delete [] supportedSurfaceFormats;
}

static VkExtent2D getSwapchainExtent(const VkSurfaceCapabilitiesKHR& vk_surfaceCapabilities, const VkExtent2D vk_requestedImageExtent)
{
    VkExtent2D vk_extent;
    
    // The Vulkan Spec states that if the current width/height is 0xFFFFFFFF, then the surface size
    // will be deteremined by the extent specified in the VkSwapchainCreateInfoKHR.

    if (vk_surfaceCapabilities.currentExtent.width != (uint32_t)-1)
    {
        vk_extent = vk_requestedImageExtent;
    }
    else
    {
        vk_extent = vk_surfaceCapabilities.currentExtent;
    }

    return vk_extent;
}

static VkSurfaceTransformFlagBitsKHR getSwapchainPreTransform(const VkSurfaceCapabilitiesKHR& vk_surfaceCapabilities)
{
    VkSurfaceTransformFlagBitsKHR vk_preTransform = VK_SURFACE_TRANSFORM_FLAG_BITS_MAX_ENUM_KHR;

    if (vk_surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
    {
        vk_preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }
    else
    {
        vk_preTransform = vk_surfaceCapabilities.currentTransform;
        LOG("WARNING - Swapchain pretransform is not IDENTITIY_BIT_KHR!\n");
    }

    return vk_preTransform;
}

static VkCompositeAlphaFlagBitsKHR getSwapchainCompositeAlpha(const VkSurfaceCapabilitiesKHR& vk_surfaceCapabilities)
{
    VkCompositeAlphaFlagBitsKHR vk_compositeAlpha = VK_COMPOSITE_ALPHA_FLAG_BITS_MAX_ENUM_KHR;

    // Determine the composite alpha format the application needs.
    // Find a supported composite alpha format (not all devices support alpha opaque),
    // but we prefer it.
    // Simply select the first composite alpha format available
    // Used for blending with other windows in the system

    const VkCompositeAlphaFlagBitsKHR vk_compositeAlphaFlags[4] = {
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
    };

    for (size_t i = 0; i < 4; ++i)
    {
        if (vk_surfaceCapabilities.supportedCompositeAlpha & vk_compositeAlphaFlags[i]) 
        {
            vk_compositeAlpha = vk_compositeAlphaFlags[i];
            break;
        };
    }

    return vk_compositeAlpha;
}

static VkPresentModeKHR getSwapchainPresentMode(const VkPhysicalDevice vk_physicalDevice, const VkSurfaceKHR vk_surface, const VkPresentModeKHR vk_requestedPresentMode)
{
    VkPresentModeKHR vk_presentMode = VK_PRESENT_MODE_MAX_ENUM_KHR;
    uint32_t numSupportedPresentModes = 0;
    VkPresentModeKHR* supportedPresentModes = nullptr;

    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(vk_physicalDevice, vk_surface, &numSupportedPresentModes, nullptr));
    supportedPresentModes = new VkPresentModeKHR[numSupportedPresentModes];
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(vk_physicalDevice, vk_surface, &numSupportedPresentModes, supportedPresentModes));

    // Determine the present mode the application needs.
    // Try to use mailbox, it is the lowest latency non-tearing present mode
    // All devices support FIFO (this mode waits for the vertical blank or v-sync)

    vk_presentMode = VK_PRESENT_MODE_FIFO_KHR;

    // for (uint32_t i = 0; i < numSupportedPresentModes; ++i)
    // {
    //     if (supportedPresentModes[i] == vk_requestedPresentMode)
    //     {
    //         vk_presentMode = vk_requestedPresentMode;
    //         break;
    //     }
    // }

    delete [] supportedPresentModes;
    return vk_presentMode;
}

static VkSwapchainCreateInfoKHR populate_swapchain_create_info(const nlohmann::json& json_data, const VkPhysicalDevice vk_physicalDevice, const VkSurfaceKHR vk_surface, const VkDevice vk_device, const VkExtent2D vk_requestedExtent)
{
    const ConfigInfoSwapchain config_info = json_data.at("swapchain").get<ConfigInfoSwapchain>();

    const VkFormat requested_image_format = VK_FORMAT_R8G8B8_SRGB;
    const VkPresentModeKHR requested_present_mode = [](const std::string str){
        if (str == "IMMEDIATE")
        {
            return VK_PRESENT_MODE_IMMEDIATE_KHR;
        }
        if (str == "MAILBOX")
        {
            return VK_PRESENT_MODE_MAILBOX_KHR;
        }
        if (str == "FIFO_RELAXED")
        {
            return VK_PRESENT_MODE_FIFO_RELAXED_KHR;
        }

        if (str != "FIFO")
        {
            EXIT("Invalid present mode specified in config file: %s\n", str.data());
        }

        return VK_PRESENT_MODE_FIFO_RELAXED_KHR;
    }(config_info.present_mode);

    VkSurfaceCapabilitiesKHR vk_surfaceCapabilities;
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_physicalDevice, vk_surface, &vk_surfaceCapabilities));

    VkFormat vk_imageFormat;
    VkColorSpaceKHR vk_imageColorSpace;
    getSwapchainImageFormatAndColorSpace(vk_physicalDevice, vk_surface, requested_image_format, vk_imageFormat, vk_imageColorSpace);

    VkSwapchainCreateInfoKHR vk_swapchainCreateInfo {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = vk_surface,
        .minImageCount = getSwapchainMinImageCount(vk_surfaceCapabilities, config_info.min_image_count),
        .imageFormat = vk_imageFormat,
        .imageColorSpace = vk_imageColorSpace,
        .imageExtent = getSwapchainExtent(vk_surfaceCapabilities, vk_requestedExtent),
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .preTransform = getSwapchainPreTransform(vk_surfaceCapabilities),
        .compositeAlpha = getSwapchainCompositeAlpha(vk_surfaceCapabilities),
        .presentMode = getSwapchainPresentMode(vk_physicalDevice, vk_surface, requested_present_mode),
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE,
    };

    return vk_swapchainCreateInfo;
}

static VkSwapchainKHR create_swapchain(const VkDevice vk_device, const VkSwapchainCreateInfoKHR& vk_swapchainCreateInfo)
{
    VkSwapchainKHR vk_swapchain;
    VK_CHECK(vkCreateSwapchainKHR(vk_device, &vk_swapchainCreateInfo, nullptr, &vk_swapchain));
    return vk_swapchain;
}

static std::vector<VkImage> get_swapchain_images(const VkDevice vk_device, const VkSwapchainKHR vk_swapchain)
{
    uint32_t numSwapchainImages = 0;
    std::vector<VkImage> vk_swapchainImages;

    VK_CHECK(vkGetSwapchainImagesKHR(vk_device, vk_swapchain, &numSwapchainImages, nullptr));
    vk_swapchainImages.resize(numSwapchainImages);
    VK_CHECK(vkGetSwapchainImagesKHR(vk_device, vk_swapchain, &numSwapchainImages, vk_swapchainImages.data()));

    return vk_swapchainImages;
}

static std::vector<VkImageView> create_swapchain_image_views(const VkDevice vk_device, const std::vector<VkImage>& vk_swapchainImages, const VkFormat vk_swapchainImageFormat)
{
    std::vector<VkImageView> vk_swapchainImageViews(vk_swapchainImages.size());

    VkImageViewCreateInfo vk_imageViewCreateInfo {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = vk_swapchainImageFormat,
        .components = {
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY },
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1 },
    };

    for (size_t i = 0; i < vk_swapchainImages.size(); ++i)
    {
        vk_imageViewCreateInfo.image = vk_swapchainImages[i];
        VK_CHECK(vkCreateImageView(vk_device, &vk_imageViewCreateInfo, nullptr, &vk_swapchainImageViews[i]));
    }

    LOG("Vulkan Info - # Swapchain Images: %lu\n", vk_swapchainImages.size());
    return vk_swapchainImageViews;
}


namespace vk_core
{

static VkInstance vk_handle_instance = VK_NULL_HANDLE;
static VkSurfaceKHR vk_handle_surface = VK_NULL_HANDLE;
static VkPhysicalDevice vk_handle_physical_device = VK_NULL_HANDLE;
static VkPhysicalDeviceMemoryProperties vk_phys_dev_mem_props;
static VkDevice vk_handle_device = VK_NULL_HANDLE;
static VkQueue vk_handle_queue = VK_NULL_HANDLE;
static uint32_t queue_family_idx = 0u;
static VkSwapchainKHR vk_handle_swapchain = VK_NULL_HANDLE;
static std::vector<VkImage> vk_handle_swapchain_image_list;
static std::vector<VkImageView> vk_handle_swapchain_image_view_list;
static VkFormat vk_format_swapchain_image = VK_FORMAT_UNDEFINED;
static uint32_t active_swapchain_image_idx = 0u;

uint32_t get_memory_type_idx(const uint32_t memory_type_indices, const VkMemoryPropertyFlags memory_property_flags)
{
   	// Iterate over all memory types available for the device used in this example
	for (uint32_t i = 0; i < vk_phys_dev_mem_props.memoryTypeCount; i++)
	{
		if (memory_type_indices & (1 << i) && (vk_phys_dev_mem_props.memoryTypes[i].propertyFlags & memory_property_flags) == memory_property_flags)
		{
			return i;
		}
	}

    EXIT("Could not find suitable memory type!");
    return 0; 
}

void init(const uint32_t window_width, const uint32_t window_height, GLFWwindow* window, const std::string_view config_file)
{
    std::ifstream file(config_file.data());
    ASSERT(file.is_open(), "Failed to vulkan init config file: %s\n", config_file.data());

    const nlohmann::json json_data = nlohmann::json::parse(file);

    file.close();

    vk_handle_instance = create_instance(json_data);
    vk_handle_surface = create_surface(vk_handle_instance, window);
    vk_handle_physical_device = select_physical_device(vk_handle_instance);
    queue_family_idx = select_queue_family_index(vk_handle_physical_device, vk_handle_surface);
    vk_handle_device = create_device(json_data, vk_handle_physical_device, queue_family_idx);
    vk_handle_queue = get_queue(vk_handle_device, queue_family_idx);

    const VkSwapchainCreateInfoKHR swapchain_create_info = populate_swapchain_create_info(json_data, vk_handle_physical_device, vk_handle_surface, vk_handle_device, { window_width, window_height });
    vk_handle_swapchain = create_swapchain(vk_handle_device, swapchain_create_info);
    vk_handle_swapchain_image_list = get_swapchain_images(vk_handle_device, vk_handle_swapchain);
    vk_handle_swapchain_image_view_list = create_swapchain_image_views(vk_handle_device, vk_handle_swapchain_image_list, swapchain_create_info.imageFormat);

    VkPhysicalDeviceMemoryProperties physical_device_memory_properties;
    vkGetPhysicalDeviceMemoryProperties(vk_handle_physical_device, &vk_phys_dev_mem_props);

    // Transition swapchain images to present layout

    std::vector<VkImageMemoryBarrier> image_memory_barriers;

    VkImageMemoryBarrier image_memory_barrier {};
    image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_memory_barrier.pNext = nullptr;
    image_memory_barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    image_memory_barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    image_memory_barrier.srcQueueFamilyIndex = vk_core::get_queue_family_idx();
    image_memory_barrier.dstQueueFamilyIndex = vk_core::get_queue_family_idx();
    image_memory_barrier.subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };

    for (const VkImage vk_handle_image : vk_handle_swapchain_image_list)
    {
        image_memory_barrier.image = vk_handle_image;
        image_memory_barriers.push_back(image_memory_barrier);
    }

    const VkCommandPool vk_handle_cmd_pool = vk_core::create_command_pool(0x0);
    const VkCommandBuffer vk_handle_cmd_buff = vk_core::allocate_command_buffer(vk_handle_cmd_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    const VkCommandBufferBeginInfo cmd_buff_begin_info {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = 0x0,
        .pInheritanceInfo = nullptr,
    };

    vkBeginCommandBuffer(vk_handle_cmd_buff, &cmd_buff_begin_info);

    vkCmdPipelineBarrier(vk_handle_cmd_buff,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0x0,
        0, nullptr,
        0, nullptr,
        static_cast<uint32_t>(image_memory_barriers.size()), image_memory_barriers.data());

    vkEndCommandBuffer(vk_handle_cmd_buff);

    const VkPipelineStageFlags none_flag = VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM;

    const VkSubmitInfo submit_info {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = nullptr,
        .pWaitDstStageMask = &none_flag,
        .commandBufferCount = 1,
        .pCommandBuffers = &vk_handle_cmd_buff,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = nullptr,
    };

    queue_submit(1, &submit_info, VK_NULL_HANDLE);

    device_wait_idle();

    destroy_command_pool(vk_handle_cmd_pool);
}; 

void terminate()
{
    for (uint32_t i = 0; i < vk_handle_swapchain_image_list.size(); i++)
    {
        vkDestroyImageView(vk_handle_device, vk_handle_swapchain_image_view_list[i], nullptr);
    }

    vkDestroySwapchainKHR(vk_handle_device, vk_handle_swapchain, nullptr);
    vkDestroyDevice(vk_handle_device, nullptr);
    vkDestroySurfaceKHR(vk_handle_instance, vk_handle_surface, nullptr);
    vkDestroyInstance(vk_handle_instance, nullptr);
}



VkSampler create_sampler(const VkSamplerCreateInfo& create_info)
{
    VkSampler vk_handle_sampler = VK_NULL_HANDLE;
    VK_CHECK(vkCreateSampler(vk_handle_device, &create_info, nullptr, &vk_handle_sampler));
    return vk_handle_sampler;;
}

VkImage create_image(const VkImageCreateInfo& create_info)
{
    VkImage image = VK_NULL_HANDLE;
    VK_CHECK(vkCreateImage(vk_handle_device, &create_info, nullptr, &image));
    return image;
}

VkImageView create_image_view(const VkImageViewCreateInfo& create_info)
{
    VkImageView image_view = VK_NULL_HANDLE;
    VK_CHECK(vkCreateImageView(vk_handle_device, &create_info, nullptr, &image_view));
    return image_view;
}

VkDeviceMemory allocate_image_memory(const VkImage vk_handle_image, const VkMemoryPropertyFlags flags)
{
    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(vk_handle_device, vk_handle_image, &memory_requirements);

    const VkMemoryAllocateInfo memory_alloc_info{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = memory_requirements.size,
        .memoryTypeIndex = get_memory_type_idx(memory_requirements.memoryTypeBits, flags),
    };

    VkDeviceMemory vk_image_memory = VK_NULL_HANDLE;
    vkAllocateMemory(vk_handle_device, &memory_alloc_info, nullptr, &vk_image_memory);
    return vk_image_memory;
}

void bind_image_memory(const VkImage vk_handle_image, const VkDeviceMemory vk_handle_image_memory)
{
    VK_CHECK(vkBindImageMemory(vk_handle_device, vk_handle_image, vk_handle_image_memory, 0));
}

void destroy_image(const VkImage vk_handle_image)
{
    vkDestroyImage(vk_handle_device, vk_handle_image, nullptr);
}

void destroy_image_view(const VkImageView vk_handle_image_view)
{
    vkDestroyImageView(vk_handle_device, vk_handle_image_view, nullptr);
}

VkBuffer create_buffer(const VkBufferCreateInfo& create_info)
{
    VkBuffer vk_handle_buffer = VK_NULL_HANDLE;
    VK_CHECK(vkCreateBuffer(vk_handle_device, &create_info, nullptr, &vk_handle_buffer));
    return vk_handle_buffer;
}

VkDeviceMemory allocate_buffer_memory(const VkBuffer vk_handle_buffer, const VkMemoryPropertyFlags flags, VkDeviceSize& size)
{
    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(vk_handle_device, vk_handle_buffer, &memory_requirements);

    const VkMemoryAllocateInfo memory_alloc_info{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = memory_requirements.size,
        .memoryTypeIndex = get_memory_type_idx(memory_requirements.memoryTypeBits, flags),
    };

    VkDeviceMemory vk_buffer_memory = VK_NULL_HANDLE;
    vkAllocateMemory(vk_handle_device, &memory_alloc_info, nullptr, &vk_buffer_memory);

    size = memory_requirements.size;
    return vk_buffer_memory;
}

void bind_buffer_memory(const VkBuffer vk_handle_buffer, const VkDeviceMemory vk_handle_buffer_memory)
{
    VK_CHECK(vkBindBufferMemory(vk_handle_device, vk_handle_buffer, vk_handle_buffer_memory, 0));

}

void destroy_buffer(const VkBuffer vk_handle_buffer)
{
    vkDestroyBuffer(vk_handle_device, vk_handle_buffer, nullptr);
}

VkShaderModule create_shader_module(const VkShaderModuleCreateInfo& create_info)
{
    VkShaderModule vk_handle_shader_module = VK_NULL_HANDLE;
    VK_CHECK(vkCreateShaderModule(vk_handle_device, &create_info, NULL, &vk_handle_shader_module));
    return vk_handle_shader_module;
}

void destroy_shader_module(const VkShaderModule vk_handle_shader_module)
{
    vkDestroyShaderModule(vk_handle_device, vk_handle_shader_module, nullptr);
}


VkPipeline create_graphics_pipeline(const VkGraphicsPipelineCreateInfo& create_info)
{
    VkPipeline vk_handle_pipeline = VK_NULL_HANDLE;
    VK_CHECK(vkCreateGraphicsPipelines(vk_handle_device, VK_NULL_HANDLE, 1, &create_info, nullptr, &vk_handle_pipeline));
    return vk_handle_pipeline;
}

void destroy_pipeline(const VkPipeline vk_handle_pipeline)
{
    vkDestroyPipeline(vk_handle_device, vk_handle_pipeline, nullptr);
}


VkCommandPool create_command_pool(const VkCommandPoolCreateFlags flags)
{
    const VkCommandPoolCreateInfo create_info {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = flags,
        .queueFamilyIndex = queue_family_idx,
    };

    VkCommandPool cmd_pool = VK_NULL_HANDLE;
    VK_CHECK( vkCreateCommandPool(vk_handle_device, &create_info, nullptr, &cmd_pool) );
    return cmd_pool;
}

void reset_command_pool( const VkCommandPool pool )
{
    VK_CHECK( vkResetCommandPool( vk_handle_device, pool, 0x0 ) );
}

void destroy_command_pool( const VkCommandPool pool )
{
    vkDestroyCommandPool( vk_handle_device, pool, nullptr );
}

VkCommandBuffer allocate_command_buffer( const VkCommandPool cmd_pool, const VkCommandBufferLevel level )
{
    VkCommandBufferAllocateInfo alloc_info {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = cmd_pool,
        .level = level,
        .commandBufferCount = 1,
    };

    VkCommandBuffer cmd_buff = VK_NULL_HANDLE;
    VK_CHECK( vkAllocateCommandBuffers( vk_handle_device, &alloc_info, &cmd_buff ) );
    return cmd_buff;
}

VkDescriptorPool create_desc_pool(const VkDescriptorPoolCreateInfo& create_info)
{
    VkDescriptorPool vk_handle_desc_pool = VK_NULL_HANDLE;
    VK_CHECK(vkCreateDescriptorPool(vk_handle_device, &create_info, nullptr, &vk_handle_desc_pool));
    return vk_handle_desc_pool;
}

void destroy_desc_pool(const VkDescriptorPool vk_handle_desc_pool)
{
    vkDestroyDescriptorPool(vk_handle_device, vk_handle_desc_pool, nullptr);
}

VkDescriptorSetLayout create_desc_set_layout(const VkDescriptorSetLayoutCreateInfo& create_info)
{
    VkDescriptorSetLayout vk_handle_desc_set_layout = VK_NULL_HANDLE;
    VK_CHECK(vkCreateDescriptorSetLayout(vk_handle_device, &create_info, nullptr, &vk_handle_desc_set_layout));
    return vk_handle_desc_set_layout;
}

void destroy_desc_set_layout(const VkDescriptorSetLayout vk_handle_desc_set_layout)
{
    vkDestroyDescriptorSetLayout(vk_handle_device, vk_handle_desc_set_layout, nullptr);
}

std::vector<VkDescriptorSet> allocate_desc_sets(const VkDescriptorSetAllocateInfo& alloc_info)
{
    std::vector<VkDescriptorSet> vk_handle_desc_set_list(alloc_info.descriptorSetCount, VK_NULL_HANDLE);
    VK_CHECK(vkAllocateDescriptorSets(vk_handle_device, &alloc_info, vk_handle_desc_set_list.data()));
    return vk_handle_desc_set_list;
}

void update_desc_sets(const uint32_t update_count, const VkWriteDescriptorSet* const p_write_desc_set_list, const uint32_t copy_count, const VkCopyDescriptorSet* const p_copy_desc_set_list)
{
    vkUpdateDescriptorSets(vk_handle_device, update_count, p_write_desc_set_list, copy_count, p_copy_desc_set_list);
}

VkPipelineLayout create_pipeline_layout(const VkPipelineLayoutCreateInfo& create_info)
{
    VkPipelineLayout vk_handle_pipeline_layout = VK_NULL_HANDLE;
    VK_CHECK(vkCreatePipelineLayout(vk_handle_device, &create_info, nullptr, &vk_handle_pipeline_layout));
    return vk_handle_pipeline_layout;
}

void destroy_pipeline_layout(const VkPipelineLayout vk_handle_pipeline_layout)
{
    vkDestroyPipelineLayout(vk_handle_device, vk_handle_pipeline_layout, nullptr);
}

void map_memory(const VkDeviceMemory vk_handle_memory, const VkDeviceSize offset, const VkDeviceSize size, const VkMemoryMapFlags flags, void** data)
{
    VK_CHECK(vkMapMemory(vk_handle_device, vk_handle_memory, offset, size, flags, data));
}

void unmap_memory(const VkDeviceMemory vk_handle_memory)
{
    vkUnmapMemory(vk_handle_device, vk_handle_memory);
}

void free_memory(const VkDeviceMemory vk_handle_memory)
{
    vkFreeMemory(vk_handle_device, vk_handle_memory, nullptr);
}


void queue_submit(const uint32_t submit_count, const VkSubmitInfo* const p_submit_infos, const VkFence vk_handle_signal_fence)
{
    vkQueueSubmit(vk_handle_queue, submit_count, p_submit_infos, vk_handle_signal_fence);
}

void device_wait_idle()
{
    vkDeviceWaitIdle(vk_handle_device);
}

void present(const uint32_t wait_sem4_count, const VkSemaphore* const vk_handle_wait_sem4_list)
{
    const VkPresentInfoKHR present_info {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = wait_sem4_count,
        .pWaitSemaphores = vk_handle_wait_sem4_list,
        .swapchainCount = 1,
        .pSwapchains = &vk_handle_swapchain,
        .pImageIndices = &active_swapchain_image_idx,
        .pResults = nullptr,
    };

    VK_CHECK(vkQueuePresentKHR(vk_handle_queue, &present_info));
}

void acquire_next_swapchain_image(const VkSemaphore vk_handle_signal_sem4, const VkFence vk_handle_signal_fence)
{
    VK_CHECK(vkAcquireNextImageKHR(vk_handle_device, vk_handle_swapchain, UINT64_MAX, vk_handle_signal_sem4, vk_handle_signal_fence, &active_swapchain_image_idx));
}


VkFence create_fence(const VkFenceCreateFlags flags)
{
    const VkFenceCreateInfo create_info {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = flags,
    };

    VkFence vk_handle_fence = VK_NULL_HANDLE;
    VK_CHECK(vkCreateFence(vk_handle_device, &create_info, nullptr, &vk_handle_fence));
    return vk_handle_fence;
}

void wait_for_fences(const uint32_t fence_count, const VkFence* vk_handle_fence_list, const VkBool32 wait_all, const uint64_t timeout)
{
    VK_CHECK(vkWaitForFences(vk_handle_device, fence_count, vk_handle_fence_list, wait_all, timeout));
}

void reset_fences(const uint32_t fence_count, const VkFence* vk_handle_fence_list)
{
    VK_CHECK(vkResetFences(vk_handle_device, fence_count, vk_handle_fence_list)); 
}

void destroy_fence(const VkFence vk_handle_fence)
{
    vkDestroyFence(vk_handle_device, vk_handle_fence, nullptr);
}

uint32_t get_queue_family_idx()
{
    return queue_family_idx;
}

VkImage get_active_swapchain_image()
{
    return vk_handle_swapchain_image_list[active_swapchain_image_idx];
}

VkImageMemoryBarrier get_active_swapchain_image_memory_barrier(const VkAccessFlags src_access_flags, const VkAccessFlags dst_access_flags, const VkImageLayout old_layout, const VkImageLayout new_layout)
{
    const VkImageMemoryBarrier barrier {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = src_access_flags,
        .dstAccessMask = dst_access_flags,
        .oldLayout = old_layout, 
        .newLayout = new_layout,
        .srcQueueFamilyIndex = queue_family_idx,
        .dstQueueFamilyIndex = queue_family_idx,
        .image = vk_handle_swapchain_image_list[active_swapchain_image_idx],
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        }
    };

    return barrier;
}

}; // vk_core