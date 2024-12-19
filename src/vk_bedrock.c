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
        .size = {200.0f, 200.0f},
        .rotation = 0.0f,
        .corner_radius = 10.0f,
        .color = 0x88880088, // Red color in RGBA8
        .tex_index = 1,
        .tex_rect = {0.0f, 0.0f, 1.0f, 1.0f}
    },
    // Instance 1
    {
        .pos = {100.0f, 100.0f},
        .size = {100.0f, 100.0f},
        .rotation = 30.0f,
        .corner_radius = 10.0f,
        .color = 0xFF00FF88, // Green color in RGBA8
        .tex_index = 1,
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
        .pos = {200.0f, 200.0f},
        .size = {200.0f, 200.0f},
        .rotation = 90.0f,
        .corner_radius = 10.0f,
        .color = 0xFFFF00FF, // Cyan color in RGBA8
        .tex_index = 1,
        .tex_rect = {0.0f, 0.0f, 1.0f, 1.0f}
    },
    // Instance 4
    {
        .pos = {400.0f, 400.0f},
        .size = {100.0f, 100.0f},
        .rotation = 120.0f,
        .corner_radius = 10.0f,
        .color = 0xFF00FFFF, // Yellow color in RGBA8
        .tex_index = 1,
        .tex_rect = {0.0f, 0.0f, 1.0f, 1.0f}
    },
};
unsigned int indices[INDICES_COUNT] = { 0, 1, 2, 3, 4 };


VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    printf("Vulkan Debug: %s\n", pCallbackData->pMessage);
    return VK_FALSE;
}

