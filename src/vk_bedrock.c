#include "vk.h"

#define VK_USE_PLATFORM_XLIB_KHR
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

void* alloc(void* ptr, size_t size) {
    void* tmp = (ptr == NULL) ? malloc(size) : realloc(ptr, size);
    if (!tmp) {
        printf("Memory allocation failed\n");
        exit(-1);
    }
    return tmp;
}

const InstanceData all_instances[ALL_INSTANCE_COUNT] = {
    // Instance 0
    {
        .pos = {0.0f, 0.0f},
        .size = {200.0f, 100.0f},
        .rotation = 0.0f,
        .corner_radius = 10.0f,
        .color = 0xFF0000FF, // Red color in RGBA8
        .tex_index = 0,
        .tex_rect = {0.0f, 0.0f, 1.0f, 1.0f}
    },
    // Instance 1
    {
        .pos = {100.0f, 100.0f},
        .size = {100.0f, 100.0f},
        .rotation = 30.0f,
        .corner_radius = 10.0f,
        .color = 0xFF00FF00, // Green color in RGBA8
        .tex_index = 0,
        .tex_rect = {0.0f, 0.0f, 1.0f, 1.0f}
    },
    // Instance 2
    {
        .pos = {200.f, 200.0f},
        .size = {100.0f, 200.0f},
        .rotation = 60.0f,
        .corner_radius = 10.0f,
        .color = 0xFFFF00FF, // Blue color in RGBA8
        .tex_index = 0,
        .tex_rect = {0.0f, 0.0f, 1.0f, 1.0f}
    },
    // Instance 3
    {
        .pos = {300.0f, 300.0f},
        .size = {100.0f, 100.0f},
        .rotation = 90.0f,
        .corner_radius = 10.0f,
        .color = 0xFFFFFF00, // Cyan color in RGBA8
        .tex_index = 0,
        .tex_rect = {0.0f, 0.0f, 1.0f, 1.0f}
    },
    // Instance 4
    {
        .pos = {400.0f, 400.0f},
        .size = {100.0f, 100.0f},
        .rotation = 120.0f,
        .corner_radius = 10.0f,
        .color = 0xFF00FFFF, // Yellow color in RGBA8
        .tex_index = 0,
        .tex_rect = {0.0f, 0.0f, 1.0f, 1.0f}
    },
};
uint32_t indices[INDICES_COUNT] = { 0, 1, 2, 3, 4 };

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    printf("Vulkan Debug: %s\n", pCallbackData->pMessage);
    return VK_FALSE;
}

