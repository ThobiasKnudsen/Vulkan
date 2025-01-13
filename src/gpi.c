#include "gpi.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define DEBUG
#include "debug.h"


void* alloc(void* ptr, size_t size) {
    void* tmp = (ptr == NULL) ? malloc(size) : realloc(ptr, size);
    if (!tmp) {
        printf("Memory allocation failed\n");
        exit(-1);
    }
    return tmp;
}


// VULKAN =================================================================================================================================
VKAPI_ATTR VkBool32 VKAPI_CALL      _vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    printf("Vulkan Debug: %s\n", pCallbackData->pMessage);
    return VK_FALSE;
}

// context ================================================================================================================================
Gpi_Context                         gpi_Context_Create(
    unsigned int width, 
    unsigned int height, 
    const char* title) 
{
    Gpi_Context ctx;
    memset(&ctx, 0, sizeof(Gpi_Context));

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
        ctx.shaderc_compiler = shaderc_compiler_initialize();
        VERIFY(ctx.shaderc_compiler, "failed to initialize\n ");
        debug(ctx.shaderc_options = shaderc_compile_options_initialize());
        VERIFY(ctx.shaderc_options, "failed to initialize\n ");
        debug(shaderc_compile_options_set_optimization_level(ctx.shaderc_options, shaderc_optimization_level_zero));
        debug(shaderc_compile_options_set_target_env(ctx.shaderc_options, shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3));
    }
    // createWindow
    {
        VERIFY(SDL_Init(SDL_INIT_VIDEO) == 0, "failed to initialize\n ");
        debug(ctx.window_p = SDL_CreateWindow( title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN ));
        VERIFY(ctx.window_p, "failed to create\n ");
    }
    // createVulkanInstance
    {
        unsigned int totalExtensionCount = 1;
        const char** allExtensions = NULL;
        unsigned int validationLayerCount = 0;
        const char* validationLayers[] = { "VK_LAYER_KHRONOS_validation" };

        unsigned int sdlExtensionCount = 0;
        VERIFY(SDL_Vulkan_GetInstanceExtensions((SDL_Window*)ctx.window_p, &sdlExtensionCount, NULL), "%s\n ", SDL_GetError());

        const char** sdlExtensions = (const char**)alloc(NULL, sizeof(const char*) * sdlExtensionCount);
        VERIFY(sdlExtensions, "failed to allocate memory\n ");

        VERIFY(SDL_Vulkan_GetInstanceExtensions((SDL_Window*)ctx.window_p, &sdlExtensionCount, sdlExtensions), "%s\n ", SDL_GetError());
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
                .pfnUserCallback = _vk_debug_callback,
                .pUserData = NULL
            }
        };

        TRACK(result = vkCreateInstance(&instanceCreateInfo, NULL, &ctx.instance));
        free(sdlExtensions);
        free(allExtensions);
        VERIFY(result==VK_SUCCESS, "failed to create VkInstance\n ");
    }
    // setupDebugMessenger
    {
        PFN_vkCreateDebugUtilsMessengerEXT funcCreateDebugUtilsMessengerEXT =
            (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(ctx.instance, "vkCreateDebugUtilsMessengerEXT");
        if (funcCreateDebugUtilsMessengerEXT != NULL) {
            VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {
                .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
                .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                   VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                   VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
                .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
                .pfnUserCallback = _vk_debug_callback,
                .pUserData = NULL
            };

            ctx.debug_messenger = VK_NULL_HANDLE;
            VERIFY(funcCreateDebugUtilsMessengerEXT(ctx.instance, &debugCreateInfo, NULL, &ctx.debug_messenger) == VK_SUCCESS, "Failed to set up debug messenger\n ");
        } else {
            printf("vkCreateDebugUtilsMessengerEXT not available\n");
            ctx.debug_messenger = VK_NULL_HANDLE;
        }
    }
    // createSurface
    {
        ctx.surface = VK_NULL_HANDLE;
        VERIFY(SDL_Vulkan_CreateSurface((SDL_Window*)ctx.window_p, ctx.instance, &ctx.surface), "%s\n", SDL_GetError());
    }
    // getPhysicalDevice
    {
        unsigned int device_count = 0;
        result = vkEnumeratePhysicalDevices(ctx.instance, &device_count, NULL);
        VERIFY(!(result != VK_SUCCESS || device_count == 0), "Failed to find GPUs with Vulkan support\n");

        VkPhysicalDevice* devices = alloc(NULL, sizeof(VkPhysicalDevice) * device_count);
        VERIFY(devices, "Failed to allocate memory for physical devices\n");
        VERIFY(vkEnumeratePhysicalDevices(ctx.instance, &device_count, devices)==VK_SUCCESS, "Failed to enumerate physical devices\n");

        // For simplicity, choose the first device
        ctx.physical_device = devices[0];
        free(devices);

        // Check device properties to see if Vulkan 1.3 is supported
        VkPhysicalDeviceProperties deviceProps;
        vkGetPhysicalDeviceProperties(ctx.physical_device, &deviceProps);
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
            vkEnumerateDeviceExtensionProperties(ctx.physical_device, NULL, &extensionCount, NULL);
            VkExtensionProperties* extensions = alloc(NULL, sizeof(VkExtensionProperties) * extensionCount);
            vkEnumerateDeviceExtensionProperties(ctx.physical_device, NULL, &extensionCount, extensions);

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
        ctx.queue_family_indices.graphics = UINT32_MAX;
        ctx.queue_family_indices.present = UINT32_MAX;
        ctx.queue_family_indices.compute = UINT32_MAX;
        ctx.queue_family_indices.transfer = UINT32_MAX;

        unsigned int queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(ctx.physical_device, &queue_family_count, NULL);
        VERIFY(queue_family_count != 0, "Failed to find any queue families\n");

        VkQueueFamilyProperties* queue_families = alloc(NULL, sizeof(VkQueueFamilyProperties) * queue_family_count);
        VERIFY(queue_families, "Failed to allocate memory for queue family properties\n");

        vkGetPhysicalDeviceQueueFamilyProperties(ctx.physical_device, &queue_family_count, queue_families);

        for (unsigned int i = 0; i < queue_family_count; i++) {
            // Check for graphics support
            if ((queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && ctx.queue_family_indices.graphics == UINT32_MAX) {
                ctx.queue_family_indices.graphics = i;
            }

            // Check for compute support (prefer dedicated compute queues)
            if ((queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT) && 
                !(queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && 
                ctx.queue_family_indices.compute == UINT32_MAX) {
                ctx.queue_family_indices.compute = i;
            }

            // Check for transfer support (prefer dedicated transfer queues)
            if ((queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT) && 
                !(queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && 
                ctx.queue_family_indices.transfer == UINT32_MAX) {
                ctx.queue_family_indices.transfer = i;
            }

            // Check for presentation support
            VkBool32 present_support = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(ctx.physical_device, i, ctx.surface, &present_support);
            if (present_support && ctx.queue_family_indices.present == UINT32_MAX) {
                ctx.queue_family_indices.present = i;
            }

            // Break early if all family_indices are found
            if (ctx.queue_family_indices.graphics != UINT32_MAX &&
                ctx.queue_family_indices.present != UINT32_MAX &&
                ctx.queue_family_indices.compute != UINT32_MAX &&
                ctx.queue_family_indices.transfer != UINT32_MAX) {
                break;
            }
        }

        free(queue_families);

        // Validate that essential queue family_indices are found
        VERIFY(!(ctx.queue_family_indices.graphics == UINT32_MAX || ctx.queue_family_indices.present == UINT32_MAX), "Failed to find required queue families\n");
    }
    // check for driver compatibility
    {
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(ctx.physical_device, &deviceFeatures);

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
        vkGetPhysicalDeviceProperties(ctx.physical_device, &deviceProps);
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

        ADD_UNIQUE_FAMILY(ctx.queue_family_indices.graphics);
        ADD_UNIQUE_FAMILY(ctx.queue_family_indices.present);
        ADD_UNIQUE_FAMILY(ctx.queue_family_indices.compute);
        ADD_UNIQUE_FAMILY(ctx.queue_family_indices.transfer);

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

        TRACK(result = vkCreateDevice(ctx.physical_device, &device_create_info, NULL, &ctx.device));
        VERIFY(result == VK_SUCCESS, "Failed to create logical ctx.device. Error code: %d\n", result);
        printf("Logical ctx.device created successfully.\n");
        free(queue_create_infos);
    }
    // initializeVmaAllocator
    {
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = ctx.physical_device;
        allocatorInfo.device = ctx.device;
        allocatorInfo.instance = ctx.instance;

        TRACK(VkResult result = vmaCreateAllocator(&allocatorInfo, &ctx.allocator));
        VERIFY(result == VK_SUCCESS, "Failed to create VMA allocator\n");
    }
    // getQueues
    {
        ctx.queues.graphics = VK_NULL_HANDLE;
        ctx.queues.present = VK_NULL_HANDLE;
        ctx.queues.compute = VK_NULL_HANDLE;
        ctx.queues.transfer = VK_NULL_HANDLE;

        vkGetDeviceQueue(ctx.device, ctx.queue_family_indices.graphics, 0, &ctx.queues.graphics);

        if (ctx.queue_family_indices.graphics != ctx.queue_family_indices.present) {
            vkGetDeviceQueue(ctx.device, ctx.queue_family_indices.present, 0, &ctx.queues.present);
        } else {
            ctx.queues.present = ctx.queues.graphics;
        }

        if (ctx.queue_family_indices.compute != ctx.queue_family_indices.graphics &&
            ctx.queue_family_indices.compute != ctx.queue_family_indices.present) {
            vkGetDeviceQueue(ctx.device, ctx.queue_family_indices.compute, 0, &ctx.queues.compute);
        } else {
            ctx.queues.compute = ctx.queues.graphics;
        }

        if (ctx.queue_family_indices.transfer != ctx.queue_family_indices.graphics && 
            ctx.queue_family_indices.transfer != ctx.queue_family_indices.present && 
            ctx.queue_family_indices.transfer != ctx.queue_family_indices.compute) {
            vkGetDeviceQueue(ctx.device, ctx.queue_family_indices.transfer, 0, &ctx.queues.transfer);
        } else {
            ctx.queues.transfer = ctx.queues.graphics;
        }
    }
    // createSwapChain
    {
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        TRACK(VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx.physical_device, ctx.surface, &surfaceCapabilities));
        VERIFY(result == VK_SUCCESS, "Failed to get ctx.surface capabilities\n");

        VkExtent2D extent = surfaceCapabilities.currentExtent.width != UINT32_MAX ?
                                  surfaceCapabilities.currentExtent :
                                  (VkExtent2D){width, height};

        unsigned int format_count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(ctx.physical_device, ctx.surface, &format_count, NULL);
        VERIFY(format_count > 0, "Failed to find ctx.surface formats\n");
        TRACK(VkSurfaceFormatKHR* formats = alloc(NULL, sizeof(VkSurfaceFormatKHR) * format_count));
        VERIFY(formats, "Failed to allocate memory for ctx.surface formats\n");
        TRACK(vkGetPhysicalDeviceSurfaceFormatsKHR(ctx.physical_device, ctx.surface, &format_count, formats));

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

        Gpi_QueueFamilyIndices i = ctx.queue_family_indices;
        VkSwapchainCreateInfoKHR swapchainCreateInfo = {
            .sType           = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface         = ctx.surface,
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

        TRACK(result = vkCreateSwapchainKHR(ctx.device, &swapchainCreateInfo, NULL, &ctx.swap_chain));
        VERIFY(result == VK_SUCCESS, "Failed to create swapchain\n");

        ctx.images_count = 0;
        TRACK(vkGetSwapchainImagesKHR(ctx.device, ctx.swap_chain, &ctx.images_count, NULL));
        VERIFY(ctx.images_count > 0, "there is 0 images in swapchain");
        if (surfaceCapabilities.maxImageCount > 0) {
            VERIFY(ctx.images_count <= surfaceCapabilities.maxImageCount, "there is more than expected images in swapchain. images_count = %ld. maxImageCount = %d", ctx.images_count, surfaceCapabilities.maxImageCount);
        } else {
            printf("Note: maxImageCount is 0, indicating no upper limit on image count.\n");
        }

        TRACK(ctx.p_images = alloc(ctx.p_images, sizeof(Gpi_Image) * ctx.images_count));
        VERIFY(ctx.p_images, "Failed to allocate memory for swapchain images\n");

        VkImage* p_tmp_images = (VkImage*)alloc(NULL, sizeof(VkImage) * ctx.images_count);
        VERIFY(p_tmp_images, "Failed to allocate memory for temporary image array\n");
        TRACK(vkGetSwapchainImagesKHR(ctx.device, ctx.swap_chain, &ctx.images_count, p_tmp_images));

        for (unsigned int i = 0; i < ctx.images_count; i++) {
            ctx.p_images[i].image = p_tmp_images[i];
            ctx.p_images[i].extent = extent;
            ctx.p_images[i].format = format;
            ctx.p_images[i].layout = VK_IMAGE_LAYOUT_UNDEFINED;
        }

        free(p_tmp_images);

        for (unsigned int i = 0; i < ctx.images_count; i++) {
            VkImageViewCreateInfo view_info = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = ctx.p_images[i].image,
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = ctx.p_images[i].format,
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

            TRACK(result = vkCreateImageView(ctx.device, &view_info, NULL, &ctx.p_images[i].view));
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

        TRACK(result = vkCreateDescriptorPool(ctx.device, &pool_info, NULL, &ctx.descriptor_pool));
        VERIFY(result == VK_SUCCESS, "Failed to create descriptor pool\n");
    }
    // createCommandPool
    {
        VkCommandPoolCreateInfo poolInfo = {
            .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .queueFamilyIndex = ctx.queue_family_indices.graphics,
            .flags            = 0
        };

        TRACK(result = vkCreateCommandPool(ctx.device, &poolInfo, NULL, &ctx.command_pool));
        VERIFY(result == VK_SUCCESS, "Failed to create command pool\n");
    }

    return ctx;
}
void                                gpi_Context_StartApplication(
    Gpi_Context* p_ctx,
    Gpi_Cmd* p_cmds,
    Gpi_Buffer indirect_buffer)
{

    Gpi_Semaphore image_available_semaphore = gpi_Semaphore_Create(p_ctx);
    Gpi_Semaphore render_finished_semaphore = gpi_Semaphore_Create(p_ctx);
    Gpi_Fence in_flight_fence = gpi_Fence_Create(p_ctx);

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
            if (tmp_i>=5)
                tmp_i = 0;
            else
                tmp_i++;
            Gpi_DrawIndirectCommand draw_cmd_data = {
                .vertex_count   = 4,
                .instance_count = 5, 
                .first_vertex   = 0,
                .first_instance = 0
            };
            gpi_Buffer_Update(p_ctx, indirect_buffer, 0, &draw_cmd_data, sizeof(draw_cmd_data));
        }

        TRACK(vkWaitForFences(p_ctx->device, 1, &in_flight_fence.fence, VK_TRUE, UINT64_MAX));
        TRACK(vkResetFences(p_ctx->device, 1, &in_flight_fence.fence));

        // Acquire the next image from the swap chain
        unsigned int image_index;
        TRACK(VkResult acquire_result = vkAcquireNextImageKHR(p_ctx->device, p_ctx->swap_chain, UINT64_MAX, image_available_semaphore.semaphore, VK_NULL_HANDLE, &image_index));
        VERIFY(!(acquire_result == VK_ERROR_OUT_OF_DATE_KHR || acquire_result == VK_SUBOPTIMAL_KHR), "Swapchain out of date or suboptimal. Consider recreating swapchain.\n");
        VERIFY(!(acquire_result != VK_SUCCESS && acquire_result != VK_SUBOPTIMAL_KHR), "Failed to acquire swapchain image: %d\n", acquire_result);

        //vk_Image_TransitionLayoutWithoutCommandBuffer(p_ctx, &p_ctx->p_images[image_index], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        // Submit the command buffer
        VkSubmitInfo submit_info = {
            .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount   = 1,
            .pWaitSemaphores      = (VkSemaphore[]){ image_available_semaphore.semaphore },
            .pWaitDstStageMask    = (VkPipelineStageFlags[]){ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT },
            .commandBufferCount   = 1,
            .pCommandBuffers      = &p_cmds[image_index].command_buffer,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores    = (VkSemaphore[]){ render_finished_semaphore.semaphore }
        };
        TRACK(VkResult submit_result = vkQueueSubmit(p_ctx->queues.graphics, 1, &submit_info, in_flight_fence.fence));
        VERIFY(submit_result == VK_SUCCESS, "Failed to submit draw command buffer: %d\n", submit_result);


        //vk_Image_TransitionLayoutWithoutCommandBuffer(p_ctx, &p_ctx->p_images[image_index], VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        // Present the image
        VkPresentInfoKHR present_info = {
            .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores    = (VkSemaphore[]){ render_finished_semaphore.semaphore },
            .swapchainCount     = 1,
            .pSwapchains        = (VkSwapchainKHR[]){ p_ctx->swap_chain },
            .pImageIndices      = &image_index  
        };

        TRACK(VkResult present_result = vkQueuePresentKHR(p_ctx->queues.present, &present_info));
        VERIFY(!(present_result == VK_ERROR_OUT_OF_DATE_KHR || present_result == VK_SUBOPTIMAL_KHR), "Swapchain out of date or suboptimal during present. Consider recreating swapchain.\n");
        VERIFY(present_result == VK_SUCCESS, "Failed to present swapchain image: %d\n", present_result);

        // Optionally wait for the present queue to be idle
        TRACK(vkQueueWaitIdle(p_ctx->queues.present));
        //usleep(100000);
    }
}
void                                gpi_Context_StopApplication(Gpi_Context* p_ctx);
void                                gpi_Context_Destroy(Gpi_Context* p_ctx) {

}

// buffer =================================================================================================================================
Gpi_Buffer                          gpi_Buffer_Create(
    Gpi_Context* p_ctx, 
    VkDeviceSize size, 
    VkBufferUsageFlags usage) 
{
    VERIFY(p_ctx, "given p_ctx context is NULL\n");

    Gpi_Buffer buffer = {0};

    VkBufferCreateInfo bufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    // Add necessary usage flags for transfer operations
    if (usage & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT || usage & VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT) {
        bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }
    if (usage & (VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT)) {
        bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }

    VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_AUTO; // Default to auto selection

    // Prioritize memory usage based on buffer type
    if (usage & (VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT)) {
        // For staging or transfer buffers, prefer CPU memory for better CPU-GPU transfer
        memoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
    } else if (usage & (VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)) {
        // Uniform and storage buffers might benefit from being in device memory for performance
        memoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    } else if (usage & (VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT)) {
        // These are typically GPU-only for optimal performance
        memoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    }

    VmaAllocationCreateInfo allocInfo = {
        .usage = memoryUsage,
        .flags = 0
    };

    // Ensure host access for transfer buffers used in staging operations
    if (usage & (VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT)) {
        allocInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    } else if (usage & (VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)) {
        allocInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    }

    TRACK(VkResult result = vmaCreateBuffer(p_ctx->allocator, &bufferInfo, &allocInfo, &buffer.buffer, &buffer.allocation, NULL));
    VERIFY(result == VK_SUCCESS, "Failed to create buffer with VMA!\n");

    buffer.size = size;
    buffer.usage = memoryUsage; // Store the usage for later operations

    return buffer;
}
void                                gpi_Buffer_Clear(
    Gpi_Context* p_ctx, 
    Gpi_Buffer buffer, 
    int clear_value) 
{
    VERIFY(p_ctx, "given p_ctx context is NULL\n");

    VkDeviceSize size = buffer.size;
    void* p_dst_data = NULL;
    VkResult result;

    // Decide if direct mapping is preferable or if staging is needed
    if (buffer.usage == VMA_MEMORY_USAGE_AUTO_PREFER_HOST) {
        // If the buffer is host-preferred, we can map directly
        result = vmaMapMemory(p_ctx->allocator, buffer.allocation, &p_dst_data);
        VERIFY(result == VK_SUCCESS && p_dst_data, "Failed to map buffer memory!\n");
        memset(p_dst_data, clear_value, size);
        vmaUnmapMemory(p_ctx->allocator, buffer.allocation);
    } else {
        // For device-preferred memory, use staging
        Gpi_Buffer staging_buffer = gpi_Buffer_Create(p_ctx, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

        result = vmaMapMemory(p_ctx->allocator, staging_buffer.allocation, &p_dst_data);
        VERIFY(result == VK_SUCCESS && p_dst_data, "Failed to map staging buffer memory!\n");
        memset(p_dst_data, clear_value, size);
        vmaUnmapMemory(p_ctx->allocator, staging_buffer.allocation);

        Gpi_Cmd cmd = gpi_Cmd_CreateAndBeginSingleTimeUsage(p_ctx);
        VkBufferCopy copyRegion = {
            .srcOffset = 0,
            .dstOffset = 0,
            .size = size,
        };

        vkCmdCopyBuffer(cmd.command_buffer, staging_buffer.buffer, buffer.buffer, 1, &copyRegion);
        gpi_Cmd_EndAndDestroySingleTimeUsage(p_ctx, &cmd);
        vmaDestroyBuffer(p_ctx->allocator, staging_buffer.buffer, staging_buffer.allocation);
    }
}
void                                gpi_Buffer_Update(
    Gpi_Context* p_ctx,
    Gpi_Buffer buffer,
    VkDeviceSize dst_offset,
    const void* p_src_data,
    VkDeviceSize size)
{
    if (size == 0)
        return;
    VERIFY(p_ctx, "NULL pointer");
    VERIFY(p_src_data, "NULL pointer");
    VERIFY(p_ctx, "given p_ctx context is NULL\n");
    VERIFY(p_src_data, "given p_src_data is NULL\n");
    VERIFY(buffer.size >= dst_offset + size, "Writing to buffer will go out of bounds\n");

    void* p_dst_data = NULL;
    VkResult result;

    if (buffer.usage == VMA_MEMORY_USAGE_AUTO_PREFER_HOST) {
        // Directly map if buffer is host-preferred
        result = vmaMapMemory(p_ctx->allocator, buffer.allocation, &p_dst_data);
        VERIFY(result == VK_SUCCESS && p_dst_data, "Failed to map buffer memory!\n");
        memcpy((uint8_t*)p_dst_data + dst_offset, p_src_data, size);
        vmaUnmapMemory(p_ctx->allocator, buffer.allocation);
    } else {
        // Use staging for device-preferred buffers
        Gpi_Buffer staging_buffer = gpi_Buffer_Create(p_ctx, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

        result = vmaMapMemory(p_ctx->allocator, staging_buffer.allocation, &p_dst_data);
        VERIFY(result == VK_SUCCESS && p_dst_data, "Failed to map staging buffer memory!\n");
        memcpy(p_dst_data, p_src_data, size);
        vmaUnmapMemory(p_ctx->allocator, staging_buffer.allocation);

        Gpi_Cmd cmd = gpi_Cmd_CreateAndBeginSingleTimeUsage(p_ctx);

        VkBufferCopy copyRegion = {
            .srcOffset = 0,
            .dstOffset = dst_offset,
            .size = size,
        };

        vkCmdCopyBuffer(cmd.command_buffer, staging_buffer.buffer, buffer.buffer, 1, &copyRegion);
        gpi_Cmd_EndAndDestroySingleTimeUsage(p_ctx, &cmd);
        vmaDestroyBuffer(p_ctx->allocator, staging_buffer.buffer, staging_buffer.allocation);
    }
}
void                                gpi_Buffer_CopyOtherBuffer(
    Gpi_Context* p_ctx, 
    Gpi_Buffer src_buffer, 
    Gpi_Buffer dst_buffer, 
    VkDeviceSize src_offset, 
    VkDeviceSize dst_offset, 
    VkDeviceSize size) 
{
    VERIFY(p_ctx, "given p_ctx context is NULL\n");
    
    VERIFY(src_buffer.size >= src_offset + size, "Source buffer copy operation would go out of bounds\n");
    VERIFY(dst_buffer.size >= dst_offset + size, "Destination buffer copy operation would go out of bounds\n");
    
    Gpi_Cmd cmd = gpi_Cmd_CreateAndBeginSingleTimeUsage(p_ctx);

    VkBufferCopy copyRegion = {
        .srcOffset = src_offset,
        .dstOffset = dst_offset,
        .size = size,
    };

    // Execute the copy command
    TRACK(vkCmdCopyBuffer(cmd.command_buffer, src_buffer.buffer, dst_buffer.buffer, 1, &copyRegion));

    // End and submit the command buffer for execution
    gpi_Cmd_EndAndDestroySingleTimeUsage(p_ctx, &cmd);
}
void                                gpi_Buffer_Destroy(
    Gpi_Context* p_ctx, 
    Gpi_Buffer* p_buffer) 
{
    VERIFY(p_buffer, "NULL pointer");
    VERIFY(p_ctx, "NULL pointer");

    if (p_buffer->allocation) {
        vmaDestroyBuffer(p_ctx->allocator, p_buffer->buffer, p_buffer->allocation);
    }
    memset(p_buffer, 0, sizeof(Gpi_Buffer));
}

// image ==================================================================================================================================
Gpi_Image                           gpi_Image_Create_ReadWrite(
    Gpi_Context* p_ctx, 
    VkExtent2D extent, 
    VkFormat format) 
{

    VERIFY(p_ctx, "NULL pointer");
    VERIFY(p_ctx->device!=VK_NULL_HANDLE, "VK_NULL_HANDLE");
    VERIFY(p_ctx->allocator!=VK_NULL_HANDLE, "VK_NULL_HANDLE");

    Gpi_Image image;
    image.layout = VK_IMAGE_LAYOUT_UNDEFINED;
    image.extent = extent;
    image.format = format; //VK_FORMAT_R8G8B8A8_SRGB

    VkImageCreateInfo image_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .extent.width = extent.width,
        .extent.height = extent.height,
        .extent.depth = 1,
        .mipLevels = 1,
        .arrayLayers = 1,
        .format = format,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    VmaAllocationCreateInfo alloc_info = {
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
    };  

    TRACK(VkResult result = vmaCreateImage(p_ctx->allocator, &image_info, &alloc_info, &image.image, &image.allocation, NULL));
    VERIFY(result == VK_SUCCESS, "failed to create\n ");
    VERIFY(image.image != VK_NULL_HANDLE, "failed to create");

    VkImageViewCreateInfo view_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image.image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format, //VK_FORMAT_R8G8B8A8_SRGB
        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.baseMipLevel = 0,
        .subresourceRange.levelCount = 1,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount = 1,
    };

    TRACK(result = vkCreateImageView(p_ctx->device, &view_info, NULL, &image.view));
    VERIFY(result == VK_SUCCESS, "failed to create\n ");
    VERIFY(image.view != VK_NULL_HANDLE, "failed to create");

    VkSamplerCreateInfo sampler_info = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = 16,
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .mipLodBias = 0.0f,
        .minLod = 0.0f,
        .maxLod = 0.0f,
    };

    TRACK(result = vkCreateSampler(p_ctx->device, &sampler_info, NULL, &image.sampler));
    VERIFY(result == VK_SUCCESS, "failed to create\n ");
    VERIFY(image.sampler != VK_NULL_HANDLE, "failed to create");

    return image;
}
Gpi_Image                           gpi_Image_Create_FromImageFile(
    Gpi_Context* p_ctx, 
    const char* filename, 
    VkFormat format, 
    VkImageLayout layout ) 
{

    VERIFY(p_ctx, "Vk pointer is NULL.");
    VERIFY(p_ctx->allocator, "VmaAllocator is NULL.");
    VERIFY(filename, "Filename is NULL.");

    char absolute_path[PATH_MAX];
    VERIFY(realpath(filename, absolute_path), "realpath");
    printf("Loading image from: %s\n", absolute_path);

    TRACK(FILE* file = fopen(filename, "rb"));
    VERIFY(file, "Cannot open file: %s\n", filename);
    TRACK(fclose(file));

    int width, height, channels;
    TRACK(unsigned char* p_data = stbi_load(filename, &width, &height, &channels, STBI_rgb_alpha));
    VERIFY(p_data, "Failed to load image file: %s\n", filename);

    size_t pixel_size = 4;
    VkRect2D rect = {
        .offset = {0, 0},
        .extent = {width, height}
    };

    TRACK(Gpi_Image image = gpi_Image_Create_ReadWrite(p_ctx, rect.extent, format));

    TRACK(gpi_Image_CopyData( p_ctx, &image, layout, p_data, rect, pixel_size ));
    TRACK(stbi_image_free(p_data));

    return image;
}
void                                gpi_Image_TransitionLayout(
    Gpi_Cmd* p_cmd, 
    Gpi_Image* p_image,
    VkImageLayout new_layout) 
{

    VERIFY(p_image, "NULL pointer");

    VkImageLayout old_layout = p_image->layout;
    if (old_layout == new_layout) {
        printf("WARNING: image layout is already in the prefered image layout\n");
        return;
    }

    VkImageAspectFlags aspect_mask = 0;
    if (p_image->format == VK_FORMAT_D32_SFLOAT ||
        p_image->format == VK_FORMAT_D24_UNORM_S8_UINT ||
        p_image->format == VK_FORMAT_D32_SFLOAT_S8_UINT) {
        aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;
        // Add stencil aspect if present
        if (p_image->format == VK_FORMAT_D24_UNORM_S8_UINT || p_image->format == VK_FORMAT_D32_SFLOAT_S8_UINT) {
            aspect_mask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    } else {
        aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    VkImageSubresourceRange subresource_range = {
        .aspectMask = aspect_mask,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };

    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = old_layout,
        .newLayout = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = p_image->image,
        .subresourceRange = subresource_range,
    };

    VkPipelineStageFlags src_stage;
    VkPipelineStageFlags dst_stage;

    // Define source access mask and pipeline stage based on old_layout
    switch (old_layout) {
        case VK_IMAGE_LAYOUT_UNDEFINED:
            barrier.srcAccessMask = 0;
            src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            break;
        case VK_IMAGE_LAYOUT_PREINITIALIZED:
            barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
            src_stage = VK_PIPELINE_STAGE_HOST_BIT;
            break;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            src_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            break;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            src_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            break;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            break;
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            break;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            src_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            break;
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            src_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            break;
        default:
            VERIFY(false, "unsupported old layout\n");
    }

    // Define destination access mask and pipeline stage based on new_layout
    switch (new_layout) {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            break;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            break;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dst_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            break;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            dst_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            break;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            break;
        case VK_IMAGE_LAYOUT_GENERAL:
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            break;
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            dst_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            break;
        case VK_IMAGE_LAYOUT_UNDEFINED:
            barrier.dstAccessMask = 0;
            dst_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            break;
        case VK_IMAGE_LAYOUT_PREINITIALIZED:
            barrier.dstAccessMask = VK_ACCESS_HOST_WRITE_BIT;
            dst_stage = VK_PIPELINE_STAGE_HOST_BIT;
            break;
        default:
            VERIFY(false, "unsupported old layout\n");
    }
    
    TRACK(vkCmdPipelineBarrier( p_cmd->command_buffer, src_stage, dst_stage, 0, 0, NULL, 0, NULL, 1, &barrier ));

    p_image->layout = new_layout;
}
void                                gpi_Image_CopyData(
    Gpi_Context* p_ctx, 
    Gpi_Image* p_image, 
    VkImageLayout final_layout, 
    const void* p_data, 
    const VkRect2D rect, 
    const size_t pixel_size ) 
{

    VERIFY(p_ctx, "NULL pointer");
    VERIFY(p_image, "NULL pointer");
    VERIFY(p_data, "NULL pointer");

    size_t image_size = rect.extent.width*rect.extent.height*pixel_size;
    TRACK(Gpi_Buffer staging_buffer = gpi_Buffer_Create(p_ctx, image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT));
    VERIFY(image_size, "image_size is 0");

    void* p_dst_data;
    TRACK(vmaMapMemory(p_ctx->allocator, staging_buffer.allocation, &p_dst_data));
    VERIFY(p_dst_data, "NULL pointer");
    TRACK(memcpy(p_dst_data, p_data, image_size));
    TRACK(vmaUnmapMemory(p_ctx->allocator, staging_buffer.allocation));

    VkBufferImageCopy region = {
        .bufferOffset = 0,
        .bufferRowLength = 0, 
        .bufferImageHeight = 0, 
        .imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .imageSubresource.mipLevel = 0,
        .imageSubresource.baseArrayLayer = 0,
        .imageSubresource.layerCount = 1,
        .imageOffset = {
            .x = rect.offset.x,
            .y = rect.offset.y,
            .z = 0
        },
        .imageExtent = {
            .width = rect.extent.width,
            .height = rect.extent.height,
            .depth = 1
        },
    };

    TRACK(Gpi_Cmd cmd = gpi_Cmd_CreateAndBeginSingleTimeUsage(p_ctx));
    TRACK(gpi_Image_TransitionLayout(&cmd, p_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL));
    TRACK(vkCmdCopyBufferToImage( cmd.command_buffer, staging_buffer.buffer, p_image->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region ));
    TRACK(gpi_Image_TransitionLayout(&cmd, p_image, final_layout));
    TRACK(gpi_Cmd_EndAndDestroySingleTimeUsage(p_ctx, &cmd));
    TRACK(vmaDestroyBuffer(p_ctx->allocator, staging_buffer.buffer, staging_buffer.allocation));
}
void 								gpi_Image_Destroy(
	Gpi_Context* p_ctx, 
	Gpi_Image* p_image) 
{
    VERIFY(p_image, "NULL pointer");
    VERIFY(p_ctx, "NULL pointer");

    if (p_image->sampler) {
        vkDestroySampler(p_ctx->device, p_image->sampler, NULL);
    }
    if (p_image->view) {
        vkDestroyImageView(p_ctx->device, p_image->view, NULL);
    }
    if (p_image->allocation) {
        vmaDestroyImage(p_ctx->allocator, p_image->image, p_image->allocation);
    }
    memset(p_image, 0, sizeof(Gpi_Image));
}

// shader =================================================================================================================================
size_t 								_gpi_Shader_ReadFile(
	const char* filename, 
	char** dst_buffer) 
{
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("Failed to open shader source file '%s'\n", filename);
        exit(-1);
    }

    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    *dst_buffer = (char*)alloc(NULL, file_size + 1);
    if (!*dst_buffer) {
        printf("Failed to allocate memory for shader source '%s'\n", filename);
        fclose(file);
        exit(-1);
    }

    size_t readSize = fread(*dst_buffer, 1, file_size, file);
    (*dst_buffer)[file_size] = '\0'; 

    if (readSize != file_size) {
        printf("Failed to read shader source '%s'\n", filename);
        free(*dst_buffer);
        fclose(file);
        exit(-1);
    }

    fclose(file);
    return (unsigned int)file_size;
}
uint32_t 							_gpi_Shader_FormatSize(
	VkFormat format) 
{
  uint32_t result = 0;
  switch (format) {
    case VK_FORMAT_UNDEFINED:
      result = 0;
      break;
    case VK_FORMAT_R4G4_UNORM_PACK8:
      result = 1;
      break;
    case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
      result = 2;
      break;
    case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
      result = 2;
      break;
    case VK_FORMAT_R5G6B5_UNORM_PACK16:
      result = 2;
      break;
    case VK_FORMAT_B5G6R5_UNORM_PACK16:
      result = 2;
      break;
    case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
      result = 2;
      break;
    case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
      result = 2;
      break;
    case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
      result = 2;
      break;
    case VK_FORMAT_R8_UNORM:
      result = 1;
      break;
    case VK_FORMAT_R8_SNORM:
      result = 1;
      break;
    case VK_FORMAT_R8_USCALED:
      result = 1;
      break;
    case VK_FORMAT_R8_SSCALED:
      result = 1;
      break;
    case VK_FORMAT_R8_UINT:
      result = 1;
      break;
    case VK_FORMAT_R8_SINT:
      result = 1;
      break;
    case VK_FORMAT_R8_SRGB:
      result = 1;
      break;
    case VK_FORMAT_R8G8_UNORM:
      result = 2;
      break;
    case VK_FORMAT_R8G8_SNORM:
      result = 2;
      break;
    case VK_FORMAT_R8G8_USCALED:
      result = 2;
      break;
    case VK_FORMAT_R8G8_SSCALED:
      result = 2;
      break;
    case VK_FORMAT_R8G8_UINT:
      result = 2;
      break;
    case VK_FORMAT_R8G8_SINT:
      result = 2;
      break;
    case VK_FORMAT_R8G8_SRGB:
      result = 2;
      break;
    case VK_FORMAT_R8G8B8_UNORM:
      result = 3;
      break;
    case VK_FORMAT_R8G8B8_SNORM:
      result = 3;
      break;
    case VK_FORMAT_R8G8B8_USCALED:
      result = 3;
      break;
    case VK_FORMAT_R8G8B8_SSCALED:
      result = 3;
      break;
    case VK_FORMAT_R8G8B8_UINT:
      result = 3;
      break;
    case VK_FORMAT_R8G8B8_SINT:
      result = 3;
      break;
    case VK_FORMAT_R8G8B8_SRGB:
      result = 3;
      break;
    case VK_FORMAT_B8G8R8_UNORM:
      result = 3;
      break;
    case VK_FORMAT_B8G8R8_SNORM:
      result = 3;
      break;
    case VK_FORMAT_B8G8R8_USCALED:
      result = 3;
      break;
    case VK_FORMAT_B8G8R8_SSCALED:
      result = 3;
      break;
    case VK_FORMAT_B8G8R8_UINT:
      result = 3;
      break;
    case VK_FORMAT_B8G8R8_SINT:
      result = 3;
      break;
    case VK_FORMAT_B8G8R8_SRGB:
      result = 3;
      break;
    case VK_FORMAT_R8G8B8A8_UNORM:
      result = 4;
      break;
    case VK_FORMAT_R8G8B8A8_SNORM:
      result = 4;
      break;
    case VK_FORMAT_R8G8B8A8_USCALED:
      result = 4;
      break;
    case VK_FORMAT_R8G8B8A8_SSCALED:
      result = 4;
      break;
    case VK_FORMAT_R8G8B8A8_UINT:
      result = 4;
      break;
    case VK_FORMAT_R8G8B8A8_SINT:
      result = 4;
      break;
    case VK_FORMAT_R8G8B8A8_SRGB:
      result = 4;
      break;
    case VK_FORMAT_B8G8R8A8_UNORM:
      result = 4;
      break;
    case VK_FORMAT_B8G8R8A8_SNORM:
      result = 4;
      break;
    case VK_FORMAT_B8G8R8A8_USCALED:
      result = 4;
      break;
    case VK_FORMAT_B8G8R8A8_SSCALED:
      result = 4;
      break;
    case VK_FORMAT_B8G8R8A8_UINT:
      result = 4;
      break;
    case VK_FORMAT_B8G8R8A8_SINT:
      result = 4;
      break;
    case VK_FORMAT_B8G8R8A8_SRGB:
      result = 4;
      break;
    case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A8B8G8R8_UINT_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A8B8G8R8_SINT_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A2R10G10B10_UINT_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A2R10G10B10_SINT_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A2B10G10R10_UINT_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A2B10G10R10_SINT_PACK32:
      result = 4;
      break;
    case VK_FORMAT_R16_UNORM:
      result = 2;
      break;
    case VK_FORMAT_R16_SNORM:
      result = 2;
      break;
    case VK_FORMAT_R16_USCALED:
      result = 2;
      break;
    case VK_FORMAT_R16_SSCALED:
      result = 2;
      break;
    case VK_FORMAT_R16_UINT:
      result = 2;
      break;
    case VK_FORMAT_R16_SINT:
      result = 2;
      break;
    case VK_FORMAT_R16_SFLOAT:
      result = 2;
      break;
    case VK_FORMAT_R16G16_UNORM:
      result = 4;
      break;
    case VK_FORMAT_R16G16_SNORM:
      result = 4;
      break;
    case VK_FORMAT_R16G16_USCALED:
      result = 4;
      break;
    case VK_FORMAT_R16G16_SSCALED:
      result = 4;
      break;
    case VK_FORMAT_R16G16_UINT:
      result = 4;
      break;
    case VK_FORMAT_R16G16_SINT:
      result = 4;
      break;
    case VK_FORMAT_R16G16_SFLOAT:
      result = 4;
      break;
    case VK_FORMAT_R16G16B16_UNORM:
      result = 6;
      break;
    case VK_FORMAT_R16G16B16_SNORM:
      result = 6;
      break;
    case VK_FORMAT_R16G16B16_USCALED:
      result = 6;
      break;
    case VK_FORMAT_R16G16B16_SSCALED:
      result = 6;
      break;
    case VK_FORMAT_R16G16B16_UINT:
      result = 6;
      break;
    case VK_FORMAT_R16G16B16_SINT:
      result = 6;
      break;
    case VK_FORMAT_R16G16B16_SFLOAT:
      result = 6;
      break;
    case VK_FORMAT_R16G16B16A16_UNORM:
      result = 8;
      break;
    case VK_FORMAT_R16G16B16A16_SNORM:
      result = 8;
      break;
    case VK_FORMAT_R16G16B16A16_USCALED:
      result = 8;
      break;
    case VK_FORMAT_R16G16B16A16_SSCALED:
      result = 8;
      break;
    case VK_FORMAT_R16G16B16A16_UINT:
      result = 8;
      break;
    case VK_FORMAT_R16G16B16A16_SINT:
      result = 8;
      break;
    case VK_FORMAT_R16G16B16A16_SFLOAT:
      result = 8;
      break;
    case VK_FORMAT_R32_UINT:
      result = 4;
      break;
    case VK_FORMAT_R32_SINT:
      result = 4;
      break;
    case VK_FORMAT_R32_SFLOAT:
      result = 4;
      break;
    case VK_FORMAT_R32G32_UINT:
      result = 8;
      break;
    case VK_FORMAT_R32G32_SINT:
      result = 8;
      break;
    case VK_FORMAT_R32G32_SFLOAT:
      result = 8;
      break;
    case VK_FORMAT_R32G32B32_UINT:
      result = 12;
      break;
    case VK_FORMAT_R32G32B32_SINT:
      result = 12;
      break;
    case VK_FORMAT_R32G32B32_SFLOAT:
      result = 12;
      break;
    case VK_FORMAT_R32G32B32A32_UINT:
      result = 16;
      break;
    case VK_FORMAT_R32G32B32A32_SINT:
      result = 16;
      break;
    case VK_FORMAT_R32G32B32A32_SFLOAT:
      result = 16;
      break;
    case VK_FORMAT_R64_UINT:
      result = 8;
      break;
    case VK_FORMAT_R64_SINT:
      result = 8;
      break;
    case VK_FORMAT_R64_SFLOAT:
      result = 8;
      break;
    case VK_FORMAT_R64G64_UINT:
      result = 16;
      break;
    case VK_FORMAT_R64G64_SINT:
      result = 16;
      break;
    case VK_FORMAT_R64G64_SFLOAT:
      result = 16;
      break;
    case VK_FORMAT_R64G64B64_UINT:
      result = 24;
      break;
    case VK_FORMAT_R64G64B64_SINT:
      result = 24;
      break;
    case VK_FORMAT_R64G64B64_SFLOAT:
      result = 24;
      break;
    case VK_FORMAT_R64G64B64A64_UINT:
      result = 32;
      break;
    case VK_FORMAT_R64G64B64A64_SINT:
      result = 32;
      break;
    case VK_FORMAT_R64G64B64A64_SFLOAT:
      result = 32;
      break;
    case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
      result = 4;
      break;
    case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
      result = 4;
      break;

    default:
      break;
  }
  return result;
}
void 								_gpi_Shader_PrintAttributeDescriptions(
	VkVertexInputAttributeDescription* p_attribs, 
	size_t attribs_count) 
{
    for (unsigned int i = 0; i < attribs_count; ++i) {
        printf("attrib %d\n", i);
        printf("\t%d\n", p_attribs[i].location);
        printf("\t%d\n", p_attribs[i].binding);
        printf("\t%d\n", p_attribs[i].format);
        printf("\t%d\n", p_attribs[i].offset);
    }
}
Gpi_Shader 							gpi_Shader_CreateFromGlslFile(
	Gpi_Context* p_ctx, 
	const char* glsl_file_path, 
	shaderc_shader_kind shader_kind) 
{
	VERIFY(p_ctx, "NULL pointer");
	VERIFY(glsl_file_path, "NULL pointer");
	VERIFY(	shader_kind == shaderc_vertex_shader ||
  			shader_kind == shaderc_fragment_shader || 
  			shader_kind == shaderc_compute_shader || 
  			shader_kind == shaderc_geometry_shader ||
  			shader_kind == shaderc_tess_control_shader ||
  			shader_kind == shaderc_tess_evaluation_shader, 
  			"shader kind is not supported");

	Gpi_Shader shader = {0};

	// shaderc_shader_kind
	{
		shader.shader_kind = shader_kind;
	}

	// spv code compilation
	{
		char* p_glsl_code = NULL;
	    TRACK(size_t glsl_code_size = _gpi_Shader_ReadFile(glsl_file_path, &p_glsl_code));
	    VERIFY(p_glsl_code, "NULL pointer");
	   	TRACK(shaderc_compilation_result_t result = shaderc_compile_into_spv(
	    	p_ctx->shaderc_compiler, p_glsl_code, glsl_code_size, shader_kind, glsl_file_path, "main", p_ctx->shaderc_options));
	    VERIFY(shaderc_result_get_compilation_status(result) == shaderc_compilation_status_success, 
	    	"Shader compilation error in '%s':\n%s\n", glsl_file_path, shaderc_result_get_error_message(result));
		TRACK(shader.spv_code_size = shaderc_result_get_length(result));
	    TRACK(shader.p_spv_code = alloc(NULL, shader.spv_code_size));
	    VERIFY(shader.p_spv_code, "Failed to allocate memory for SPIR-V code");
	    memcpy(shader.p_spv_code, shaderc_result_get_bytes(result), shader.spv_code_size);
	    TRACK(shaderc_result_release(result));
	}

    // SpvReflectShaderModule
    {
		TRACK(SpvReflectResult reflectResult = spvReflectCreateShaderModule(shader.spv_code_size, shader.p_spv_code, &shader.reflect_shader_module));
	    VERIFY(reflectResult == SPV_REFLECT_RESULT_SUCCESS, "Failed to create SPIRV-Reflect shader module\n");
	    if (shader_kind == shaderc_vertex_shader) {
	    	VERIFY(shader.reflect_shader_module.shader_stage  == SPV_REFLECT_SHADER_STAGE_VERTEX_BIT, "generated reflect shader and shaderc kind is not the same");
	    }
	    else if (shader_kind == shaderc_fragment_shader) {
	    	VERIFY(shader.reflect_shader_module.shader_stage  == SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT, "generated reflect shader and shaderc kind is not the same");
	    }
	    else if (shader_kind == shaderc_compute_shader) {
	    	VERIFY(shader.reflect_shader_module.shader_stage  == SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT, "generated reflect shader and shaderc kind is not the same");
	    }
	    else if (shader_kind == shaderc_geometry_shader) {
	    	VERIFY(shader.reflect_shader_module.shader_stage  == SPV_REFLECT_SHADER_STAGE_GEOMETRY_BIT, "generated reflect shader and shaderc kind is not the same");
	    }
	    else if (shader_kind == shaderc_tess_control_shader) {
	    	VERIFY(shader.reflect_shader_module.shader_stage  == SPV_REFLECT_SHADER_STAGE_TESSELLATION_CONTROL_BIT, "generated reflect shader and shaderc kind is not the same");
	    }
	    else if (shader_kind == shaderc_tess_evaluation_shader) {
	    	VERIFY(shader.reflect_shader_module.shader_stage  == SPV_REFLECT_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, "generated reflect shader and shaderc kind is not the same");
	    }	
	    else {
	    	VERIFY(false, "shader kind is not supported. AND THIS SHOULD HAVE BEEN CHECKED EARLIER");
	    }
	}

    // VkShaderModule
    {
    	VkShaderModuleCreateInfo create_info = {
	        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
	        .codeSize = shader.spv_code_size,
	        .pCode = shader.p_spv_code
	    };
	    TRACK(VkResult result = vkCreateShaderModule(p_ctx->device, &create_info, NULL, &shader.shader_module));
	    VERIFY(result == VK_SUCCESS, "Failed to create Vulkan shader module\n");
    }

    return shader;
}
VkVertexInputAttributeDescription* 	gpi_Shader_Create_VertexInputAttribDesc( 
	const Gpi_Shader shader,
	unsigned int* p_attribute_count, 
	unsigned int* p_binding_stride) 
{
	VERIFY(p_attribute_count, " ");
	VERIFY(p_binding_stride, " ");
    VERIFY(shader.reflect_shader_module.shader_stage == SPV_REFLECT_SHADER_STAGE_VERTEX_BIT, "Provided shader is not a vertex shader\n");

    // Enumerate input variables
    uint32_t input_var_count = 0;
    TRACK(SpvReflectResult result = spvReflectEnumerateInputVariables(&shader.reflect_shader_module, &input_var_count, NULL));
    VERIFY(result == SPV_REFLECT_RESULT_SUCCESS, "Failed to enumerate input variables\n");

    TRACK(SpvReflectInterfaceVariable** input_vars = alloc(NULL, input_var_count * sizeof(SpvReflectInterfaceVariable*)));
    VERIFY(input_vars, "Failed to allocate memory for input variables\n");

    TRACK(result = spvReflectEnumerateInputVariables(&shader.reflect_shader_module, &input_var_count, input_vars));
    VERIFY(result == SPV_REFLECT_RESULT_SUCCESS, "Failed to get input variables\n");

    // Create an array to hold VkVertexInputAttributeDescription
    TRACK(VkVertexInputAttributeDescription* attribute_descriptions = alloc(NULL, input_var_count * sizeof(VkVertexInputAttributeDescription)));
    VERIFY(attribute_descriptions, "Failed to allocate memory for vertex input attribute descriptions\n");

    uint32_t attribute_index = 0;
    for (uint32_t i = 0; i < input_var_count; ++i) {
        SpvReflectInterfaceVariable* refl_var = input_vars[i];

        // Ignore built-in variables
        if (refl_var->decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN) {
            continue;
        }

        VkVertexInputAttributeDescription attr_desc = {};
        attr_desc.location = refl_var->location;
        attr_desc.binding = 0;
        attr_desc.format = (VkFormat)refl_var->format;
        attr_desc.offset = 0; // WILL CALCULATE OFFSET LATER
        attribute_descriptions[attribute_index++] = attr_desc;
    }

    // Update the attribute count
    *p_attribute_count = attribute_index;

    // Sort attributes by location
    for (uint32_t i = 0; i < attribute_index - 1; ++i) {
        for (uint32_t j = 0; j < attribute_index - i - 1; ++j) {
            if (attribute_descriptions[j].location > attribute_descriptions[j + 1].location) {
                VkVertexInputAttributeDescription temp = attribute_descriptions[j];
                attribute_descriptions[j] = attribute_descriptions[j + 1];
                attribute_descriptions[j + 1] = temp;
            }
        }
    }

    // Compute offsets and binding stride
    uint32_t offset = 0;
    for (uint32_t i = 0; i < attribute_index; ++i) {
        VkVertexInputAttributeDescription* attr_desc = &attribute_descriptions[i];
        TRACK(uint32_t format_size = _gpi_Shader_FormatSize(attr_desc->format));

        if (format_size == 0) {
            printf("Unsupported format for input variable at location %u\n", attr_desc->location);
            continue;
        }

        // Align the offset if necessary (e.g., 4-byte alignment)
        uint32_t alignment = 4;
        offset = (offset + (alignment - 1)) & ~(alignment - 1);
        attr_desc->offset = offset;
        offset += format_size;
    }

    *p_binding_stride = offset;
    free(input_vars);
    _gpi_Shader_PrintAttributeDescriptions(attribute_descriptions, *p_attribute_count);

    return attribute_descriptions;  
}
void 								gpi_Shader_Destroy(
	Gpi_Context* p_ctx, 
	Gpi_Shader* p_shader) 
{
    VERIFY(p_ctx, "NULL pointer");
    VERIFY(p_shader, "NULL pointer");

    if (p_shader->shader_module) {
        vkDestroyShaderModule(p_ctx->device, p_shader->shader_module, NULL);
    }
    if (p_shader->p_spv_code) {
        free(p_shader->p_spv_code);
    }
    spvReflectDestroyShaderModule(&p_shader->reflect_shader_module);
    memset(p_shader, 0, sizeof(Gpi_Shader));
    p_shader = NULL;
}

// pipeline ===============================================================================================================================
Gpi_Pipeline_Graphics 				gpi_Pipeline_Graphics_Create(
    Gpi_Context* p_ctx, 
    Gpi_Shader* p_shaders, 
    unsigned int shaders_count, 
    VkFormat format) 
{
	VERIFY(p_ctx, "NULL pointer");
	VERIFY(p_shaders, "NULL pointer");
	VERIFY(shaders_count > 0, "need at least one shader");

	Gpi_Pipeline_Graphics pipeline = {0};
	pipeline.p_ctx = p_ctx;
	printf("%p\n", pipeline.p_ctx);
	pipeline.shaders_count = shaders_count;
	
	// copying shaders
	{
		pipeline.shaders_count = shaders_count;
		TRACK(pipeline.p_shaders = alloc(pipeline.p_shaders, shaders_count * sizeof(Gpi_Shader)));
		VERIFY(pipeline.p_shaders, " ");

		for (unsigned int i = 0; i < shaders_count; ++i) {
			VERIFY(p_shaders[i].p_spv_code, "shader %d", i);
			VERIFY(p_shaders[i].spv_code_size > 0, "shader %d", i);
			// VERIFY(p_shaders[i].shader_kind != VK_NULL_HANDLE > 0, "");
			// VERIFY(p_shaders[i].reflect_shader_module != VK_NULL_HANDLE > 0, "");
			VERIFY(p_shaders[i].shader_module != VK_NULL_HANDLE, "shader %d", i);

		    // Ensure no shader of same kind
		    for (unsigned int j = 0; j < i; ++j) {
		        VERIFY(p_shaders[i].shader_kind != p_shaders[j].shader_kind, "you cannot provide multiple of the same shader_kind. shader %d and %d", j, i);
		    }

		    pipeline.p_shaders[i].spv_code_size = p_shaders[i].spv_code_size;
		    TRACK(pipeline.p_shaders[i].p_spv_code = alloc(NULL, pipeline.p_shaders[i].spv_code_size));
		    VERIFY(pipeline.p_shaders[i].spv_code_size, " ");
		    TRACK(memcpy(pipeline.p_shaders[i].p_spv_code, p_shaders[i].p_spv_code, pipeline.p_shaders[i].spv_code_size));
		    pipeline.p_shaders[i].reflect_shader_module = p_shaders[i].reflect_shader_module;
		    pipeline.p_shaders[i].shader_module = p_shaders[i].shader_module;
		    pipeline.p_shaders[i].shader_kind = p_shaders[i].shader_kind;
		}
	}

    // Gather all reflect shader modules
    TRACK(SpvReflectShaderModule* p_shader_modules = alloc(NULL, pipeline.shaders_count * sizeof(SpvReflectShaderModule)));
    for (unsigned int i = 0; i < pipeline.shaders_count; ++i) {
        p_shader_modules[i] = pipeline.p_shaders[i].reflect_shader_module;
    }

    // Create descriptor set layouts from reflection
    TRACK(pipeline.p_desc_sets_layout_create_info = _gpi_DescriptorSetLayoutCreateInfo_Create(pipeline.p_ctx, p_shader_modules, pipeline.shaders_count, (size_t*)&pipeline.desc_sets_count));

    // Create the descriptor set layouts
    TRACK(pipeline.p_desc_sets_layout = _gpi_DescriptorSetLayout_Create(pipeline.p_ctx, pipeline.p_desc_sets_layout_create_info, pipeline.desc_sets_count));

    TRACK(free(p_shader_modules));

    // Create pipeline layout
    VkPipelineLayoutCreateInfo pipeline_layout_info = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount         = pipeline.desc_sets_count,
        .pSetLayouts            = pipeline.p_desc_sets_layout,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges    = NULL
    };

    TRACK(VkResult result = vkCreatePipelineLayout(pipeline.p_ctx->device, &pipeline_layout_info, NULL, &pipeline.pipeline_layout));
    VERIFY(result == VK_SUCCESS, "Failed to create pipeline layout");

    // Find vertex shader index
    unsigned int vertex_index = 0;
    for (; vertex_index < pipeline.shaders_count; ++vertex_index) {
        if (pipeline.p_shaders[vertex_index].shader_kind == shaderc_vertex_shader) {
            break;
        }
    }
    VERIFY(vertex_index < pipeline.shaders_count, "Could not find vertex shader");
    printf("vertex_index %d\n", vertex_index);

    unsigned int vertex_attrib_count;
    unsigned int vertex_binding_stride;
    TRACK(VkVertexInputAttributeDescription* vertex_input_attrib_desc = gpi_Shader_Create_VertexInputAttribDesc(
        pipeline.p_shaders[vertex_index], 
        &vertex_attrib_count,
        &vertex_binding_stride
    ));

    // Set up shader stages
    const unsigned int stage_count = (unsigned int)pipeline.shaders_count;
    TRACK(VkPipelineShaderStageCreateInfo* p_stages = alloc(NULL, stage_count * sizeof(VkPipelineShaderStageCreateInfo)));
    VERIFY(p_stages, "Failed to allocate memory for pipeline stages");
    memset(p_stages, 0, stage_count * sizeof(VkPipelineShaderStageCreateInfo));

    for (unsigned int i = 0; i < stage_count; ++i) {
        p_stages[i].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        p_stages[i].pName  = "main";
        p_stages[i].module = pipeline.p_shaders[i].shader_module;

        if (pipeline.p_shaders[i].shader_kind == shaderc_fragment_shader) {
            p_stages[i].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        } else if (pipeline.p_shaders[i].shader_kind == shaderc_vertex_shader) {
            p_stages[i].stage = VK_SHADER_STAGE_VERTEX_BIT;
        } else if (pipeline.p_shaders[i].shader_kind == shaderc_compute_shader) {
            p_stages[i].stage = VK_SHADER_STAGE_COMPUTE_BIT;
        } else if (pipeline.p_shaders[i].shader_kind == shaderc_geometry_shader) {
            p_stages[i].stage = VK_SHADER_STAGE_GEOMETRY_BIT;
        } else if (pipeline.p_shaders[i].shader_kind == shaderc_tess_control_shader) {
            p_stages[i].stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        } else if (pipeline.p_shaders[i].shader_kind == shaderc_tess_evaluation_shader) {
            p_stages[i].stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        } else {
            VERIFY(false, "Unsupported shader kind encountered");
        }
    }

    VkPipelineRenderingCreateInfo rendering_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &format,
    };

    // Match the pipeline configuration from Pipeline_Graphics_Create
    VkViewport viewport = {
        .x        = 0.0f,
        .y        = 0.0f,
        .width    = (float)pipeline.p_ctx->p_images[0].extent.width,
        .height   = (float)pipeline.p_ctx->p_images[0].extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = pipeline.p_ctx->p_images[0].extent
    };

    VkGraphicsPipelineCreateInfo pipeline_info = {
        .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext               = &rendering_info,
        .stageCount          = stage_count,
        .pStages             = p_stages,
        .pVertexInputState   = &(VkPipelineVertexInputStateCreateInfo) {
            .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount   = 1,
            .pVertexBindingDescriptions      = (VkVertexInputBindingDescription[1]) {{
                .binding   = 0,
                .stride    = vertex_binding_stride,
                .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE
            }},
            .vertexAttributeDescriptionCount = vertex_attrib_count,
            .pVertexAttributeDescriptions    = vertex_input_attrib_desc,
        },
        .pInputAssemblyState = &(VkPipelineInputAssemblyStateCreateInfo) {
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
            .primitiveRestartEnable = VK_FALSE
        },
        .pViewportState      = &(VkPipelineViewportStateCreateInfo) {
            .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .pViewports    = &viewport,
            .scissorCount  = 1,
            .pScissors     = &scissor
        },
        .pRasterizationState = &(VkPipelineRasterizationStateCreateInfo) {
            .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .depthClampEnable        = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode             = VK_POLYGON_MODE_FILL,
            .lineWidth               = 1.0f,
            .cullMode                = VK_CULL_MODE_BACK_BIT,
            .frontFace               = VK_FRONT_FACE_CLOCKWISE,
            .depthBiasEnable         = VK_FALSE
        },
        .pMultisampleState   = &(VkPipelineMultisampleStateCreateInfo) {
            .sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .sampleShadingEnable  = VK_FALSE,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
        },
        .pDepthStencilState  = &(VkPipelineDepthStencilStateCreateInfo) {
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .depthTestEnable        = VK_FALSE,
            .depthWriteEnable       = VK_FALSE,
            .depthCompareOp         = VK_COMPARE_OP_LESS,
            .depthBoundsTestEnable  = VK_FALSE,
            .stencilTestEnable      = VK_FALSE
        },
        .pColorBlendState    = &(VkPipelineColorBlendStateCreateInfo) {
            .sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOpEnable     = VK_FALSE,
            .logicOp           = VK_LOGIC_OP_COPY,
            .attachmentCount   = 1,
            .pAttachments      = &(VkPipelineColorBlendAttachmentState) {
                .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | 
                                       VK_COLOR_COMPONENT_G_BIT | 
                                       VK_COLOR_COMPONENT_B_BIT | 
                                       VK_COLOR_COMPONENT_A_BIT,
                .blendEnable         = VK_TRUE,
                .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
                .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                .colorBlendOp        = VK_BLEND_OP_ADD,
                .srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
                .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                .alphaBlendOp        = VK_BLEND_OP_ADD
            },
            .blendConstants[0] = 0.0f,
            .blendConstants[1] = 0.0f,
            .blendConstants[2] = 0.0f,
            .blendConstants[3] = 0.0f
        },
        .pDynamicState       = NULL, // Matching the old method: no dynamic viewport/scissor
        .layout              = pipeline.pipeline_layout,
        .renderPass          = VK_NULL_HANDLE,
        .subpass             = 0,
        .basePipelineHandle  = VK_NULL_HANDLE,
        .basePipelineIndex   = -1
    };


    TRACK(result = vkCreateGraphicsPipelines(pipeline.p_ctx->device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &pipeline.graphics_pipeline));
    VERIFY(result == VK_SUCCESS, "Failed to create graphics pipeline");

    free(p_stages);
    free(vertex_input_attrib_desc);

    return pipeline;
}
void 								gpi_Pipeline_Graphics_Destroy(
    Gpi_Context* p_ctx, 
    Gpi_Pipeline_Graphics* p_pipeline) 
{
    VERIFY(p_ctx, "NULL pointer");
    VERIFY(p_pipeline, "NULL pointer");

    if (p_pipeline->graphics_pipeline) {
        vkDestroyPipeline(p_ctx->device, p_pipeline->graphics_pipeline, NULL);
    }
    if (p_pipeline->pipeline_layout) {
        vkDestroyPipelineLayout(p_ctx->device, p_pipeline->pipeline_layout, NULL);
    }
    if (p_pipeline->p_desc_sets_layout) {
        for (unsigned int i = 0; i < p_pipeline->desc_sets_count; ++i) {
            if (p_pipeline->p_desc_sets_layout[i]) {
                vkDestroyDescriptorSetLayout(p_ctx->device, p_pipeline->p_desc_sets_layout[i], NULL);
            }
        }
        free(p_pipeline->p_desc_sets_layout);
    }
    if (p_pipeline->p_desc_sets_layout_create_info) {
        free(p_pipeline->p_desc_sets_layout_create_info);
    }
    if (p_pipeline->p_shaders) {
        for (unsigned int i = 0; i < p_pipeline->shaders_count; ++i) {
            gpi_Shader_Destroy(p_ctx, &p_pipeline->p_shaders[i]);
        }
        free(p_pipeline->p_shaders);
    }
    memset(p_pipeline, 0, sizeof(Gpi_Pipeline_Graphics));
}