Vk vk_Create(unsigned int width, unsigned int height, const char* title) {
    Vk vk;
    memset(&vk, 0, sizeof(Vk));

    VkResult result;

    // Query the highest supported Vulkan version by the loader
    unsigned int loader_version = 0;
    if (vkEnumerateInstanceVersion != NULL) {
        TRACK(vkEnumerateInstanceVersion(&loader_version));
    } else {
        // If not available, assume at least Vulkan 1.0
        loader_version = VK_API_VERSION_1_0;
    }

    printf("Loader supports Vulkan version: %u.%u.%u\n",
           VK_VERSION_MAJOR(loader_version),
           VK_VERSION_MINOR(loader_version),
           VK_VERSION_PATCH(loader_version));

    // Desired API version (we want Vulkan 1.3)
    unsigned int desired_version = VK_API_VERSION_1_3;
    if (loader_version < desired_version) {
        printf("Warning: Loader does not support Vulkan 1.3, will use lower version.\n");
        desired_version = loader_version;
    }

    // shaderc
    {
        vk.shaderc_compiler = shaderc_compiler_initialize();
        VERIFY(vk.shaderc_compiler, "failed to initialize\n ");
        debug(vk.shaderc_options = shaderc_compile_options_initialize());
        VERIFY(vk.shaderc_options, "failed to initialize\n ");
        debug(shaderc_compile_options_set_optimization_level(vk.shaderc_options, shaderc_optimization_level_zero));
        debug(shaderc_compile_options_set_target_env(vk.shaderc_options, shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3));
    }
    // createWindow
    {
        VERIFY(SDL_Init(SDL_INIT_VIDEO) == 0, "failed to initialize\n ");
        debug(vk.window_p = SDL_CreateWindow( title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN ));
        VERIFY(vk.window_p, "failed to create\n ");
    }
    // createVulkanInstance
    {
        unsigned int totalExtensionCount = 1;
        const char** allExtensions = NULL;
        unsigned int validationLayerCount = 0;
        const char* validationLayers[] = { "VK_LAYER_KHRONOS_validation" };

        unsigned int sdlExtensionCount = 0;
        VERIFY(SDL_Vulkan_GetInstanceExtensions((SDL_Window*)vk.window_p, &sdlExtensionCount, NULL), "%s\n ", SDL_GetError());

        const char** sdlExtensions = (const char**)alloc(NULL, sizeof(const char*) * sdlExtensionCount);
        VERIFY(sdlExtensions, "failed to allocate memory\n ");

        VERIFY(SDL_Vulkan_GetInstanceExtensions((SDL_Window*)vk.window_p, &sdlExtensionCount, sdlExtensions), "%s\n ", SDL_GetError());
        // Add debug utils extension
        totalExtensionCount = sdlExtensionCount + 1;
        allExtensions = (const char**)alloc(NULL, sizeof(const char*) * totalExtensionCount);
        VERIFY(allExtensions, "failed to allocate memory\n ");
        memcpy(allExtensions, sdlExtensions, sizeof(const char*) * sdlExtensionCount);
        allExtensions[sdlExtensionCount] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

        validationLayerCount = 1;

        VkApplicationInfo appInfo = {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "Logos Application",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "Logos Engine",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = desired_version
        };

        VkInstanceCreateInfo instanceCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &appInfo,
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

        TRACK(result = vkCreateInstance(&instanceCreateInfo, NULL, &vk.instance));
        free(sdlExtensions);
        free(allExtensions);
        VERIFY(result==VK_SUCCESS, "failed to create VkInstance\n ");
    }
    // setupDebugMessenger
    {
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
            VERIFY(funcCreateDebugUtilsMessengerEXT(vk.instance, &debugCreateInfo, NULL, &vk.debug_messenger) == VK_SUCCESS, "Failed to set up debug messenger\n ");
        } else {
            printf("vkCreateDebugUtilsMessengerEXT not available\n");
            vk.debug_messenger = VK_NULL_HANDLE;
        }
    }
    // createSurface
    {
        vk.surface = VK_NULL_HANDLE;
        VERIFY(SDL_Vulkan_CreateSurface((SDL_Window*)vk.window_p, vk.instance, &vk.surface), "%s\n", SDL_GetError());
    }
    // getPhysicalDevice
    {
        unsigned int device_count = 0;
        result = vkEnumeratePhysicalDevices(vk.instance, &device_count, NULL);
        VERIFY(!(result != VK_SUCCESS || device_count == 0), "Failed to find GPUs with Vulkan support\n");

        VkPhysicalDevice* devices = alloc(NULL, sizeof(VkPhysicalDevice) * device_count);
        VERIFY(devices, "Failed to allocate memory for physical devices\n");
        VERIFY(vkEnumeratePhysicalDevices(vk.instance, &device_count, devices)==VK_SUCCESS, "Failed to enumerate physical devices\n");

        // For simplicity, choose the first device
        vk.physical_device = devices[0];
        free(devices);

        // Check device properties to see if Vulkan 1.3 is supported
        VkPhysicalDeviceProperties deviceProps;
        vkGetPhysicalDeviceProperties(vk.physical_device, &deviceProps);
        unsigned int device_api_version = deviceProps.apiVersion;
        printf("Device supports Vulkan version: %u.%u.%u\n",
               VK_VERSION_MAJOR(device_api_version),
               VK_VERSION_MINOR(device_api_version),
               VK_VERSION_PATCH(device_api_version));

        bool wants_dynamic_rendering = true;
        if (device_api_version >= VK_API_VERSION_1_3) {
            // Device supports Vulkan 1.3 natively
            printf("Vulkan 1.3 is supported by the device.\n");
            wants_dynamic_rendering = false; // Core 1.3 includes dynamic rendering
        }

        // If we need dynamic rendering extension (if not Vulkan 1.3)
        bool dynamicRenderingSupported = false;
        if (wants_dynamic_rendering) {
            unsigned int extensionCount = 0;
            vkEnumerateDeviceExtensionProperties(vk.physical_device, NULL, &extensionCount, NULL);
            VkExtensionProperties* extensions = malloc(sizeof(VkExtensionProperties) * extensionCount);
            vkEnumerateDeviceExtensionProperties(vk.physical_device, NULL, &extensionCount, extensions);

            for (unsigned int i = 0; i < extensionCount; i++) {
                if (strcmp(extensions[i].extensionName, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME) == 0) {
                    dynamicRenderingSupported = true;
                    break;
                }
            }
            free(extensions);
            VERIFY(dynamicRenderingSupported, "Dynamic Rendering not supported on this device.\n");
        }
    }
    // getQueueFamilyIndices
    {
        vk.queue_family_indices.graphics = UINT32_MAX;
        vk.queue_family_indices.present = UINT32_MAX;
        vk.queue_family_indices.compute = UINT32_MAX;
        vk.queue_family_indices.transfer = UINT32_MAX;

        unsigned int queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(vk.physical_device, &queue_family_count, NULL);
        VERIFY(queue_family_count != 0, "Failed to find any queue families\n");

        VkQueueFamilyProperties* queue_families = alloc(NULL, sizeof(VkQueueFamilyProperties) * queue_family_count);
        VERIFY(queue_families, "Failed to allocate memory for queue family properties\n");

        vkGetPhysicalDeviceQueueFamilyProperties(vk.physical_device, &queue_family_count, queue_families);

        for (unsigned int i = 0; i < queue_family_count; i++) {
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
        VERIFY(!(vk.queue_family_indices.graphics == UINT32_MAX || vk.queue_family_indices.present == UINT32_MAX), "Failed to find required queue families\n");
    }
    // check for driver compatibility
    {
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(vk.physical_device, &deviceFeatures);

        if (!deviceFeatures.samplerAnisotropy) {
            printf("samplerAnisotropy is not supported\n");
        } else {
            printf("samplerAnisotropy is supported\n");
        }
    }
    // getDevice
    {
        // Check again if we need dynamic rendering
        VkPhysicalDeviceProperties deviceProps;
        vkGetPhysicalDeviceProperties(vk.physical_device, &deviceProps);
        bool use_dynamic_rendering_extension = (deviceProps.apiVersion < VK_API_VERSION_1_3);

        // Define queue priorities
        float queue_priority = 1.0f;

        unsigned int unique_queue_families[4];
        unsigned int unique_count = 0;

        #define ADD_UNIQUE_FAMILY(family) \
            do { \
                bool exists = false; \
                for (unsigned int i = 0; i < unique_count; i++) { \
                    if (unique_queue_families[i] == family) { \
                        exists = true; \
                        break; \
                    } \
                } \
                if (!exists && unique_count < 4) { \
                    unique_queue_families[unique_count++] = family; \
                } \
            } while(0)

        ADD_UNIQUE_FAMILY(vk.queue_family_indices.graphics);
        ADD_UNIQUE_FAMILY(vk.queue_family_indices.present);
        ADD_UNIQUE_FAMILY(vk.queue_family_indices.compute);
        ADD_UNIQUE_FAMILY(vk.queue_family_indices.transfer);

        #undef ADD_UNIQUE_FAMILY

        VkDeviceQueueCreateInfo* queue_create_infos = alloc(NULL, sizeof(VkDeviceQueueCreateInfo) * unique_count);
        VERIFY(queue_create_infos, "Failed to allocate memory for queue create infos.\n");
        for (unsigned int i = 0; i < unique_count; i++) {
            memset(&queue_create_infos[i], 0, sizeof(VkDeviceQueueCreateInfo));
            queue_create_infos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_create_infos[i].queueFamilyIndex = unique_queue_families[i];
            queue_create_infos[i].queueCount = 1;
            queue_create_infos[i].pQueuePriorities = &queue_priority;
        }

        VkDeviceCreateInfo device_create_info = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = &(VkPhysicalDeviceDynamicRenderingFeatures) {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
                .dynamicRendering = VK_TRUE,
            },
            .pQueueCreateInfos = queue_create_infos,
            .queueCreateInfoCount = unique_count,
            .pEnabledFeatures = &(VkPhysicalDeviceFeatures){
                .samplerAnisotropy = VK_TRUE,
            },
            .enabledExtensionCount = use_dynamic_rendering_extension ? 2 : 1,
            .ppEnabledExtensionNames = &(const char*) {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME
            },
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = NULL,
        };

        TRACK(result = vkCreateDevice(vk.physical_device, &device_create_info, NULL, &vk.device));
        VERIFY(result == VK_SUCCESS, "Failed to create logical vk.device. Error code: %d\n", result);
        printf("Logical vk.device created successfully.\n");
        free(queue_create_infos);
    }
    // initializeVmaAllocator
    {
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = vk.physical_device;
        allocatorInfo.device = vk.device;
        allocatorInfo.instance = vk.instance;

        TRACK(VkResult result = vmaCreateAllocator(&allocatorInfo, &vk.allocator));
        VERIFY(result == VK_SUCCESS, "Failed to create VMA allocator\n");
    }
    // getQueues
    {
        vk.queues.graphics = VK_NULL_HANDLE;
        vk.queues.present = VK_NULL_HANDLE;
        vk.queues.compute = VK_NULL_HANDLE;
        vk.queues.transfer = VK_NULL_HANDLE;

        vkGetDeviceQueue(vk.device, vk.queue_family_indices.graphics, 0, &vk.queues.graphics);

        if (vk.queue_family_indices.graphics != vk.queue_family_indices.present) {
            vkGetDeviceQueue(vk.device, vk.queue_family_indices.present, 0, &vk.queues.present);
        } else {
            vk.queues.present = vk.queues.graphics;
        }

        if (vk.queue_family_indices.compute != vk.queue_family_indices.graphics &&
            vk.queue_family_indices.compute != vk.queue_family_indices.present) {
            vkGetDeviceQueue(vk.device, vk.queue_family_indices.compute, 0, &vk.queues.compute);
        } else {
            vk.queues.compute = vk.queues.graphics;
        }

        if (vk.queue_family_indices.transfer != vk.queue_family_indices.graphics && 
            vk.queue_family_indices.transfer != vk.queue_family_indices.present && 
            vk.queue_family_indices.transfer != vk.queue_family_indices.compute) {
            vkGetDeviceQueue(vk.device, vk.queue_family_indices.transfer, 0, &vk.queues.transfer);
        } else {
            vk.queues.transfer = vk.queues.graphics;
        }
    }
    // createSwapChain
    {
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        TRACK(VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk.physical_device, vk.surface, &surfaceCapabilities));
        VERIFY(result == VK_SUCCESS, "Failed to get vk.surface capabilities\n");

        VkExtent2D extent = surfaceCapabilities.currentExtent.width != UINT32_MAX ?
                                  surfaceCapabilities.currentExtent :
                                  (VkExtent2D){width, height};

        unsigned int format_count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(vk.physical_device, vk.surface, &format_count, NULL);
        VERIFY(format_count > 0, "Failed to find vk.surface formats\n");
        TRACK(VkSurfaceFormatKHR* formats = alloc(NULL, sizeof(VkSurfaceFormatKHR) * format_count));
        VERIFY(formats, "Failed to allocate memory for vk.surface formats\n");
        TRACK(vkGetPhysicalDeviceSurfaceFormatsKHR(vk.physical_device, vk.surface, &format_count, formats));

        // Choose a suitable format (prefer R8G8B8A8_SRGB then R8G8B8A8_UNORM)
        VkSurfaceFormatKHR chosenFormat = formats[0]; // Default to first format if none of the preferred are available
        for (unsigned int i = 0; i < format_count; i++) {
            if (formats[i].format == VK_FORMAT_R8G8B8A8_SRGB && 
                formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                chosenFormat = formats[i];
                break;
            } else if (formats[i].format == VK_FORMAT_R8G8B8A8_UNORM && 
                       formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                chosenFormat = formats[i];
                break;
            }
        }
        
        VkFormat format = chosenFormat.format;
        printf("swapchain image format = %d\n", format);
        free(formats);

        unsigned int minImageCount = surfaceCapabilities.minImageCount;
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
            .imageExtent     = extent,
            .imageArrayLayers = 1,
            .imageUsage      = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageSharingMode      = i.graphics != i.present ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = i.graphics != i.present ? 2 : 0,
            .pQueueFamilyIndices   = i.graphics != i.present ? (unsigned int[]){i.graphics, i.present} : NULL,
            .preTransform   = surfaceCapabilities.currentTransform,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode    = VK_PRESENT_MODE_FIFO_KHR,
            .clipped        = VK_TRUE,
            .oldSwapchain   = VK_NULL_HANDLE
        };

        TRACK(result = vkCreateSwapchainKHR(vk.device, &swapchainCreateInfo, NULL, &vk.swap_chain));
        VERIFY(result == VK_SUCCESS, "Failed to create swapchain\n");

        vk.images_count = 0;
        TRACK(vkGetSwapchainImagesKHR(vk.device, vk.swap_chain, &vk.images_count, NULL));
        VERIFY(vk.images_count > 0, "there is 0 images in swapchain");
        if (surfaceCapabilities.maxImageCount > 0) {
            VERIFY(vk.images_count <= surfaceCapabilities.maxImageCount, "there is more than expected images in swapchain. images_count = %d. maxImageCount = %d", vk.images_count, surfaceCapabilities.maxImageCount);
        } else {
            printf("Note: maxImageCount is 0, indicating no upper limit on image count.\n");
        }

        TRACK(vk.p_images = alloc(vk.p_images, sizeof(Image) * vk.images_count));
        VERIFY(vk.p_images, "Failed to allocate memory for swapchain images\n");

        VkImage* p_tmp_images = (VkImage*)alloc(NULL, sizeof(VkImage) * vk.images_count);
        VERIFY(p_tmp_images, "Failed to allocate memory for temporary image array\n");
        TRACK(vkGetSwapchainImagesKHR(vk.device, vk.swap_chain, &vk.images_count, p_tmp_images));

        for (unsigned int i = 0; i < vk.images_count; i++) {
            vk.p_images[i].image = p_tmp_images[i];
            vk.p_images[i].extent = extent;
            vk.p_images[i].format = format;
            vk.p_images[i].layout = VK_IMAGE_LAYOUT_UNDEFINED;
        }

        free(p_tmp_images);

        for (unsigned int i = 0; i < vk.images_count; i++) {
            VkImageViewCreateInfo view_info = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = vk.p_images[i].image,
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = vk.p_images[i].format,
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

            TRACK(result = vkCreateImageView(vk.device, &view_info, NULL, &vk.p_images[i].view));
            VERIFY(result == VK_SUCCESS, "Failed to create image view %u\n", i);
        }
    }
    // createDescriptorPool
    {
        VkDescriptorPoolCreateInfo pool_info = {
            .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .poolSizeCount = 4,
            .pPoolSizes    = (VkDescriptorPoolSize[]) {
                {
                    .type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = 20,
                },
                {
                    .type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = 100,
                },
                {
                    .type            = VK_DESCRIPTOR_TYPE_SAMPLER,
                    .descriptorCount = 20,
                },
                {
                    .type            = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                    .descriptorCount = 20,
                },
            },
            .maxSets       = 40
        };

        TRACK(result = vkCreateDescriptorPool(vk.device, &pool_info, NULL, &vk.descriptor_pool));
        VERIFY(result == VK_SUCCESS, "Failed to create descriptor pool\n");
    }
    // createCommandPool
    {
        VkCommandPoolCreateInfo poolInfo = {
            .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .queueFamilyIndex = vk.queue_family_indices.graphics,
            .flags            = 0
        };

        TRACK(result = vkCreateCommandPool(vk.device, &poolInfo, NULL, &vk.command_pool));
        VERIFY(result == VK_SUCCESS, "Failed to create command pool\n");
    }

    return vk;
}