VK VK_create(unsigned int width, unsigned int height, const char* title) {
    VK vk;
    memset(&vk, 0, sizeof(VK));
    vk.window_p                     = NULL;
    vk.swap_chain.images_p          = NULL;
    vk.swap_chain.image_views_p     = NULL;

    VkResult result;

    // shaderc
    {
        vk.shaderc_compiler = shaderc_compiler_initialize();
        vk.shaderc_options = shaderc_compile_options_initialize();
        shaderc_compile_options_set_optimization_level(vk.shaderc_options, shaderc_optimization_level_performance);
    }
    // createWindow
    {
        if (SDL_Init(SDL_INIT_VIDEO) != 0) {
            printf("ERROR: SDL_Init: %s\n", SDL_GetError());
            exit(-1);
        }
        vk.window_p = SDL_CreateWindow(title,SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,width,height,SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN);
        if (!vk.window_p) {
            printf("ERROR: SDL_CreateWindow: %s\n", SDL_GetError());
            SDL_Quit();
            exit(-1);
        }
    }
    // createVulkanInstance
    {
        uint32_t totalExtensionCount = 1;
        const char** allExtensions = NULL;
        uint32_t validationLayerCount = 0;
        const char* validationLayers[] = { "VK_LAYER_KHRONOS_validation" };

        uint32_t sdlExtensionCount = 0;
        if (!SDL_Vulkan_GetInstanceExtensions((SDL_Window*)vk.window_p, &sdlExtensionCount, NULL)) {
            printf("Failed to get SDL Vulkan instance extensions: %s\n", SDL_GetError());
            exit(-1);
        }

        const char** sdlExtensions = (const char**)alloc(NULL, sizeof(const char*) * sdlExtensionCount);
        if (sdlExtensions == NULL) {
            printf("Failed to allocate memory for SDL Vulkan extensions\n");
            exit(-1);
        }

        if (!SDL_Vulkan_GetInstanceExtensions((SDL_Window*)vk.window_p, &sdlExtensionCount, sdlExtensions)) {
            printf("Failed to get SDL Vulkan instance extensions: %s\n", SDL_GetError());
            free(sdlExtensions);
            exit(-1);
        }

        // Optional: Add VK_EXT_debug_utils_EXTENSION_NAME for debugging
        totalExtensionCount = sdlExtensionCount + 1;
        allExtensions = (const char**)alloc(NULL, sizeof(const char*) * totalExtensionCount);
        if (allExtensions == NULL) {
            printf("Failed to allocate memory for all Vulkan extensions\n");
            free(sdlExtensions);
            exit(-1);
        }
        memcpy(allExtensions, sdlExtensions, sizeof(const char*) * sdlExtensionCount);
        allExtensions[sdlExtensionCount] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

        validationLayerCount = 1;

        VkInstanceCreateInfo instanceCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &(VkApplicationInfo){
                .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                .pApplicationName = "Logos Application",
                .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
                .pEngineName = "Logos Engine",
                .engineVersion = VK_MAKE_VERSION(1, 0, 0),
                .apiVersion = VK_API_VERSION_1_3
            },
            .enabledExtensionCount = totalExtensionCount,
            .ppEnabledExtensionNames = allExtensions,
            .enabledLayerCount = validationLayerCount,
            .ppEnabledLayerNames = validationLayers,
            .pNext = &(VkDebugUtilsMessengerCreateInfoEXT) {
                .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
                .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                   VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                   VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
                .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
                .pfnUserCallback = debugCallback,
                .pUserData = NULL
            }
        };

        result = vkCreateInstance(&instanceCreateInfo, NULL, &vk.instance);
        if (sdlExtensions == NULL) {
            printf("sdl allready freed\n");
        }
        free(sdlExtensions);
        free(allExtensions);
        if (result != VK_SUCCESS) {
            printf("Failed to create Vulkan instance\n");
            exit(-1);
        }
    }
    // setupDebugMessenger
    {
        // Function pointer for vkCreateDebugUtilsMessengerEXT
        PFN_vkCreateDebugUtilsMessengerEXT funcCreateDebugUtilsMessengerEXT = 
            (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vk.instance, "vkCreateDebugUtilsMessengerEXT");
        if (funcCreateDebugUtilsMessengerEXT != NULL) {
            VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {
                .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
                .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                                  VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                                  VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
                .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                              VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                              VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
                .pfnUserCallback = debugCallback,
                .pUserData = NULL
            };

            vk.debug_messenger = VK_NULL_HANDLE;
            if (funcCreateDebugUtilsMessengerEXT(vk.instance, &debugCreateInfo, NULL, &vk.debug_messenger) != VK_SUCCESS) {
                printf("Failed to set up debug messenger\n");
                exit(-1);
            }
        } else {
            printf("vkCreateDebugUtilsMessengerEXT not available\n");
            vk.debug_messenger = VK_NULL_HANDLE;
        }
    }
    // createSurface
    {
        vk.surface = VK_NULL_HANDLE;
        if (!SDL_Vulkan_CreateSurface((SDL_Window*)vk.window_p, vk.instance, &vk.surface)) {
            printf( "Failed to create Vulkan surface: %s\n", SDL_GetError());
            exit(-1);
        }
    }
    // getPhysicalDevice
    {
        uint32_t device_count = 0;
        result = vkEnumeratePhysicalDevices(vk.instance, &device_count, NULL);
        if (result != VK_SUCCESS || device_count == 0) {
            printf( "Failed to find GPUs with Vulkan support\n");
            exit(-1);
        }

        VkPhysicalDevice* devices = alloc(NULL, sizeof(VkPhysicalDevice) * device_count);
        if (devices == NULL) {
            printf( "Failed to allocate memory for physical devices\n");
            exit(-1);
        }
        result = vkEnumeratePhysicalDevices(vk.instance, &device_count, devices);
        if (result != VK_SUCCESS) {
            printf( "Failed to enumerate physical devices\n");
            free(devices);
            exit(-1);
        }

        // Select the first suitable device
        vk.physical_device = devices[0];
        free(devices);

        // check if dynamic rendering is supported
        uint32_t extensionCount = 0;
        vkEnumerateDeviceExtensionProperties(vk.physical_device, NULL, &extensionCount, NULL);
        VkExtensionProperties* extensions = malloc(sizeof(VkExtensionProperties) * extensionCount);
        vkEnumerateDeviceExtensionProperties(vk.physical_device, NULL, &extensionCount, extensions);

        bool dynamicRenderingSupported = false;
        for (uint32_t i = 0; i < extensionCount; i++) {
            if (strcmp(extensions[i].extensionName, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME) == 0) {
                dynamicRenderingSupported = true;
                break;
            }
        }
        free(extensions);

        if (!dynamicRenderingSupported) {
            printf("Dynamic Rendering not supported on this device.\n");
            exit(-1);
        }
    }
    // getQueueFamilyIndices
    {
        vk.queue_family_indices.graphics = UINT32_MAX;
        vk.queue_family_indices.present = UINT32_MAX;
        vk.queue_family_indices.compute = UINT32_MAX;
        vk.queue_family_indices.transfer = UINT32_MAX;

        uint32_t queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(vk.physical_device, &queue_family_count, NULL);
        if (queue_family_count == 0) {
            printf("Failed to find any queue families\n");
            exit(-1);
        }

        VkQueueFamilyProperties* queue_families = alloc(NULL, sizeof(VkQueueFamilyProperties) * queue_family_count);
        if (queue_families == NULL) {
            printf("Failed to allocate memory for queue family properties\n");
            exit(-1);
        }

        vkGetPhysicalDeviceQueueFamilyProperties(vk.physical_device, &queue_family_count, queue_families);

        for (uint32_t i = 0; i < queue_family_count; i++) {
            // Check for graphics support
            if ((queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && vk.queue_family_indices.graphics == UINT32_MAX) {
                vk.queue_family_indices.graphics = i;
            }

            // Check for compute support (prefer dedicated compute queues)
            if ((queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT) && 
                !(queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && 
                vk.queue_family_indices.compute == UINT32_MAX) {
                vk.queue_family_indices.compute = i;
            }

            // Check for transfer support (prefer dedicated transfer queues)
            if ((queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT) && 
                !(queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && 
                vk.queue_family_indices.transfer == UINT32_MAX) {
                vk.queue_family_indices.transfer = i;
            }

            // Check for presentation support
            VkBool32 present_support = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(vk.physical_device, i, vk.surface, &present_support);
            if (present_support && vk.queue_family_indices.present == UINT32_MAX) {
                vk.queue_family_indices.present = i;
            }

            // Break early if all family_indices are found
            if (vk.queue_family_indices.graphics != UINT32_MAX &&
                vk.queue_family_indices.present != UINT32_MAX &&
                vk.queue_family_indices.compute != UINT32_MAX &&
                vk.queue_family_indices.transfer != UINT32_MAX) {
                break;
            }
        }

        free(queue_families);

        // Validate that essential queue family_indices are found
        if (vk.queue_family_indices.graphics == UINT32_MAX || vk.queue_family_indices.present == UINT32_MAX) {
            printf("Failed to find required queue families\n");
            exit(-1);
        }
    }
    // getDevice
    {
        // Define queue priorities
        float queue_priority = 1.0f;

        // To avoid creating multiple VkDeviceQueueCreateInfo for the same family,
        // collect unique queue family vk.queue_family_indices
        uint32_t unique_queue_families[4];
        uint32_t unique_count = 0;

        // Helper macro to add unique queue families
        #define ADD_UNIQUE_FAMILY(family) \
            do { \
                bool exists = false; \
                for (uint32_t i = 0; i < unique_count; i++) { \
                    if (unique_queue_families[i] == family) { \
                        exists = true; \
                        break; \
                    } \
                } \
                if (!exists && unique_count < 4) { \
                    unique_queue_families[unique_count++] = family; \
                } \
            } while(0)

        // Add all required queue families
        ADD_UNIQUE_FAMILY(vk.queue_family_indices.graphics);
        ADD_UNIQUE_FAMILY(vk.queue_family_indices.present);
        ADD_UNIQUE_FAMILY(vk.queue_family_indices.compute);
        ADD_UNIQUE_FAMILY(vk.queue_family_indices.transfer);

        #undef ADD_UNIQUE_FAMILY

        VkDeviceQueueCreateInfo* queue_create_infos = alloc(NULL, sizeof(VkDeviceQueueCreateInfo) * unique_count);
        if (!queue_create_infos) {
            printf("Failed to allocate memory for queue create infos.\n");
            exit(-1);
        }
        for (uint32_t i = 0; i < unique_count; i++) {
            memset(&queue_create_infos[i], 0, sizeof(VkDeviceQueueCreateInfo));
            queue_create_infos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_create_infos[i].queueFamilyIndex = unique_queue_families[i];
            queue_create_infos[i].queueCount = 1;
            queue_create_infos[i].pQueuePriorities = &queue_priority;
            queue_create_infos[i].flags = 0;
            queue_create_infos[i].pNext = NULL;
        }
        VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeature = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
            .pNext = NULL,
            .dynamicRendering = VK_TRUE,
        };
        const char* deviceExtensions[] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME
        };
        VkDeviceCreateInfo device_create_info = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pQueueCreateInfos = queue_create_infos,
            .queueCreateInfoCount = unique_count,
            .pEnabledFeatures = &(VkPhysicalDeviceFeatures){0},
            .enabledExtensionCount = 2,
            .ppEnabledExtensionNames = deviceExtensions,
            .enabledLayerCount = 0, // Deprecated in newer Vulkan versions
            .ppEnabledLayerNames = NULL,
            .pNext = &dynamicRenderingFeature,
        };
        result = vkCreateDevice(vk.physical_device, &device_create_info, NULL, &vk.device);
        if (result != VK_SUCCESS) {
            printf("Failed to create logical vk.device. Error code: %d\n", result);
            free(queue_create_infos);
            exit(-1);
        }
        printf("Logical vk.device created successfully.\n");
        free(queue_create_infos);
    }
    // initializeVmaAllocator
    {
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = vk.physical_device;
        allocatorInfo.device = vk.device;
        allocatorInfo.instance = vk.instance;

        VkResult result = vmaCreateAllocator(&allocatorInfo, &vk.allocator);
        if (result != VK_SUCCESS) {
            printf("Failed to create VMA allocator\n");
            exit(-1);
        }
    }
    // getQueues
    {
        vk.queues.graphics = VK_NULL_HANDLE;
        vk.queues.present = VK_NULL_HANDLE;
        vk.queues.compute = VK_NULL_HANDLE;
        vk.queues.transfer = VK_NULL_HANDLE;

        // Retrieve Graphics Queue
        vkGetDeviceQueue(vk.device, vk.queue_family_indices.graphics, 0, &vk.queues.graphics);
        printf("Graphics queue retrieved.\n");

        // Retrieve Presentation Queue
        if (vk.queue_family_indices.graphics != vk.queue_family_indices.present) {
            vkGetDeviceQueue(vk.device, vk.queue_family_indices.present, 0, &vk.queues.present);
            printf("Presentation queue retrieved.\n");
        } else {
            vk.queues.present = vk.queues.graphics;
            printf("Presentation queue is the same as graphics queue.\n");
        }

        // Retrieve Compute Queue
        if (vk.queue_family_indices.compute != vk.queue_family_indices.graphics && vk.queue_family_indices.compute != vk.queue_family_indices.present) {
            vkGetDeviceQueue(vk.device, vk.queue_family_indices.compute, 0, &vk.queues.compute);
            printf("Compute queue retrieved.\n");
        } else {
            vk.queues.compute = vk.queues.graphics;
            printf("Compute queue is the same as graphics queue.\n");
        }

        // Retrieve Transfer Queue
        if (vk.queue_family_indices.transfer != vk.queue_family_indices.graphics && 
            vk.queue_family_indices.transfer != vk.queue_family_indices.present && 
            vk.queue_family_indices.transfer != vk.queue_family_indices.compute) {
            vkGetDeviceQueue(vk.device, vk.queue_family_indices.transfer, 0, &vk.queues.transfer);
            printf("Transfer queue retrieved.\n");
        } else {
            vk.queues.transfer = vk.queues.graphics;
            printf("Transfer queue is the same as graphics queue.\n");
        }
    }
    // createSwapChain
    {
        vk.swap_chain.extent = (VkExtent2D){width, height};

        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk.physical_device, vk.surface, &surfaceCapabilities);
        if (result != VK_SUCCESS) {
            printf("Failed to get vk.surface capabilities\n");
            exit(-1);
        }

        VkExtent2D actualExtent = surfaceCapabilities.currentExtent.width != UINT32_MAX ?
                                  surfaceCapabilities.currentExtent :
                                  vk.swap_chain.extent;

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(vk.physical_device, vk.surface, &formatCount, NULL);
        if (formatCount == 0) {
            printf("Failed to find vk.surface formats\n");
            exit(-1);
        }
        VkSurfaceFormatKHR* formats = alloc(NULL, sizeof(VkSurfaceFormatKHR) * formatCount);
        if (formats == NULL) {
            printf("Failed to allocate memory for vk.surface formats\n");
            exit(-1);
        }
        vkGetPhysicalDeviceSurfaceFormatsKHR(vk.physical_device, vk.surface, &formatCount, formats);

        // Choose a suitable format (e.g., prefer SRGB)
        VkSurfaceFormatKHR chosenFormat = formats[0];
        for (uint32_t i = 0; i < formatCount; i++) {
            if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
                formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                chosenFormat = formats[i];
                break;
            }
        }
        vk.swap_chain.image_format = chosenFormat.format;
        free(formats);

        uint32_t minImageCount = surfaceCapabilities.minImageCount + 1;
        if (surfaceCapabilities.maxImageCount > 0 && minImageCount > surfaceCapabilities.maxImageCount) {
            minImageCount = surfaceCapabilities.maxImageCount;
        }

        VkQueueFamilyIndices i = vk.queue_family_indices;
        VkSwapchainCreateInfoKHR swapchainCreateInfo = {
            .sType           = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface         = vk.surface,
            .minImageCount   = minImageCount,
            .imageFormat     = chosenFormat.format,
            .imageColorSpace = chosenFormat.colorSpace,
            .imageExtent     = actualExtent,
            .imageArrayLayers = 1,
            .imageUsage      = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageSharingMode      = i.graphics != i.present ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = i.graphics != i.present ? 2 : 0,
            .pQueueFamilyIndices   = i.graphics != i.present ? (uint32_t[]){i.graphics, i.present} : NULL,
            .preTransform   = surfaceCapabilities.currentTransform,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode    = VK_PRESENT_MODE_FIFO_KHR,
            .clipped        = VK_TRUE,
            .oldSwapchain   = VK_NULL_HANDLE
        };
        result = vkCreateSwapchainKHR(vk.device, &swapchainCreateInfo, NULL, &vk.swap_chain.swap_chain);
        if (result != VK_SUCCESS) {
            printf("Failed to create swapchain\n");
            exit(-1);
        }

        // get VkImages 
        vk.swap_chain.image_count = 0;
        vkGetSwapchainImagesKHR(vk.device, vk.swap_chain.swap_chain, &vk.swap_chain.image_count, NULL);
        vk.swap_chain.images_p = alloc(vk.swap_chain.images_p, sizeof(VkImage) * vk.swap_chain.image_count);
        if (vk.swap_chain.images_p == NULL) {
            printf("Failed to allocate memory for swapchain images\n");
            exit(-1);
        }
        vkGetSwapchainImagesKHR(vk.device, vk.swap_chain.swap_chain, &vk.swap_chain.image_count, vk.swap_chain.images_p);

        // create VkImageViews
        vk.swap_chain.image_views_p = alloc(vk.swap_chain.image_views_p, sizeof(VkImageView) * vk.swap_chain.image_count);
        if (vk.swap_chain.image_views_p == NULL) {
            printf("Failed to allocate memory for image views\n");
            exit(-1);
        }
        for (uint32_t i = 0; i < vk.swap_chain.image_count; i++) {
            VkImageViewCreateInfo viewInfo = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = vk.swap_chain.images_p[i],  // Use the stored VkImage
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = vk.swap_chain.image_format,
                .components = {
                    .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .a = VK_COMPONENT_SWIZZLE_IDENTITY,
                },
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
            };

            if (vkCreateImageView(vk.device, &viewInfo, NULL, &vk.swap_chain.image_views_p[i]) != VK_SUCCESS) {
                printf("Failed to create image view %u\n", i);
                exit(-1);
            }
        }
    }
    // createDescriptorPool
    {
        VkDescriptorPoolCreateInfo pool_info = {
            .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .poolSizeCount = 1,
            .pPoolSizes    = &(VkDescriptorPoolSize) {
                .type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1
            },
            .maxSets       = 1
        };
        if (vkCreateDescriptorPool(vk.device, &pool_info, NULL, &vk.descriptor_pool) != VK_SUCCESS) {
            printf("Failed to create descriptor pool\n");
            exit(-1);
        }
    }
    // createCommandPool
    {
        VkCommandPoolCreateInfo poolInfo = {
            .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .queueFamilyIndex = vk.queue_family_indices.graphics,
            .flags            = 0 // Optional flags
        };
        if (vkCreateCommandPool(vk.device, &poolInfo, NULL, &vk.command_pool) != VK_SUCCESS) {
            printf("Failed to create command pool\n");
            exit(-1);
        }
    }

    return vk;
}