// descriptor set =========================================================================================================================
void                                _gpi_DescriptorSetLayoutCreateInfo_Print(
    const VkDescriptorSetLayoutCreateInfo* p_create_info, 
    const size_t create_info_count) 
{
    VERIFY(p_create_info, "NULL pointer");
    for (unsigned int i = 0; i < create_info_count; ++i) {
        printf("set_number %u\n", i);
        printf("binding_count %u\n", p_create_info[i].bindingCount);
        for (unsigned int j = 0; j < p_create_info[i].bindingCount; j++) {
            printf("\tbinding %u\n", p_create_info[i].pBindings[j].binding);
            printf("\tdescriptorType %u\n", p_create_info[i].pBindings[j].descriptorType);
            printf("\tdescriptorCount %u\n", p_create_info[i].pBindings[j].descriptorCount);
            printf("\tstageFlags %u\n", p_create_info[i].pBindings[j].stageFlags);
            printf("\tsampler %p\n", (void*)p_create_info[i].pBindings[j].pImmutableSamplers);
        }
    }
}
VkDescriptorSetLayoutCreateInfo*    _gpi_DescriptorSetLayoutCreateInfo_Create(
    Gpi_Context* p_ctx, 
    const SpvReflectShaderModule* p_shader_modules, 
    unsigned int shader_modules_count, 
    size_t* p_create_info_count) 
{
    VERIFY(p_ctx, "NULL pointer");
    VERIFY(p_shader_modules, "NULL pointer");
    VERIFY(p_create_info_count, "NULL pointer");

    VkDescriptorSetLayoutCreateInfo*    p_create_info = NULL;
    size_t                              create_info_count = 0;

    for (unsigned int module = 0; module < shader_modules_count; module++) {
        printf("module %d\n", module);

        // for each binding there is a binding and set number
        for (unsigned int binding_i = 0; binding_i < p_shader_modules[module].descriptor_binding_count; binding_i++) {
            printf("binding_i %d\n", binding_i);

            unsigned int set_number = p_shader_modules[module].descriptor_bindings[binding_i].set ;
            unsigned int binding_number = p_shader_modules[module].descriptor_bindings[binding_i].binding;

            printf("set_number %d\n", set_number);
            printf("binding_number %d\n", binding_number);

            // is set_number allready in p_desc_set_layouts?
            bool set_allready_exists = false;
            if (p_create_info) {
                if (create_info_count > set_number) {
                    if (p_create_info[set_number].sType == VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO)
                        set_allready_exists = true;
                }
            }

            // if not set allready exists in p_create_info then it needs to be allocated
            if (!set_allready_exists) {
                if (create_info_count <= set_number) {

                    size_t prev_count = create_info_count;
                    create_info_count = set_number+1;

                    TRACK(p_create_info = alloc(  p_create_info,   create_info_count * sizeof(VkDescriptorSetLayoutCreateInfo)  ));
                    memset(  &p_create_info[prev_count],  0,  (create_info_count - prev_count) * sizeof(VkDescriptorSetLayoutCreateInfo)  );
                    for (unsigned int new = create_info_count - prev_count; new < create_info_count; ++new) {
                        p_create_info[new].sType = VK_STRUCTURE_TYPE_MAX_ENUM;
                    }

                    printf("%p  p_create_info\n", p_create_info);
                }
                p_create_info[set_number].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            }

            if (p_create_info[set_number].pBindings) {
                for (unsigned int i = 0; i < p_create_info[set_number].bindingCount; i++) {
                    VERIFY(p_create_info[set_number].pBindings[i].binding != binding_number, "binding-set pair already exists");
                }
            }

            // allocating space for new binding
            p_create_info[set_number].bindingCount++;
            TRACK(p_create_info[set_number].pBindings = alloc(
                p_create_info[set_number].pBindings,  
                p_create_info[set_number].bindingCount * sizeof(VkDescriptorSetLayoutBinding)
            ));
            printf("%p p_create_info[set_number].pBindings \n", p_create_info[set_number].pBindings);

            // writing binding data
            VkDescriptorSetLayoutBinding* p_new_binding = &p_create_info[set_number].pBindings[p_create_info[set_number].bindingCount-1];
            p_new_binding->binding = binding_number;
            p_new_binding->descriptorType = p_shader_modules[module].descriptor_bindings[binding_i].descriptor_type;
            p_new_binding->descriptorCount = p_shader_modules[module].descriptor_bindings[binding_i].count;
            p_new_binding->stageFlags = (VkShaderStageFlags)p_shader_modules[module].shader_stage;
            p_new_binding->pImmutableSamplers = NULL; // Update if using immutable samplers

        }
    }

    for (unsigned int i = 0; i < create_info_count; i++) {
        VERIFY(p_create_info[i].sType == VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, "set numbers is not continuous");
    }

    TRACK(_gpi_DescriptorSetLayoutCreateInfo_Print(p_create_info, create_info_count));

    *p_create_info_count = create_info_count;
    return p_create_info;
}
VkDescriptorSetLayout*              _gpi_DescriptorSetLayout_Create(
    Gpi_Context* p_ctx, 
    const VkDescriptorSetLayoutCreateInfo* p_create_info, 
    const size_t create_info_count) 
{
    VERIFY(p_ctx, "p_ctx is NULL pointer");
    if (!p_create_info && create_info_count==0) {
        return NULL;
    }
    VERIFY(p_create_info, "p_create_info is NULL pointer");
    VERIFY(create_info_count > 0, "create_info_count is 0 even though p_create_info is not NULL");

    TRACK(VkDescriptorSetLayout* p_set_layout = alloc(NULL, create_info_count*sizeof(VkDescriptorSetLayout)));
    memset(p_set_layout, 0, create_info_count*sizeof(VkDescriptorSetLayout));

    for (unsigned int i = 0; i < create_info_count; ++i) {
        VERIFY(vkCreateDescriptorSetLayout(p_ctx->device, &p_create_info[i], NULL, &p_set_layout[i]) == VK_SUCCESS, "failed to create");
    }

    return p_set_layout;
}
Gpi_DescriptorSets 					gpi_DescriptorSets_Create(
	Gpi_Pipeline_Graphics* p_pipeline)
{
	VERIFY(p_pipeline, " ");
    VERIFY(p_pipeline->p_ctx, " ");
	Gpi_DescriptorSets desc_sets = {0};
	desc_sets.p_pipeline = p_pipeline;
	desc_sets.desc_sets_count = p_pipeline->desc_sets_count;
    TRACK(desc_sets.p_desc_sets = alloc(NULL, desc_sets.desc_sets_count * sizeof(VkDescriptorSet)));
    VERIFY(desc_sets.p_desc_sets, " ");

    for (unsigned int set = 0; set < desc_sets.desc_sets_count; ++set) {
        printf("set %d\n", set);
        VkDescriptorSetAllocateInfo alloc_info = {
            .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool     = p_pipeline->p_ctx->descriptor_pool,
            .descriptorSetCount = 1,
            .pSetLayouts        = &p_pipeline->p_desc_sets_layout[set]
        };
        TRACK(VkResult result = vkAllocateDescriptorSets(p_pipeline->p_ctx->device, &alloc_info, &desc_sets.p_desc_sets[set]));
        VERIFY(result == VK_SUCCESS, "Failed to allocate descriptor set. result = %d", result);
    }

    return desc_sets;
}
void 								gpi_DescriptorSets_Destroy(
	Gpi_Context* p_ctx, 
	Gpi_DescriptorSets* p_desc_sets) 
{
    VERIFY(p_desc_sets, "NULL pointer");
    VERIFY(p_ctx, "NULL pointer");

    if (p_desc_sets->p_desc_sets) {
        vkFreeDescriptorSets(p_ctx->device, p_ctx->descriptor_pool, p_desc_sets->desc_sets_count, p_desc_sets->p_desc_sets);
        free(p_desc_sets->p_desc_sets);
    }
    if (p_desc_sets->p_pipeline) {
        // Assuming p_pipeline is managed elsewhere, we do not destroy it here
        memset(p_desc_sets->p_pipeline, 0, sizeof(Gpi_Pipeline_Graphics));
    }
    memset(p_desc_sets, 0, sizeof(Gpi_DescriptorSets));
}
void 								gpi_DescriptorSets_UpdateUniformBuffer(
	Gpi_DescriptorSets* p_desc_sets, 
	const unsigned int set, 
	const unsigned int binding, 
	const unsigned int array_element, 
	const Gpi_Buffer* p_buffer, 
	const VkDeviceSize offset, 
	const VkDeviceSize range) 
{
    VERIFY(p_desc_sets, "set %d binding %d array_element %d", set, binding, array_element);
    VERIFY(p_desc_sets->p_pipeline, "set %d binding %d array_element %d", set, binding, array_element);
    Gpi_Pipeline_Graphics* p_pipeline = p_desc_sets->p_pipeline;
    VERIFY(p_pipeline->p_ctx, "set %d binding %d array_element %d", set, binding, array_element);
    VERIFY(p_pipeline->p_desc_sets_layout_create_info, "set %d binding %d array_element %d", set, binding, array_element);
    VERIFY(p_buffer, "set %d binding %d array_element %d", set, binding, array_element);
    if (range != VK_WHOLE_SIZE)
    	VERIFY(offset + range < p_buffer->size, "set %d binding %d array_element %d", set, binding, array_element);
    else
		VERIFY(offset < p_buffer->size, "set %d binding %d array_element %d", set, binding, array_element);
    VERIFY(p_pipeline, "set %d binding %d array_element %d", set, binding, array_element);
    VERIFY(set < p_pipeline->desc_sets_count, "Set index out of range. set %d binding %d array_element %d", set, binding, array_element);
    unsigned int binding_index = UINT32_MAX; // Sentinel value, assuming no binding will exceed this

    for (unsigned int i = 0; i < p_pipeline->p_desc_sets_layout_create_info[set].bindingCount; ++i) {
        if (binding == p_pipeline->p_desc_sets_layout_create_info[set].pBindings[i].binding) {
            binding_index = i;
            break;
        }
    }

    VERIFY(binding_index != UINT32_MAX, "Binding does not exist in graphics pipeline. set %d binding %d array_element %d", set, binding, array_element);
    VERIFY(p_pipeline->p_desc_sets_layout_create_info[set].pBindings[binding_index].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, "Binding is not a uniform buffer type");
    VERIFY(array_element < p_pipeline->p_desc_sets_layout_create_info[set].pBindings[binding_index].descriptorCount, "Array element is out of bounds for this binding");

    VkWriteDescriptorSet desc_write = {
        .sType              = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet             = p_desc_sets->p_desc_sets[set],
        .dstBinding         = binding,
        .dstArrayElement    = array_element,
        .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount    = 1,
        .pBufferInfo        = &(VkDescriptorBufferInfo){
            .buffer = p_buffer->buffer,
            .offset = offset,
            .range  = range,
        }
    };

    TRACK(vkUpdateDescriptorSets(p_pipeline->p_ctx->device, 1, &desc_write, 0, NULL));
}
void 								gpi_DescriptorSets_UpdateCombinedImageSampler(
	Gpi_DescriptorSets* p_desc_sets, 
	const unsigned int set, 
	const unsigned int binding, 
	const unsigned int array_element, 
	const Gpi_Image* p_image) 
{
    VERIFY(p_desc_sets, "NULL pointer");
    VERIFY(p_image, "NULL pointer");
    Gpi_Pipeline_Graphics* p_pipeline = p_desc_sets->p_pipeline;
    VERIFY(p_pipeline, "NULL pointer");
    VERIFY(set < p_pipeline->desc_sets_count, "Set index out of range");

    unsigned int binding_index = UINT32_MAX;

    for (unsigned int i = 0; i < p_pipeline->p_desc_sets_layout_create_info[set].bindingCount; ++i) {
        if (binding == p_pipeline->p_desc_sets_layout_create_info[set].pBindings[i].binding) {
            binding_index = i;
            break;
        }
    }

    VERIFY(binding_index != UINT32_MAX, "Could not find the specified binding");
    VERIFY(p_pipeline->p_desc_sets_layout_create_info[set].pBindings[binding_index].descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, "Binding is not of type COMBINED_IMAGE_SAMPLER");
    VERIFY(array_element < p_pipeline->p_desc_sets_layout_create_info[set].pBindings[binding_index].descriptorCount, "Array element is out of bounds");

    VkDescriptorImageInfo image_info = {
        .sampler     = p_image->sampler,
        .imageView   = p_image->view,
        .imageLayout = p_image->layout
    };

    VkWriteDescriptorSet desc_write = {
        .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet           = p_desc_sets->p_desc_sets[set],
        .dstBinding       = binding,
        .dstArrayElement  = array_element,
        .descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount  = 1,
        .pImageInfo       = &image_info
    };

    TRACK(vkUpdateDescriptorSets(p_pipeline->p_ctx->device, 1, &desc_write, 0, NULL));
}
void 								gpi_DescriptorSets_UpdateSampledImage(
	Gpi_DescriptorSets* p_desc_sets, 
	const unsigned int set, 
	const unsigned int binding, 
	const unsigned int array_element, 
	const VkImageView image_view, 
	const VkImageLayout image_layout) 
{
    VERIFY(p_desc_sets, "NULL pointer");
    Gpi_Pipeline_Graphics* p_pipeline = p_desc_sets->p_pipeline;
    VERIFY(p_pipeline, "NULL pointer");
    VERIFY(set < p_pipeline->desc_sets_count, "Set index out of range");

    unsigned int binding_index = UINT32_MAX;

    for (unsigned int i = 0; i < p_pipeline->p_desc_sets_layout_create_info[set].bindingCount; ++i) {
        if (binding == p_pipeline->p_desc_sets_layout_create_info[set].pBindings[i].binding) {
            binding_index = i;
            break;
        }
    }

    VERIFY(binding_index != UINT32_MAX, "Could not find the specified binding");
    VERIFY(p_pipeline->p_desc_sets_layout_create_info[set].pBindings[binding_index].descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, "Binding is not of type SAMPLED_IMAGE");
    VERIFY(array_element < p_pipeline->p_desc_sets_layout_create_info[set].pBindings[binding_index].descriptorCount, "Array element is out of bounds");

    VkDescriptorImageInfo image_info = {
        .sampler     = VK_NULL_HANDLE,
        .imageView   = image_view,
        .imageLayout = image_layout
    };

    VkWriteDescriptorSet desc_write = {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet          = p_desc_sets->p_desc_sets[set],
        .dstBinding      = binding,
        .dstArrayElement = array_element,
        .descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        .descriptorCount = 1,
        .pImageInfo      = &image_info
    };

    TRACK(vkUpdateDescriptorSets(p_pipeline->p_ctx->device, 1, &desc_write, 0, NULL));
}
void 								gpi_DescriptorSets_UpdateStorageImage(
	Gpi_DescriptorSets* p_desc_sets, 
	const unsigned int set, 
	const unsigned int binding, 
	const unsigned int array_element, 
	const VkImageView image_view, 
	const VkImageLayout image_layout) 
{
    VERIFY(p_desc_sets, "NULL pointer");
    Gpi_Pipeline_Graphics* p_pipeline = p_desc_sets->p_pipeline;
    VERIFY(p_pipeline, "NULL pointer");
    VERIFY(set < p_pipeline->desc_sets_count, "Set index out of range");

    unsigned int binding_index = UINT32_MAX;

    for (unsigned int i = 0; i < p_pipeline->p_desc_sets_layout_create_info[set].bindingCount; ++i) {
        if (binding == p_pipeline->p_desc_sets_layout_create_info[set].pBindings[i].binding) {
            binding_index = i;
            break;
        }
    }

    VERIFY(binding_index != UINT32_MAX, "Could not find the specified binding");
    VERIFY(p_pipeline->p_desc_sets_layout_create_info[set].pBindings[binding_index].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, "Binding is not of type STORAGE_IMAGE");
    VERIFY(array_element < p_pipeline->p_desc_sets_layout_create_info[set].pBindings[binding_index].descriptorCount, "Array element is out of bounds");

    VkDescriptorImageInfo image_info = {
        .sampler     = VK_NULL_HANDLE,
        .imageView   = image_view,
        .imageLayout = image_layout
    };

    VkWriteDescriptorSet desc_write = {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet          = p_desc_sets->p_desc_sets[set],
        .dstBinding      = binding,
        .dstArrayElement = array_element,
        .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .descriptorCount = 1,
        .pImageInfo      = &image_info
    };

    TRACK(vkUpdateDescriptorSets(p_pipeline->p_ctx->device, 1, &desc_write, 0, NULL));
}
void 								gpi_DescriptorSets_UpdateStorageBuffer(
	Gpi_DescriptorSets* p_desc_sets, 
	const unsigned int set, 
	const unsigned int binding,
	const unsigned int array_element, 
	const Gpi_Buffer buffer, 
	const VkDeviceSize offset, 
	const VkDeviceSize range) 
{
    VERIFY(p_desc_sets, "NULL pointer");
    Gpi_Pipeline_Graphics* p_pipeline = p_desc_sets->p_pipeline;
    VERIFY(p_pipeline, "NULL pointer");
    VERIFY(set < p_pipeline->desc_sets_count, "Set index out of range");

    unsigned int binding_index = UINT32_MAX;

    for (unsigned int i = 0; i < p_pipeline->p_desc_sets_layout_create_info[set].bindingCount; ++i) {
        if (binding == p_pipeline->p_desc_sets_layout_create_info[set].pBindings[i].binding) {
            binding_index = i;
            break;
        }
    }

    VERIFY(binding_index != UINT32_MAX, "Could not find the specified binding");
    VERIFY(p_pipeline->p_desc_sets_layout_create_info[set].pBindings[binding_index].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, "Binding is not of type STORAGE_BUFFER");
    VERIFY(array_element < p_pipeline->p_desc_sets_layout_create_info[set].pBindings[binding_index].descriptorCount, "Array element is out of bounds");

    VkDescriptorBufferInfo buffer_info = {
        .buffer = buffer.buffer,
        .offset = offset,
        .range  = range,
    };

    VkWriteDescriptorSet desc_write = {
        .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet           = p_desc_sets->p_desc_sets[set],
        .dstBinding       = binding,
        .dstArrayElement  = array_element,
        .descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .descriptorCount  = 1,
        .pBufferInfo      = &buffer_info
    };

    TRACK(vkUpdateDescriptorSets(p_pipeline->p_ctx->device, 1, &desc_write, 0, NULL));
}