void vk_StartApp(
    Vk* vk,
    VkSemaphore imageAvailableSemaphore,
    VkSemaphore renderFinishedSemaphore,
    VkFence inFlightFence,
    VkCommandBuffer* commandBuffers,
    VkBuffer uniformBuffer,
    VmaAllocation uniformBufferAllocation,
    Buffer instance_buffer)
{
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
            //tmp_i=ALL_INSTANCE_COUNT;
            TRACK(vk_Buffer_Clear(vk, instance_buffer, 0));
            TRACK(vk_Buffer_Update(vk, instance_buffer, 0, all_instances, sizeof(InstanceData) * tmp_i));
            if (tmp_i==ALL_INSTANCE_COUNT) {
                tmp_i = 0;
            }
            else {
                tmp_i++;
            }
        }

        
        // Update Uniform Buffer Data
        UniformBufferObject ubo = {};
        ubo.targetWidth = (float)vk->p_images[0].extent.width;
        ubo.targetHeight = (float)vk->p_images[0].extent.height;

        void* data;
        TRACK(VkResult map_result = vmaMapMemory(vk->allocator, uniformBufferAllocation, &data));
        VERIFY(map_result == VK_SUCCESS, "Failed to map uniform buffer memory: %d\n", map_result);
        TRACK(memcpy(data, &ubo, sizeof(ubo)));
        TRACK(vmaUnmapMemory(vk->allocator, uniformBufferAllocation));
        TRACK(vkWaitForFences(vk->device, 1, &inFlightFence, VK_TRUE, UINT64_MAX));
        TRACK(vkResetFences(vk->device, 1, &inFlightFence));

        // Acquire the next image from the swap chain
        unsigned int image_index;
        TRACK(VkResult acquire_result = vkAcquireNextImageKHR(vk->device, vk->swap_chain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &image_index));
        VERIFY(!(acquire_result == VK_ERROR_OUT_OF_DATE_KHR || acquire_result == VK_SUBOPTIMAL_KHR), "Swapchain out of date or suboptimal. Consider recreating swapchain.\n");
        VERIFY(!(acquire_result != VK_SUCCESS && acquire_result != VK_SUBOPTIMAL_KHR), "Failed to acquire swapchain image: %d\n", acquire_result);

        //vk_Image_TransitionLayoutWithoutCommandBuffer(vk, &vk->p_images[image_index], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

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
        TRACK(VkResult submit_result = vkQueueSubmit(vk->queues.graphics, 1, &submit_info, inFlightFence));
        VERIFY(submit_result == VK_SUCCESS, "Failed to submit draw command buffer: %d\n", submit_result);


        //vk_Image_TransitionLayoutWithoutCommandBuffer(vk, &vk->p_images[image_index], VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        // Present the image
        VkPresentInfoKHR present_info = {
            .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores    = (VkSemaphore[]){ renderFinishedSemaphore },
            .swapchainCount     = 1,
            .pSwapchains        = (VkSwapchainKHR[]){ vk->swap_chain },
            .pImageIndices      = &image_index  
        };

        TRACK(VkResult present_result = vkQueuePresentKHR(vk->queues.present, &present_info));
        VERIFY(!(present_result == VK_ERROR_OUT_OF_DATE_KHR || present_result == VK_SUBOPTIMAL_KHR), "Swapchain out of date or suboptimal during present. Consider recreating swapchain.\n");
        VERIFY(present_result == VK_SUCCESS, "Failed to present swapchain image: %d\n", present_result);

        // Optionally wait for the present queue to be idle
        TRACK(vkQueueWaitIdle(vk->queues.present));
        //usleep(100000);
    }
}

void vk_Destroy(
    Vk* p_vk,
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

    for (unsigned int i = 0; i < p_vk->images_count; i++) {
        if (p_vk->p_images[i].view != VK_NULL_HANDLE) {
            vkDestroyImageView(p_vk->device, p_vk->p_images[i].view, NULL);
        }
    }
    free(p_vk->p_images);

    if (p_vk->swap_chain != VK_NULL_HANDLE) 
        vkDestroySwapchainKHR(p_vk->device, p_vk->swap_chain, NULL);
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
    if (p_vk->swap_chain != VK_NULL_HANDLE)
        vkDestroySwapchainKHR(p_vk->device, p_vk->swap_chain, NULL);
    if (p_vk->allocator != VK_NULL_HANDLE)
        vmaDestroyAllocator(p_vk->allocator);
    if (p_vk->window_p != NULL)
        SDL_DestroyWindow(p_vk->window_p);
    SDL_Quit();
}