void VK_startApp(
    VK* vk,
    VkSemaphore imageAvailableSemaphore,
    VkSemaphore renderFinishedSemaphore,
    VkFence inFlightFence,
    VkCommandBuffer* commandBuffers,
    VkBuffer uniformBuffer,
    VmaAllocation uniformBufferAllocation,
    Buffer instance_buffer
){
    int running = 1;
    SDL_Event event;

    unsigned int tmp_i = 0;
    
    while (running) {

        // Handle SDL Events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }
        }

        // for testing
        {
            tmp_i=ALL_INSTANCE_COUNT;
            clearBuffer(vk, instance_buffer, 0);
            updateBuffer(vk, instance_buffer, 0, all_instances, sizeof(InstanceData) * tmp_i);
            if (tmp_i==ALL_INSTANCE_COUNT) {
                tmp_i = 0;
            }
            else {
                tmp_i++;
            }
        }

        // Update Uniform Buffer Data
        UniformBufferObject ubo = {};
        ubo.targetWidth = (float)vk->swap_chain.extent.width;
        ubo.targetHeight = (float)vk->swap_chain.extent.height;

        void* data;
        VkResult map_result = vmaMapMemory(vk->allocator, uniformBufferAllocation, &data);
        if (map_result != VK_SUCCESS) {
            printf("Failed to map uniform buffer memory: %d\n", map_result);
            running = 0;
            continue;
        }
        memcpy(data, &ubo, sizeof(ubo));
        vmaUnmapMemory(vk->allocator, uniformBufferAllocation);
        vkWaitForFences(vk->device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
        vkResetFences(vk->device, 1, &inFlightFence);

        // Acquire the next image from the swap chain
        uint32_t image_index;
        VkResult acquire_result = vkAcquireNextImageKHR(vk->device, vk->swap_chain.swap_chain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &image_index);
        if (acquire_result == VK_ERROR_OUT_OF_DATE_KHR || acquire_result == VK_SUBOPTIMAL_KHR) {
            printf("Swapchain out of date or suboptimal. Consider recreating swapchain.\n");
            running = 0;
            continue;
        }
        else if (acquire_result != VK_SUCCESS && acquire_result != VK_SUBOPTIMAL_KHR) {
            printf("Failed to acquire swapchain image: %d\n", acquire_result);
            running = 0;
            continue;
        }

        // Submit the command buffer
        VkSubmitInfo submit_info = {
            .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount   = 1,
            .pWaitSemaphores      = (VkSemaphore[]){ imageAvailableSemaphore },
            .pWaitDstStageMask    = (VkPipelineStageFlags[]){ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT },
            .commandBufferCount   = 1,
            .pCommandBuffers      = &commandBuffers[image_index],
            .signalSemaphoreCount = 1,
            .pSignalSemaphores    = (VkSemaphore[]){ renderFinishedSemaphore }
        };
        VkResult submit_result = vkQueueSubmit(vk->queues.graphics, 1, &submit_info, inFlightFence);
        if (submit_result != VK_SUCCESS) {
            printf("Failed to submit draw command buffer: %d\n", submit_result);
            running = 0;
            continue;
        }

        // Present the image
        VkPresentInfoKHR present_info = {
            .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores    = (VkSemaphore[]){ renderFinishedSemaphore },
            .swapchainCount     = 1,
            .pSwapchains        = (VkSwapchainKHR[]){ vk->swap_chain.swap_chain },
            .pImageIndices      = &image_index  
        };

        VkResult present_result = vkQueuePresentKHR(vk->queues.present, &present_info);
        if (present_result == VK_ERROR_OUT_OF_DATE_KHR || present_result == VK_SUBOPTIMAL_KHR) {
            printf("Swapchain out of date or suboptimal during present. Consider recreating swapchain.\n");
            running = 0;
            continue;
        }
        else if (present_result != VK_SUCCESS) {
            printf("Failed to present swapchain image: %d\n", present_result);
            running = 0;
            continue;
        }

        // Optionally wait for the present queue to be idle
        vkQueueWaitIdle(vk->queues.present);
        //usleep(100000);
    }
}