// command buffer =========================================================================================================================
Gpi_Cmd                             gpi_Cmd_CreateAndStartRecording(
    Gpi_Context* p_ctx) 
{
    VERIFY(p_ctx, "NULL pointer");
    VERIFY(p_ctx->command_pool!=VK_NULL_HANDLE, "VK_NULL_HANDLE");
    VkCommandBufferAllocateInfo alloc_info = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = p_ctx->command_pool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };
    Gpi_Cmd cmd = {0};
    TRACK(VkResult result = vkAllocateCommandBuffers( p_ctx->device, &alloc_info, &cmd.command_buffer ));
    VERIFY(result == VK_SUCCESS, "Failed to allocate command buffers\n");
    VkCommandBufferBeginInfo begin_info = {0};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO; 
    TRACK(result = vkBeginCommandBuffer( cmd.command_buffer, &begin_info ));
    VERIFY(result == VK_SUCCESS, "failed to begin command buffer");
    return cmd;
}
void                                gpi_Cmd_RecordStaticRendering (
    Gpi_Cmd* p_cmd,
    Gpi_Context* p_ctx,
    Gpi_Image* p_target_image,
    VkDescriptorSet* p_desc_sets, 
    size_t desc_sets_count,
    VkPipeline graphics_pipeline,
    VkPipelineLayout graphics_pipeline_layout,
    VkBuffer instance_buffer,
    size_t instances_count)
{
    VERIFY(p_cmd, "NULL pointer");
    VERIFY(p_ctx, "NULL pointer");
    VERIFY(p_target_image, "NULL pointer");
    VERIFY(p_desc_sets, "NULL pointer");
    VERIFY(desc_sets_count, "count is 0");
    VERIFY(graphics_pipeline!=VK_NULL_HANDLE, "VK_NULL_HANDLE");
    VERIFY(graphics_pipeline_layout!=VK_NULL_HANDLE, "VK_NULL_HANDLE");
    VERIFY(instance_buffer!=VK_NULL_HANDLE, "VK_NULL_HANDLE");
    VERIFY(instances_count, "count is 0");

    VkImageMemoryBarrier barrier_to_color_attachment = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = p_target_image->image,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };

    TRACK(vkCmdPipelineBarrier(p_cmd->command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, NULL, 0, NULL, 1, &barrier_to_color_attachment));

    VkRenderingInfo rendering_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = { 
            .offset = {0, 0}, 
            .extent = p_target_image->extent 
        },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &(VkRenderingAttachmentInfo) {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = p_target_image->view,
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = { 
                .color = {
                    0.0f, 0.0f, 0.0f, 1.0f
                }
            },
        },
    };
        
    TRACK( vkCmdBeginRendering(p_cmd->command_buffer, &rendering_info ) );
    TRACK( vkCmdBindPipeline(p_cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline ) );
    TRACK( vkCmdBindDescriptorSets(p_cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline_layout, 0, desc_sets_count, p_desc_sets, 0, NULL ) );
    TRACK( vkCmdBindVertexBuffers(p_cmd->command_buffer, 0, 1, (VkBuffer[]){instance_buffer}, (VkDeviceSize[]){0} ) );
    TRACK( vkCmdDraw(p_cmd->command_buffer, 4, instances_count, 0, 0 ) );
    // vkCmdDrawIndirect
    TRACK( vkCmdEndRendering(p_cmd->command_buffer) );

    VkImageMemoryBarrier barrier_to_present = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = 0,
        .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = p_target_image->image,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };
    TRACK(vkCmdPipelineBarrier(p_cmd->command_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 1, &barrier_to_present));
}
void                                gpi_Cmd_RecordStaticRendering_Indirect (
    Gpi_Cmd* p_cmd,
    Gpi_Context* p_ctx,
    Gpi_Image* p_target_image,
    VkDescriptorSet* p_desc_sets, 
    size_t desc_sets_count,
    VkPipeline graphics_pipeline,
    VkPipelineLayout graphics_pipeline_layout,
    VkBuffer instance_buffer,
    size_t instances_count,
    Gpi_Buffer indirect_buffer)
{
    VERIFY(p_cmd, "NULL pointer");
    VERIFY(p_ctx, "NULL pointer");
    VERIFY(p_target_image, "NULL pointer");
    VERIFY(p_desc_sets, "NULL pointer");
    VERIFY(desc_sets_count, "count is 0");
    VERIFY(graphics_pipeline!=VK_NULL_HANDLE, "VK_NULL_HANDLE");
    VERIFY(graphics_pipeline_layout!=VK_NULL_HANDLE, "VK_NULL_HANDLE");
    VERIFY(instance_buffer!=VK_NULL_HANDLE, "VK_NULL_HANDLE");
    VERIFY(instances_count, "count is 0");

    VkImageMemoryBarrier barrier_to_color_attachment = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = p_target_image->image,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };

    TRACK(vkCmdPipelineBarrier(p_cmd->command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, NULL, 0, NULL, 1, &barrier_to_color_attachment));

    VkRenderingInfo rendering_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = { 
            .offset = {0, 0}, 
            .extent = p_target_image->extent 
        },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &(VkRenderingAttachmentInfo) {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = p_target_image->view,
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = { 
                .color = {
                    0.0f, 0.0f, 0.0f, 1.0f
                }
            },
        },
    };
        
    TRACK( vkCmdBeginRendering(p_cmd->command_buffer, &rendering_info ) );
    TRACK( vkCmdBindPipeline(p_cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline ) );
    TRACK( vkCmdBindDescriptorSets(p_cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline_layout, 0, desc_sets_count, p_desc_sets, 0, NULL ) );
    TRACK( vkCmdBindVertexBuffers(p_cmd->command_buffer, 0, 1, (VkBuffer[]){instance_buffer}, (VkDeviceSize[]){0} ) );
    TRACK( vkCmdDrawIndirect(p_cmd->command_buffer, indirect_buffer.buffer, 0, 1, indirect_buffer.size));
    TRACK( vkCmdEndRendering(p_cmd->command_buffer) );

    VkImageMemoryBarrier barrier_to_present = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = 0,
        .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = p_target_image->image,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };
    TRACK(vkCmdPipelineBarrier(p_cmd->command_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 1, &barrier_to_present));
}
void                                gpi_Cmd_StopRecording(
    Gpi_Cmd* p_cmd) 
{
    VERIFY(p_cmd, "NULL pointer");
    VERIFY(vkEndCommandBuffer(p_cmd->command_buffer) == VK_SUCCESS, "failed to end command buffer");
}
void                                gpi_Cmd_Destroy(
    Gpi_Context* p_ctx,
    Gpi_Cmd* p_cmd)
{
    VERIFY(p_ctx, "NULL pointer");
    VERIFY(p_ctx->device, "NULL pointer");
    VERIFY(p_ctx->command_pool, "NULL pointer");
    VERIFY(p_cmd, "NULL pointer");
    TRACK(vkFreeCommandBuffers(p_ctx->device, p_ctx->command_pool, 1, &p_cmd->command_buffer));
}
Gpi_Cmd                             gpi_Cmd_CreateAndBeginSingleTimeUsage(
    Gpi_Context* p_ctx) 
{
    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandPool = p_ctx->command_pool,
        .commandBufferCount = 1,
    };
    Gpi_Cmd cmd = {0};
    TRACK(vkAllocateCommandBuffers(p_ctx->device, &alloc_info, &cmd.command_buffer));
    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    TRACK(vkBeginCommandBuffer(cmd.command_buffer, &beginInfo));
    return cmd;
}
void                                gpi_Cmd_EndAndDestroySingleTimeUsage(
    Gpi_Context* p_ctx, 
    Gpi_Cmd* p_cmd)
{
    VERIFY(p_ctx, "NULL pointer");
    VERIFY(p_ctx->device, "NULL pointer");
    VERIFY(p_ctx->command_pool, "NULL pointer");
    VERIFY(p_cmd, "NULL pointer");
       
    TRACK(vkEndCommandBuffer(p_cmd->command_buffer));
    
    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &p_cmd->command_buffer,
    };
    
    TRACK(vkQueueSubmit(p_ctx->queues.graphics, 1, &submit_info, VK_NULL_HANDLE));
    TRACK(vkQueueWaitIdle(p_ctx->queues.graphics));
    TRACK(gpi_Cmd_Destroy(p_ctx, p_cmd));
}

// Semaphore =========================================================================================================================
Gpi_Semaphore                       gpi_Semaphore_Create(
    Gpi_Context* p_ctx) 
{
    VERIFY(p_ctx, "NULL pointer");
    VERIFY(p_ctx->device!=VK_NULL_HANDLE, "VK_NULL_HANDLE");
    VkSemaphoreCreateInfo semaphore_info = {};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    Gpi_Semaphore semaphore;
    VERIFY(vkCreateSemaphore(p_ctx->device, &semaphore_info, NULL, &semaphore.semaphore) == VK_SUCCESS, " ");
    return semaphore;
}

// Fence ==============================================================================================================================
Gpi_Fence                           gpi_Fence_Create(
    Gpi_Context* p_ctx) 
{
    VERIFY(p_ctx, "NULL pointer");
    VERIFY(p_ctx->device!=VK_NULL_HANDLE, "VK_NULL_HANDLE");
    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    Gpi_Fence fence;
    VERIFY(vkCreateFence(p_ctx->device, &fence_info, NULL, &fence.fence) == VK_SUCCESS, " ");
    return fence;
}