void VK_destroy(
    VK* p_vk,
    VkPipeline graphicsPipeline,
    VkPipelineLayout pipelineLayout,
    VkDescriptorSetLayout descriptorSetLayout,
    VkBuffer uniformBuffer,
    VmaAllocation uniformBufferAllocation,
    VkBuffer instanceBuffer,
    VmaAllocation instanceBufferAllocation,
    VkDescriptorSet descriptorSet,
    VkCommandBuffer* commandBuffers,
    VkSemaphore imageAvailableSemaphore,
    VkSemaphore renderFinishedSemaphore,
    VkFence inFlightFence
) {
    shaderc_compiler_release(p_vk->shaderc_compiler);
    if (p_vk->device != VK_NULL_HANDLE)
        vkDeviceWaitIdle(p_vk->device);
    if (graphicsPipeline != VK_NULL_HANDLE) 
        vkDestroyPipeline(p_vk->device, graphicsPipeline, NULL);
    if (pipelineLayout != VK_NULL_HANDLE) 
        vkDestroyPipelineLayout(p_vk->device, pipelineLayout, NULL);
    if (descriptorSetLayout != VK_NULL_HANDLE)
        vkDestroyDescriptorSetLayout(p_vk->device, descriptorSetLayout, NULL);
    if (p_vk->descriptor_pool != VK_NULL_HANDLE) 
        vkDestroyDescriptorPool(p_vk->device, p_vk->descriptor_pool, NULL);
    if (uniformBuffer != VK_NULL_HANDLE)
        vmaDestroyBuffer(p_vk->allocator, uniformBuffer, uniformBufferAllocation);
    if (instanceBuffer != VK_NULL_HANDLE)
        vmaDestroyBuffer(p_vk->allocator, instanceBuffer, instanceBufferAllocation);
    if (p_vk->swap_chain.image_views_p != NULL) {
        for (uint32_t i = 0; i < p_vk->swap_chain.image_count; i++) {
            if (p_vk->swap_chain.image_views_p[i] != VK_NULL_HANDLE) {
                vkDestroyImageView(p_vk->device, p_vk->swap_chain.image_views_p[i], NULL);
            }
        }
        free(p_vk->swap_chain.image_views_p);
    }
    if (p_vk->swap_chain.swap_chain != VK_NULL_HANDLE) 
        vkDestroySwapchainKHR(p_vk->device, p_vk->swap_chain.swap_chain, NULL);
    if (p_vk->command_pool != VK_NULL_HANDLE)
        vkDestroyCommandPool(p_vk->device, p_vk->command_pool, NULL);
    if (imageAvailableSemaphore != VK_NULL_HANDLE) 
        vkDestroySemaphore(p_vk->device, imageAvailableSemaphore, NULL);
    if (renderFinishedSemaphore != VK_NULL_HANDLE) 
        vkDestroySemaphore(p_vk->device, renderFinishedSemaphore, NULL);
    if (inFlightFence != VK_NULL_HANDLE)
        vkDestroyFence(p_vk->device, inFlightFence, NULL);
    if (p_vk->device != VK_NULL_HANDLE)
        vkDestroyDevice(p_vk->device, NULL);
    if (p_vk->debug_messenger != VK_NULL_HANDLE) {
        PFN_vkDestroyDebugUtilsMessengerEXT funcDestroyDebugUtilsMessengerEXT = 
            (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(p_vk->instance, "vkDestroyDebugUtilsMessengerEXT");
        if (funcDestroyDebugUtilsMessengerEXT != NULL) {
            funcDestroyDebugUtilsMessengerEXT(p_vk->instance, p_vk->debug_messenger, NULL);
        }
    }
    if (p_vk->surface != VK_NULL_HANDLE)
        vkDestroySurfaceKHR(p_vk->instance, p_vk->surface, NULL);
    if (p_vk->swap_chain.swap_chain != VK_NULL_HANDLE)
        vkDestroySwapchainKHR(p_vk->device, p_vk->swap_chain.swap_chain, NULL);
    if (p_vk->allocator != VK_NULL_HANDLE)
        vmaDestroyAllocator(p_vk->allocator);
    if (p_vk->window_p != NULL)
        SDL_DestroyWindow(p_vk->window_p);
    SDL_Quit();
}