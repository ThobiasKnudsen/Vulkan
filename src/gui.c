// main.c
#define VK_USE_PLATFORM_XLIB_KHR
#define USE_SDL2

#include "gui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <shaderc/shaderc.h>
#ifdef USE_SDL2
    #include <SDL2/SDL.h>
    #include <SDL2/SDL_vulkan.h>
#endif

#ifdef USE_GLFW
    #include <GLFW/glfw3.h>
#endif


#define DEBUG
#include "debug.h"

// Define the all_instances array
const InstanceData all_instances[ALL_INSTANCE_COUNT] = {
    // Instance 0
    {
        .pos = {0.0f, 0.0f},
        .size = {100.0f, 100.0f},
        .rotation = 0.0f,
        .corner_radius = 100.0f,
        .color = 0xFF0000FF, // Red color in RGBA8
        .tex_index = 0,
        .tex_rect = {0.0f, 0.0f, 1.0f, 1.0f}
    },
    // Instance 1
    {
        .pos = {100.0f, 100.0f},
        .size = {100.0f, 100.0f},
        .rotation = 30.0f,
        .corner_radius = 100.0f,
        .color = 0xFF00FF00, // Green color in RGBA8
        .tex_index = 0,
        .tex_rect = {0.0f, 0.0f, 1.0f, 1.0f}
    },
    // Instance 2
    {
        .pos = {200.f, 200.0f},
        .size = {100.0f, 100.0f},
        .rotation = 60.0f,
        .corner_radius = 100.0f,
        .color = 0xFFFF0000, // Blue color in RGBA8
        .tex_index = 0,
        .tex_rect = {0.0f, 0.0f, 1.0f, 1.0f}
    },
    // Instance 3
    {
        .pos = {300.0f, 300.0f},
        .size = {100.0f, 100.0f},
        .rotation = 90.0f,
        .corner_radius = 100.0f,
        .color = 0xFFFFFF00, // Cyan color in RGBA8
        .tex_index = 0,
        .tex_rect = {0.0f, 0.0f, 1.0f, 1.0f}
    },
    // Instance 4
    {
        .pos = {400.0f, 400.0f},
        .size = {100.0f, 100.0f},
        .rotation = 120.0f,
        .corner_radius = 100.0f,
        .color = 0xFF00FFFF, // Yellow color in RGBA8
        .tex_index = 0,
        .tex_rect = {0.0f, 0.0f, 1.0f, 1.0f}
    },
};

// Indices array
#define INDICES_COUNT 3
uint32_t indices[INDICES_COUNT] = { 0, 2, 4 }; // Indices of instances to draw

// Utility function for logging Vulkan errors
static void logVulkanError(const char* msg) {
    printf("%s\n", msg);
}

// Function Implementations

void cleanup(
    VkDevice device,
    VkInstance instance,
    VkSurfaceKHR surface,
    VkDebugUtilsMessengerEXT debugMessenger,
    VkPipeline graphicsPipeline,
    VkPipelineLayout pipelineLayout,
    VkRenderPass renderPass,
    VkDescriptorSetLayout descriptorSetLayout,
    VkDescriptorPool descriptorPool,
    VkBuffer uniformBuffer,
    VkDeviceMemory uniformBufferMemory,
    VkBuffer instanceBuffer,
    VkDeviceMemory instanceBufferMemory,
    VkDescriptorSet descriptorSet,
    VkSwapchainKHR swapChain,
    VkImageView* swapChainImageViews,
    uint32_t swapChainImageCount,
    VkFramebuffer* swapChainFramebuffers,
    VkCommandPool commandPool,
    VkCommandBuffer* commandBuffers,
    VkSemaphore imageAvailableSemaphore,
    VkSemaphore renderFinishedSemaphore,
    VkFence inFlightFence,
    void* window
) {
    if (device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device);
    }

    // Destroy Vulkan objects in reverse order of creation
    if (graphicsPipeline != VK_NULL_HANDLE)
        vkDestroyPipeline(device, graphicsPipeline, NULL);

    if (pipelineLayout != VK_NULL_HANDLE)
        vkDestroyPipelineLayout(device, pipelineLayout, NULL);

    if (renderPass != VK_NULL_HANDLE)
        vkDestroyRenderPass(device, renderPass, NULL);

    if (descriptorSetLayout != VK_NULL_HANDLE)
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, NULL);

    if (descriptorPool != VK_NULL_HANDLE)
        vkDestroyDescriptorPool(device, descriptorPool, NULL);

    if (uniformBuffer != VK_NULL_HANDLE)
        vkDestroyBuffer(device, uniformBuffer, NULL);

    if (uniformBufferMemory != VK_NULL_HANDLE)
        vkFreeMemory(device, uniformBufferMemory, NULL);

    if (instanceBuffer != VK_NULL_HANDLE)
        vkDestroyBuffer(device, instanceBuffer, NULL);

    if (instanceBufferMemory != VK_NULL_HANDLE)
        vkFreeMemory(device, instanceBufferMemory, NULL);

    if (swapChainFramebuffers != NULL) {
        for (uint32_t i = 0; i < swapChainImageCount; i++) {
            if (swapChainFramebuffers[i] != VK_NULL_HANDLE)
                vkDestroyFramebuffer(device, swapChainFramebuffers[i], NULL);
        }
        free(swapChainFramebuffers);
    }

    if (swapChainImageViews != NULL) {
        for (uint32_t i = 0; i < swapChainImageCount; i++) {
            if (swapChainImageViews[i] != VK_NULL_HANDLE)
                vkDestroyImageView(device, swapChainImageViews[i], NULL);
        }
        free(swapChainImageViews);
    }

    if (swapChain != VK_NULL_HANDLE)
        vkDestroySwapchainKHR(device, swapChain, NULL);

    if (commandPool != VK_NULL_HANDLE)
        vkDestroyCommandPool(device, commandPool, NULL);

    if (imageAvailableSemaphore != VK_NULL_HANDLE)
        vkDestroySemaphore(device, imageAvailableSemaphore, NULL);

    if (renderFinishedSemaphore != VK_NULL_HANDLE)
        vkDestroySemaphore(device, renderFinishedSemaphore, NULL);

    if (inFlightFence != VK_NULL_HANDLE)
        vkDestroyFence(device, inFlightFence, NULL);

    if (device != VK_NULL_HANDLE)
        vkDestroyDevice(device, NULL);

    if (debugMessenger != VK_NULL_HANDLE) {
        PFN_vkDestroyDebugUtilsMessengerEXT funcDestroyDebugUtilsMessengerEXT = 
            (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (funcDestroyDebugUtilsMessengerEXT != NULL)
            funcDestroyDebugUtilsMessengerEXT(instance, debugMessenger, NULL);
    }

    if (surface != VK_NULL_HANDLE)
        vkDestroySurfaceKHR(instance, surface, NULL);

    if (instance != VK_NULL_HANDLE)
        vkDestroyInstance(instance, NULL);


    #if defined(USE_SDL2)
        if (window != NULL)
            SDL_DestroyWindow(window);
        SDL_Quit();
    #endif
}

unsigned int readShaderSource(const char* filename, char** buffer) {
    FILE* file = fopen(filename, "rb"); // Open in binary mode
    if (!file) {
        printf("Failed to open shader source file '%s'\n", filename);
        return 0;
    }

    fseek(file, 0, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    *buffer = (char*)malloc(fileSize + 1);
    if (!*buffer) {
        printf("Failed to allocate memory for shader source '%s'\n", filename);
        fclose(file);
        return 0;
    }

    size_t readSize = fread(*buffer, 1, fileSize, file);
    (*buffer)[fileSize] = '\0'; // Null-terminate the string

    if (readSize != fileSize) {
        printf("Failed to read shader source '%s'\n", filename);
        free(*buffer);
        fclose(file);
        return 0;
    }

    fclose(file);
    return (unsigned int)fileSize;
}

VkShaderModule createShaderModule(const char* filename, shaderc_shader_kind kind, VkDevice device) {
    char* sourceCode = NULL;
    unsigned int sourceSize = readShaderSource(filename, &sourceCode);
    if (sourceSize == 0 || sourceCode == NULL) {
        printf("Failed to read shader source '%s'\n", filename);
        exit(EXIT_FAILURE);
    }
    shaderc_compiler_t compiler = shaderc_compiler_initialize();
    shaderc_compile_options_t options = shaderc_compile_options_initialize();
    shaderc_compile_options_set_optimization_level(options, shaderc_optimization_level_performance);
    shaderc_compilation_result_t result = shaderc_compile_into_spv(compiler, sourceCode, sourceSize, kind, filename, "main", options);
    if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) {
        printf("Shader compilation error in '%s':\n%s\n", filename, shaderc_result_get_error_message(result));
        shaderc_result_release(result);
        shaderc_compiler_release(compiler);
        free(sourceCode);
        exit(EXIT_FAILURE);
    }
    size_t codeSize = shaderc_result_get_length(result);
    const uint32_t* code = (const uint32_t*)shaderc_result_get_bytes(result);

    VkShaderModuleCreateInfo createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = codeSize;
    createInfo.pCode = code;

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    if (vkCreateShaderModule(device, &createInfo, NULL, &shaderModule) != VK_SUCCESS) {
        printf("Failed to create shader module from '%s'\n", filename);
        shaderc_result_release(result);
        shaderc_compiler_release(compiler);
        free(sourceCode);
        exit(EXIT_FAILURE);
    }

    // Cleanup
    shaderc_result_release(result);
    shaderc_compiler_release(compiler);
    free(sourceCode);

    return shaderModule;
}

// Debug Callback
VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    printf("Vulkan Debug: %s\n", pCallbackData->pMessage);
    return VK_FALSE;
}

// Find Memory Type
uint32_t findMemoryType(
    VkPhysicalDevice physicalDevice,
    uint32_t typeFilter,
    VkMemoryPropertyFlags properties
) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for(uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if( (typeFilter & (1 << i)) && 
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties ) {
            return i;
        }
    }

    printf("Failed to find suitable memory type!\n");
    exit(EXIT_FAILURE);
}

void* createWindow(int width, int height, const char* title) {
    #ifdef USE_SDL2
        // Initialize SDL
        if (SDL_Init(SDL_INIT_VIDEO) != 0) {
            printf("ERROR: SDL_Init: %s\n", SDL_GetError());
            exit(EXIT_FAILURE);
        }

        // Create SDL Window
        SDL_Window* window = SDL_CreateWindow(title,SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,width,height,SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN);

        if (!window) {
            printf("ERROR: SDL_CreateWindow: %s\n", SDL_GetError());
            SDL_Quit();
            exit(EXIT_FAILURE);
        }

        return (void*)window;

    #elif defined(USE_GLFW)
        // Initialize GLFW
        if (!glfwInit()) {
            fprintf(stderr, "ERROR: Failed to initialize GLFW\n");
            exit(EXIT_FAILURE);
        }

        // Specify that we want Vulkan support
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        // Create GLFW Window
        GLFWwindow* window = glfwCreateWindow(width, height, title, NULL, NULL);
        if (!window) {
            fprintf(stderr, "ERROR: Failed to create GLFW window\n");
            glfwTerminate();
            exit(EXIT_FAILURE);
        }

        return (void*)window;

    #else
        printf("there is no supported windowing API!\n");
        exit(EXIT_FAILURE);
    #endif
}

// Create Vulkan Instance
VkInstance createVulkanInstance(void* window) {

    uint32_t totalExtensionCount = 1;
    const char** allExtensions = NULL;
    uint32_t validationLayerCount = 0;
    const char* validationLayers[] = { "VK_LAYER_KHRONOS_validation" };

    #ifdef USE_SDL2
        uint32_t sdlExtensionCount = 0;
        if (!SDL_Vulkan_GetInstanceExtensions((SDL_Window*)window, &sdlExtensionCount, NULL)) {
            printf("Failed to get SDL Vulkan instance extensions: %s\n", SDL_GetError());
            exit(EXIT_FAILURE);
        }

        const char** sdlExtensions = (const char**)malloc(sizeof(const char*) * sdlExtensionCount);
        if (sdlExtensions == NULL) {
            printf("Failed to allocate memory for SDL Vulkan extensions\n");
            exit(EXIT_FAILURE);
        }

        if (!SDL_Vulkan_GetInstanceExtensions((SDL_Window*)window, &sdlExtensionCount, sdlExtensions)) {
            printf("Failed to get SDL Vulkan instance extensions: %s\n", SDL_GetError());
            free(sdlExtensions);
            exit(EXIT_FAILURE);
        }

        // Optional: Add VK_EXT_debug_utils_EXTENSION_NAME for debugging
        totalExtensionCount = sdlExtensionCount + 1;
        allExtensions = (const char**)malloc(sizeof(const char*) * totalExtensionCount);
        if (allExtensions == NULL) {
            printf("Failed to allocate memory for all Vulkan extensions\n");
            free(sdlExtensions);
            exit(EXIT_FAILURE);
        }
        memcpy(allExtensions, sdlExtensions, sizeof(const char*) * sdlExtensionCount);
        allExtensions[sdlExtensionCount] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

        validationLayerCount = 1;
    #else
        printf("SDL2 has to be included\n");
        exit(EXIT_FAILURE);
    #endif

    VkInstanceCreateInfo instanceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &(VkApplicationInfo){
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "Logos Application",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "Logos Engine",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = VK_API_VERSION_1_2
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

    VkInstance vulkan_instance;

    VkResult result = vkCreateInstance(&instanceCreateInfo, NULL, &vulkan_instance);
    #if defined(USE_SDL2)
        free(sdlExtensions);
    #endif
    free(allExtensions);
    if (result != VK_SUCCESS) {
        printf("Failed to create Vulkan instance\n");
        exit(EXIT_FAILURE);
    }

    return vulkan_instance;
}

// Setup Debug Messenger
VkDebugUtilsMessengerEXT setupDebugMessenger(VkInstance instance) {
    // Function pointer for vkCreateDebugUtilsMessengerEXT
    PFN_vkCreateDebugUtilsMessengerEXT funcCreateDebugUtilsMessengerEXT = 
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
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

        VkDebugUtilsMessengerEXT debug_messenger = VK_NULL_HANDLE;
        if (funcCreateDebugUtilsMessengerEXT(instance, &debugCreateInfo, NULL, &debug_messenger) != VK_SUCCESS) {
            printf("Failed to set up debug messenger\n");
            exit(EXIT_FAILURE);
        }
        return debug_messenger;
    } else {
        printf("vkCreateDebugUtilsMessengerEXT not available\n");
    }
    return VK_NULL_HANDLE;
}

// Create Vulkan Surface
VkSurfaceKHR createSurface(void* window, VkInstance instance) {
    VkSurfaceKHR vk_surface = VK_NULL_HANDLE;
    #if defined(USE_SDL2)
        if (!SDL_Vulkan_CreateSurface((SDL_Window*)window, instance, &vk_surface)) {
            printf( "Failed to create Vulkan surface: %s\n", SDL_GetError());
            exit(EXIT_FAILURE);
        }
    #else
        printf("SDL2 has to be included\n");
        exit(EXIT_FAILURE);
    #endif
    return vk_surface;
}

// Pick Physical Device
VkPhysicalDevice getPhysicalDevice(VkInstance instance) {
    uint32_t device_count = 0;
    VkResult result = vkEnumeratePhysicalDevices(instance, &device_count, NULL);
    if (result != VK_SUCCESS || device_count == 0) {
        printf( "Failed to find GPUs with Vulkan support\n");
        exit(EXIT_FAILURE);
    }

    VkPhysicalDevice* devices = malloc(sizeof(VkPhysicalDevice) * device_count);
    if (devices == NULL) {
        printf( "Failed to allocate memory for physical devices\n");
        exit(EXIT_FAILURE);
    }
    result = vkEnumeratePhysicalDevices(instance, &device_count, devices);
    if (result != VK_SUCCESS) {
        printf( "Failed to enumerate physical devices\n");
        free(devices);
        exit(EXIT_FAILURE);
    }

    // Select the first suitable device
    VkPhysicalDevice physicalDevice = devices[0];
    free(devices);

    return physicalDevice;
}

// Find Queue Family Indices
VkQueueFamilyIndices getQueueFamilyIndices(VkSurfaceKHR surface, VkPhysicalDevice physicalDevice) {
    VkQueueFamilyIndices queue_family_indices = { 
        .graphics = UINT32_MAX,
        .present = UINT32_MAX,
        .compute = UINT32_MAX,
        .transfer = UINT32_MAX
    };

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queue_family_count, NULL);
    if (queue_family_count == 0) {
        printf("Failed to find any queue families\n");
        exit(EXIT_FAILURE);
    }

    VkQueueFamilyProperties* queue_families = malloc(sizeof(VkQueueFamilyProperties) * queue_family_count);
    if (queue_families == NULL) {
        printf("Failed to allocate memory for queue family properties\n");
        exit(EXIT_FAILURE);
    }

    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queue_family_count, queue_families);

    for (uint32_t i = 0; i < queue_family_count; i++) {
        // Check for graphics support
        if ((queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && queue_family_indices.graphics == UINT32_MAX) {
            queue_family_indices.graphics = i;
        }

        // Check for compute support (prefer dedicated compute queues)
        if ((queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT) && 
            !(queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && 
            queue_family_indices.compute == UINT32_MAX) {
            queue_family_indices.compute = i;
        }

        // Check for transfer support (prefer dedicated transfer queues)
        if ((queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT) && 
            !(queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && 
            queue_family_indices.transfer == UINT32_MAX) {
            queue_family_indices.transfer = i;
        }

        // Check for presentation support
        VkBool32 present_support = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &present_support);
        if (present_support && queue_family_indices.present == UINT32_MAX) {
            queue_family_indices.present = i;
        }

        // Break early if all family_indices are found
        if (queue_family_indices.graphics != UINT32_MAX &&
            queue_family_indices.present != UINT32_MAX &&
            queue_family_indices.compute != UINT32_MAX &&
            queue_family_indices.transfer != UINT32_MAX) {
            break;
        }
    }

    free(queue_families);

    // Validate that essential queue family_indices are found
    if (queue_family_indices.graphics == UINT32_MAX || queue_family_indices.present == UINT32_MAX) {
        printf("Failed to find required queue families\n");
        exit(EXIT_FAILURE);
    }

    return queue_family_indices;
}

// Create Logical Device
VkDevice getDevice(VkPhysicalDevice physicalDevice, VkQueueFamilyIndices queue_family_indices) {
    VkDevice device;
    VkResult result;

    // Define queue priorities
    float queue_priority = 1.0f;

    // To avoid creating multiple VkDeviceQueueCreateInfo for the same family,
    // collect unique queue family queue_family_indices
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
    ADD_UNIQUE_FAMILY(queue_family_indices.graphics);
    ADD_UNIQUE_FAMILY(queue_family_indices.present);
    ADD_UNIQUE_FAMILY(queue_family_indices.compute);
    ADD_UNIQUE_FAMILY(queue_family_indices.transfer);

    #undef ADD_UNIQUE_FAMILY

    // Allocate array for VkDeviceQueueCreateInfo
    VkDeviceQueueCreateInfo* queue_create_infos = malloc(sizeof(VkDeviceQueueCreateInfo) * unique_count);
    if (!queue_create_infos) {
        printf("Failed to allocate memory for queue create infos.\n");
        exit(EXIT_FAILURE);
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

    // Specify device features (optional)
    // VkPhysicalDeviceFeatures device_features = {0};
    // Enable required features here if needed, e.g., device_features.samplerAnisotropy = VK_TRUE;

    // Create device create info
    VkDeviceCreateInfo device_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pQueueCreateInfos = queue_create_infos,
        .queueCreateInfoCount = unique_count,
        .pEnabledFeatures = &(VkPhysicalDeviceFeatures){0},
        .enabledExtensionCount = 1,
        .ppEnabledExtensionNames = &(const char*){VK_KHR_SWAPCHAIN_EXTENSION_NAME},
        .enabledLayerCount = 0, // Deprecated in newer Vulkan versions
        .ppEnabledLayerNames = NULL,
        .pNext = NULL
    };

    // Create the logical device
    result = vkCreateDevice(physicalDevice, &device_create_info, NULL, &device);
    if (result != VK_SUCCESS) {
        printf("Failed to create logical device. Error code: %d\n", result);
        free(queue_create_infos);
        exit(EXIT_FAILURE);
    }

    printf("Logical device created successfully.\n");

    // Clean up
    free(queue_create_infos);

    return device;
}

// Get Queues
VkQueues getQueues(VkDevice device, VkQueueFamilyIndices queue_family_indices) {
    VkQueues queues = {
        .graphics = VK_NULL_HANDLE,
        .present = VK_NULL_HANDLE,
        .compute = VK_NULL_HANDLE,
        .transfer = VK_NULL_HANDLE
    };

    // Retrieve Graphics Queue
    vkGetDeviceQueue(device, queue_family_indices.graphics, 0, &queues.graphics);
    printf("Graphics queue retrieved.\n");

    // Retrieve Presentation Queue
    if (queue_family_indices.graphics != queue_family_indices.present) {
        vkGetDeviceQueue(device, queue_family_indices.present, 0, &queues.present);
        printf("Presentation queue retrieved.\n");
    } else {
        queues.present = queues.graphics;
        printf("Presentation queue is the same as graphics queue.\n");
    }

    // Retrieve Compute Queue
    if (queue_family_indices.compute != queue_family_indices.graphics && queue_family_indices.compute != queue_family_indices.present) {
        vkGetDeviceQueue(device, queue_family_indices.compute, 0, &queues.compute);
        printf("Compute queue retrieved.\n");
    } else {
        queues.compute = queues.graphics;
        printf("Compute queue is the same as graphics queue.\n");
    }

    // Retrieve Transfer Queue
    if (queue_family_indices.transfer != queue_family_indices.graphics && 
        queue_family_indices.transfer != queue_family_indices.present && 
        queue_family_indices.transfer != queue_family_indices.compute) {
        vkGetDeviceQueue(device, queue_family_indices.transfer, 0, &queues.transfer);
        printf("Transfer queue retrieved.\n");
    } else {
        queues.transfer = queues.graphics;
        printf("Transfer queue is the same as graphics queue.\n");
    }

    return queues;
}

// Create Descriptor Set Layout
VkDescriptorSetLayout createDescriptorSetLayout(VkDevice device) {
    VkDescriptorSetLayoutCreateInfo layout_info = {
        .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings    = &(VkDescriptorSetLayoutBinding) {
            .binding            = 0, // Binding 0 in the shader
            .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount    = 1,
            .stageFlags         = VK_SHADER_STAGE_VERTEX_BIT, // Used in vertex shader
            .pImmutableSamplers = NULL
        }
    };

    VkDescriptorSetLayout descriptorSetLayout;
    if (vkCreateDescriptorSetLayout(device, &layout_info, NULL, &descriptorSetLayout) != VK_SUCCESS) {
        printf("Failed to create descriptor set layout\n");
        exit(EXIT_FAILURE);
    }

    return descriptorSetLayout;
}

// Create Pipeline Layout
VkPipelineLayout createPipelineLayout(VkDevice device, VkDescriptorSetLayout layout) {
    VkPipelineLayoutCreateInfo vk_pipeline_layoutInfo = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount         = 1, // One descriptor set layout
        .pSetLayouts            = &layout,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges    = NULL
    };

    VkPipelineLayout pipelineLayout;
    if (vkCreatePipelineLayout(device, &vk_pipeline_layoutInfo, NULL, &pipelineLayout) != VK_SUCCESS) {
        printf("Failed to create pipeline layout\n");
        exit(EXIT_FAILURE);
    }

    return pipelineLayout;
}

// Create Swap Chain
VkSwapchainKHR createSwapChain(
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkSurfaceKHR surface,
    VkQueueFamilyIndices queue_family_indices,
    VkExtent2D extent,
    VkFormat* swapChainImageFormat
) {
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities);
    if (result != VK_SUCCESS) {
        printf("Failed to get surface capabilities\n");
        exit(EXIT_FAILURE);
    }

    VkExtent2D actualExtent = surfaceCapabilities.currentExtent.width != UINT32_MAX ?
                              surfaceCapabilities.currentExtent :
                              extent;

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, NULL);
    if (formatCount == 0) {
        printf("Failed to find surface formats\n");
        exit(EXIT_FAILURE);
    }
    VkSurfaceFormatKHR* formats = malloc(sizeof(VkSurfaceFormatKHR) * formatCount);
    if (formats == NULL) {
        printf("Failed to allocate memory for surface formats\n");
        exit(EXIT_FAILURE);
    }
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats);

    // Choose a suitable format (e.g., prefer SRGB)
    VkSurfaceFormatKHR chosenFormat = formats[0];
    for (uint32_t i = 0; i < formatCount; i++) {
        if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
            formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            chosenFormat = formats[i];
            break;
        }
    }
    *swapChainImageFormat = chosenFormat.format;
    free(formats);

    VkQueueFamilyIndices i = queue_family_indices;
    VkSwapchainCreateInfoKHR swapchainCreateInfo = {
        .sType           = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface         = surface,
        .minImageCount   = surfaceCapabilities.minImageCount + 1,
        .minImageCount   = surfaceCapabilities.maxImageCount > 0 && surfaceCapabilities.minImageCount + 1 > surfaceCapabilities.maxImageCount ? 
                           surfaceCapabilities.maxImageCount : surfaceCapabilities.minImageCount + 1,
        .imageFormat     = chosenFormat.format,
        .imageColorSpace = chosenFormat.colorSpace,
        .imageExtent     = actualExtent,
        .imageArrayLayers = 1,
        .imageUsage      = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode      = i.graphics!=i.present ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = i.graphics!=i.present ? 2 : 0,
        .pQueueFamilyIndices   = i.graphics!=i.present ? (unsigned int[]){i.graphics, i.present} : NULL,
        .preTransform   = surfaceCapabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode    = VK_PRESENT_MODE_FIFO_KHR, // Guaranteed to be available
        .clipped        = VK_TRUE,
        .oldSwapchain   = VK_NULL_HANDLE
    };

    VkSwapchainKHR swapChain;
    result = vkCreateSwapchainKHR(device, &swapchainCreateInfo, NULL, &swapChain);
    if (result != VK_SUCCESS) {
        printf("Failed to create swapchain\n");
        exit(EXIT_FAILURE);
    }

    return swapChain;
}

// Create Render Pass
VkRenderPass createRenderPass(VkDevice device, VkFormat imageFormat) {
    VkRenderPassCreateInfo renderPassInfo   = {
        .sType                              = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount                    = 1,
        .pAttachments                       = (VkAttachmentDescription[]){{
            .format                         = imageFormat,
            .samples                        = VK_SAMPLE_COUNT_1_BIT,
            .loadOp                         = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp                        = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp                  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp                 = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout                  = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout                    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        }},
        .subpassCount                       = 1,
        .pSubpasses                         = (VkSubpassDescription[]){{
            .pipelineBindPoint              = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount           = 1,
            .pColorAttachments              = (VkAttachmentReference[]) 
            {{
                .attachment                 = 0,
                .layout                     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            }}
        }},
        .dependencyCount                    = 1,
        .pDependencies                      = (VkSubpassDependency[]){{
            .srcSubpass                     = VK_SUBPASS_EXTERNAL,
            .dstSubpass                     = 0,
            .srcStageMask                   = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask                  = 0,
            .dstStageMask                   = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstAccessMask                  = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
        }}
    };

    VkRenderPass renderPass;
    if (vkCreateRenderPass(device, &renderPassInfo, NULL, &renderPass) != VK_SUCCESS) {
        printf("Failed to create render pass\n");
        exit(EXIT_FAILURE);
    }

    return renderPass;
}

// Create Graphics Pipeline
VkPipeline createGraphicsPipeline(
    VkDevice device,
    VkPipelineLayout pipelineLayout,
    VkRenderPass renderPass,
    VkExtent2D swapChainExtent
) {
    VkGraphicsPipelineCreateInfo pipelineInfo = {
        .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount          = 2,
        .pStages             = (VkPipelineShaderStageCreateInfo[2]) {{
            .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage  = VK_SHADER_STAGE_VERTEX_BIT,
            .module = createShaderModule("shaders/shader.vert.glsl", shaderc_glsl_vertex_shader, device),
            .pName  = "main"
        },{
            .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = createShaderModule("shaders/shader.frag.glsl", shaderc_glsl_fragment_shader, device),
            .pName  = "main"
        }},
        .pVertexInputState = &(VkPipelineVertexInputStateCreateInfo) {
            .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount   = 1,
            .pVertexBindingDescriptions      = (VkVertexInputBindingDescription[1]) {{
                .binding   = 0,
                .stride    = sizeof(InstanceData),
                .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE
            }},
            .vertexAttributeDescriptionCount = 7,
            .pVertexAttributeDescriptions    = (VkVertexInputAttributeDescription[7]) {{
                .binding  = 0,
                .location = 0,
                .format   = VK_FORMAT_R32G32_SFLOAT,
                .offset   = offsetof(InstanceData, pos)
                },{
                .binding  = 0,
                .location = 1,
                .format   = VK_FORMAT_R32G32_SFLOAT,
                .offset   = offsetof(InstanceData, size)
                },{
                .binding  = 0,
                .location = 2,
                .format   = VK_FORMAT_R32_SFLOAT,
                .offset   = offsetof(InstanceData, rotation)
                },{
                .binding  = 0,
                .location = 3,
                .format   = VK_FORMAT_R32_SFLOAT,
                .offset   = offsetof(InstanceData, corner_radius)
                },{
                .binding  = 0,
                .location = 4,
                .format   = VK_FORMAT_R8G8B8A8_UNORM,
                .offset   = offsetof(InstanceData, color)
                },{
                .binding  = 0,
                .location = 5,
                .format   = VK_FORMAT_R32_UINT,
                .offset   = offsetof(InstanceData, tex_index)
                },{
                .binding  = 0,
                .location = 6,
                .format   = VK_FORMAT_R32G32B32A32_SFLOAT,
                .offset   = offsetof(InstanceData, tex_rect)
            }},
        }, 
        .pInputAssemblyState = &(VkPipelineInputAssemblyStateCreateInfo) {
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
            .primitiveRestartEnable = VK_FALSE
        },
        .pViewportState      = &(VkPipelineViewportStateCreateInfo) {
            .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .pViewports    = &(VkViewport) {
                .x        = 0.0f,
                .y        = 0.0f,
                .width    = (float)swapChainExtent.width,
                .height   = (float)swapChainExtent.height,
                .minDepth = 0.0f,
                .maxDepth = 1.0f
            },
            .scissorCount  = 1,
            .pScissors     = &(VkRect2D) {
                .offset = (VkOffset2D){0, 0},
                .extent = swapChainExtent
            }
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
            .depthWriteEnable       = VK_FALSE, // Disable depth writes
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
        .pDynamicState       = NULL,
        .layout              = pipelineLayout,
        .renderPass          = renderPass,
        .subpass             = 0,
        .basePipelineHandle  = VK_NULL_HANDLE,
        .basePipelineIndex   = -1
    };

    VkPipeline graphicsPipeline;
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &graphicsPipeline) != VK_SUCCESS) {
        printf("Failed to create graphics pipeline\n");
        exit(EXIT_FAILURE);
    }

    vkDestroyShaderModule(device, pipelineInfo.pStages[0].module, NULL);
    vkDestroyShaderModule(device, pipelineInfo.pStages[1].module, NULL);

    return graphicsPipeline;
}

// Create Image Views
VkImageView* createImageViews(
    VkDevice device,
    VkSwapchainKHR swapChain,
    VkFormat imageFormat,
    uint32_t imageCount
) {
    VkImage* swapChainImages = malloc(sizeof(VkImage) * imageCount);
    if (swapChainImages == NULL) {
        printf("Failed to allocate memory for swapchain images\n");
        exit(EXIT_FAILURE);
    }

    VkResult result = vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages);
    if (result != VK_SUCCESS) {
        printf("Failed to get swapchain images\n");
        free(swapChainImages);
        exit(EXIT_FAILURE);
    }

    VkImageView* swapChainImageViews = malloc(sizeof(VkImageView) * imageCount);
    if (swapChainImageViews == NULL) {
        printf("Failed to allocate memory for image views\n");
        free(swapChainImages);
        exit(EXIT_FAILURE);
    }

    for (uint32_t i = 0; i < imageCount; i++) {
        VkImageViewCreateInfo viewInfo = {
            .sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image                           = swapChainImages[i],
            .viewType                        = VK_IMAGE_VIEW_TYPE_2D,
            .format                          = imageFormat,
            .components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY,
            .subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.baseMipLevel   = 0,
            .subresourceRange.levelCount     = 1,
            .subresourceRange.baseArrayLayer = 0,
            .subresourceRange.layerCount     = 1
        };

        if (vkCreateImageView(device, &viewInfo, NULL, &swapChainImageViews[i]) != VK_SUCCESS) {
            printf("Failed to create image view %u\n", i);
            free(swapChainImages);
            for (uint32_t j = 0; j < i; j++) {
                vkDestroyImageView(device, swapChainImageViews[j], NULL);
            }
            free(swapChainImageViews);
            exit(EXIT_FAILURE);
        }
    }

    free(swapChainImages);
    return swapChainImageViews;
}

// Create Framebuffers
VkFramebuffer* createFramebuffers(
    VkDevice device,
    VkRenderPass renderPass,
    VkImageView* imageViews,
    uint32_t imageCount,
    VkExtent2D extent
) {
    
    VkFramebuffer* framebuffers = malloc(sizeof(VkFramebuffer) * imageCount);
    if (framebuffers == NULL) {
        printf("Failed to allocate memory for framebuffers\n");
        exit(EXIT_FAILURE);
    }

    for (uint32_t i = 0; i < imageCount; i++) {
        VkImageView attachments[] = { imageViews[i] };

        VkFramebufferCreateInfo framebufferInfo = {
            .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass      = renderPass,
            .attachmentCount = 1,
            .pAttachments    = attachments,
            .width           = extent.width,
            .height          = extent.height,
            .layers          = 1
        };

        if (vkCreateFramebuffer(device, &framebufferInfo, NULL, &framebuffers[i]) != VK_SUCCESS) {
            printf("Failed to create framebuffer %u\n", i);
            for (uint32_t j = 0; j < i; j++) {
                vkDestroyFramebuffer(device, framebuffers[j], NULL);
            }
            free(framebuffers);
            exit(EXIT_FAILURE);
        }
    }

    return framebuffers;
}

// Create Uniform Buffer
VkBuffer createUniformBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceMemory* bufferMemory) {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    // Create the buffer
    VkBufferCreateInfo bufferInfo = {
        .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size        = bufferSize,
        .usage       = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    VkBuffer uniformBuffer;
    if (vkCreateBuffer(device, &bufferInfo, NULL, &uniformBuffer) != VK_SUCCESS) {
        printf("Failed to create uniform buffer!\n");
        exit(EXIT_FAILURE);
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, uniformBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize  = memRequirements.size,
        .memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
    };

    if (vkAllocateMemory(device, &allocInfo, NULL, bufferMemory) != VK_SUCCESS) {
        printf("Failed to allocate uniform buffer memory!\n");
        exit(EXIT_FAILURE);
    }

    vkBindBufferMemory(device, uniformBuffer, *bufferMemory, 0);

    return uniformBuffer;
}

// Create Descriptor Pool
VkDescriptorPool createDescriptorPool(VkDevice device) {

    VkDescriptorPoolCreateInfo pool_info = {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .poolSizeCount = 1,
        .pPoolSizes    = &(VkDescriptorPoolSize) {
            .type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1
        },
        .maxSets       = 1
    };

    VkDescriptorPool descriptorPool;
    if (vkCreateDescriptorPool(device, &pool_info, NULL, &descriptorPool) != VK_SUCCESS) {
        printf("Failed to create descriptor pool\n");
        exit(EXIT_FAILURE);
    }

    return descriptorPool;
}

// Create Descriptor Set
VkDescriptorSet createDescriptorSet(
    VkDevice device,
    VkDescriptorPool pool,
    VkDescriptorSetLayout layout,
    VkBuffer uniformBuffer
) {
    VkDescriptorSetAllocateInfo allocInfo = {
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool     = pool,
        .descriptorSetCount = 1,
        .pSetLayouts        = &layout
    };

    VkDescriptorSet descriptorSet;
    if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
        printf("Failed to allocate descriptor set\n");
        exit(EXIT_FAILURE);
    }

    VkWriteDescriptorSet descriptor_write = {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet          = descriptorSet,
        .dstBinding      = 0, // Must match binding in shader
        .dstArrayElement = 0,
        .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .pBufferInfo     = &(VkDescriptorBufferInfo) {
            .buffer = uniformBuffer,
            .offset = 0,
            .range  = sizeof(UniformBufferObject)
        }
    };

    vkUpdateDescriptorSets(device, 1, &descriptor_write, 0, NULL);

    return descriptorSet;
}

// Create Command Pool
VkCommandPool createCommandPool(VkDevice device, uint32_t graphicsFamily) {
    VkCommandPoolCreateInfo poolInfo = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = graphicsFamily,
        .flags            = 0 // Optional flags
    };

    VkCommandPool commandPool;
    if (vkCreateCommandPool(device, &poolInfo, NULL, &commandPool) != VK_SUCCESS) {
        printf("Failed to create command pool\n");
        exit(EXIT_FAILURE);
    }

    return commandPool;
}

// Create Instance Buffer
VkBuffer createInstanceBuffer(
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    InstanceData* instances,
    size_t instanceCount,
    VkDeviceMemory* bufferMemory
) {

    // Create the buffer
    VkBufferCreateInfo bufferInfo = {
        .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size        = instanceCount * sizeof(InstanceData),
        .usage       = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    VkBuffer instanceBuffer;
    if (vkCreateBuffer(device, &bufferInfo, NULL, &instanceBuffer) != VK_SUCCESS) {
        printf("Failed to create instance buffer!\n");
        exit(EXIT_FAILURE);
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, instanceBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize  = memRequirements.size,
        .memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
    };

    if (vkAllocateMemory(device, &allocInfo, NULL, bufferMemory) != VK_SUCCESS) {
        printf("Failed to allocate instance buffer memory!\n");
        exit(EXIT_FAILURE);
    }

    vkBindBufferMemory(device, instanceBuffer, *bufferMemory, 0);

    // Map memory and copy instance data
    void* data;
    vkMapMemory(device, *bufferMemory, 0, bufferInfo.size, 0, &data);
    memcpy(data, instances, bufferInfo.size);
    vkUnmapMemory(device, *bufferMemory);

    return instanceBuffer;
}

// Create Command Buffers
VkCommandBuffer* createCommandBuffers(
    VkDevice device,
    VkCommandPool commandPool,
    VkPipeline graphicsPipeline,
    VkPipelineLayout pipelineLayout,
    VkRenderPass renderPass,
    VkFramebuffer* framebuffers,
    uint32_t imageCount,
    VkBuffer instanceBuffer,
    VkDescriptorSet descriptorSet,
    VkExtent2D swapChainExtent
) {
    VkCommandBufferAllocateInfo allocInfo = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = commandPool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = imageCount
    };

    VkCommandBuffer* commandBuffers = malloc(sizeof(VkCommandBuffer) * imageCount);
    if (commandBuffers == NULL) {
        printf("Failed to allocate memory for command buffers\n");
        exit(EXIT_FAILURE);
    }

    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers) != VK_SUCCESS) {
        printf("Failed to allocate command buffers\n");
        free(commandBuffers);
        exit(EXIT_FAILURE);
    }

    for (uint32_t i = 0; i < imageCount; i++) {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
            printf("Failed to begin recording command buffer %u\n", i);
            exit(EXIT_FAILURE);
        }

        VkRenderPassBeginInfo renderPassInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = renderPass,
            .framebuffer = framebuffers[i],
            .renderArea.offset = (VkOffset2D){0, 0},
            .renderArea.extent = swapChainExtent,
            .clearValueCount = 1,
            .pClearValues = &(VkClearValue) { .color = {{0.3f, 0.0f, 0.0f, 1.0f}} }
        };
            
        vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
        vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);
        vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, (VkBuffer[]){instanceBuffer}, (VkDeviceSize[]){0});
        vkCmdDraw(commandBuffers[i], 4, INDICES_COUNT, 0, 0); // Adjust vertex count as needed
        vkCmdEndRenderPass(commandBuffers[i]);
        if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
            printf("Failed to record command buffer %u\n", i);
            exit(EXIT_FAILURE);
        }
    }

    return commandBuffers;
}

// Create Semaphore
VkSemaphore createSemaphore(VkDevice device) {
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkSemaphore semaphore;
    if (vkCreateSemaphore(device, &semaphoreInfo, NULL, &semaphore) != VK_SUCCESS) {
        printf("Failed to create semaphore\n");
        exit(EXIT_FAILURE);
    }

    return semaphore;
}

// Create Fence
VkFence createFence(VkDevice device) {
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkFence fence;
    if (vkCreateFence(device, &fenceInfo, NULL, &fence) != VK_SUCCESS) {
        printf("Failed to create fence\n");
        exit(EXIT_FAILURE);
    }

    return fence;
}

// gui.c

void mainLoop(
    VkDevice device,
    VkQueues queues,
    VkSwapchainKHR swapChain,
    VkSemaphore imageAvailableSemaphore,
    VkSemaphore renderFinishedSemaphore,
    VkFence inFlightFence,
    VkCommandBuffer* commandBuffers,
    uint32_t imageCount,
    VkDescriptorSet descriptorSet,
    VkExtent2D swapChainExtent,
    VkBuffer uniformBuffer,
    VkDeviceMemory uniformBufferMemory // Added parameter
) 
{
    #if defined(USE_SDL2)
        int running = 1;
        SDL_Event event;
        while (running) {
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    running = 0;
                }
            }

            // Update Uniform Buffer Data
            UniformBufferObject ubo = {};
            ubo.targetWidth = (float)swapChainExtent.width;
            ubo.targetHeight = (float)swapChainExtent.height;

            // Map memory and copy data
            void* data;
            vkMapMemory(device, uniformBufferMemory, 0, sizeof(ubo), 0, &data); // Use uniformBufferMemory
            memcpy(data, &ubo, sizeof(ubo));
            vkUnmapMemory(device, uniformBufferMemory); // Use uniformBufferMemory

            // Wait for previous frame
            vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
            vkResetFences(device, 1, &inFlightFence);

            // Acquire next image
            uint32_t imageIndex;
            VkResult acquireResult = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
            if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR || acquireResult == VK_SUBOPTIMAL_KHR){
                printf("Swapchain out of date or suboptimal. Consider recreating swapchain.\n");
                running = 0;
                continue;
            }
            else if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR){
                printf("Failed to acquire swapchain image\n");
                running = 0;
                continue;
            }

            // Submit command buffer
            VkSubmitInfo submitInfo = {
                .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                .waitSemaphoreCount   = 1,
                .pWaitSemaphores      = (VkSemaphore[]){ imageAvailableSemaphore },
                .pWaitDstStageMask    = (VkPipelineStageFlags[]){ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT },
                .commandBufferCount   = 1,
                .pCommandBuffers      = &commandBuffers[imageIndex],
                .signalSemaphoreCount = 1,
                .pSignalSemaphores    = (VkSemaphore[]){ renderFinishedSemaphore }
            };

            if (vkQueueSubmit(queues.graphics, 1, &submitInfo, inFlightFence) != VK_SUCCESS) {
                printf("Failed to submit draw command buffer\n");
                running = 0;
                continue;
            }

            // Present the image
            VkPresentInfoKHR presentInfo = {
                .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                .waitSemaphoreCount = 1,
                .pWaitSemaphores    = (VkSemaphore[]){ renderFinishedSemaphore },
                .swapchainCount     = 1,
                .pSwapchains        = (VkSwapchainKHR[]){ swapChain },
                .pImageIndices      = &imageIndex  
            };

            VkResult presentResult = vkQueuePresentKHR(queues.present, &presentInfo);
            if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR) {
                printf("Swapchain out of date or suboptimal during present. Consider recreating swapchain.\n");
                running = 0;
                continue;
            }
            else if (presentResult != VK_SUCCESS) {
                printf("Failed to present swapchain image\n");
                running = 0;
                continue;
            }

            // Optionally, you can add vkQueueWaitIdle here for simplicity
            vkQueueWaitIdle(queues.present);
        }
    #else
        printf("SDL has to be included and it has to be included\n");
        exit(EXIT_FAILURE);
    #endif
}


typedef struct {

// STATIC
    void*                       window_p;
    VkInstance                  instance;
    VkDebugUtilsMessengerEXT    debug_messanger;
    VkSurfaceKHR                surface;
    VkPhysicalDevice            physical_device;
    VkDevice                    device;
    VkQueueFamilyIndices        queue_family_indices;
    VkQueues                    queues;

// DYNAMIC
    VkRenderPass                render_pass;
    struct {
        VkSwapchainKHR          swap_chain;
        VkFormat                image_format;
        unsigned int            image_count;
        VkExtent2D              extent;
        VkImageView*            image_views_p;
        VkFramebuffer*          frame_buffers_p;
    }                           swap_chain;
    


} VK;

void test() {
    void* window = createWindow(800, 600, "Vulkan GUI");
    VkInstance instance = createVulkanInstance(window);
    VkDebugUtilsMessengerEXT debugMessenger = setupDebugMessenger(instance);
    VkSurfaceKHR surface = createSurface(window, instance);
    VkPhysicalDevice physicalDevice = getPhysicalDevice(instance);
    VkQueueFamilyIndices queueFamilyIndices = getQueueFamilyIndices(surface, physicalDevice);
    VkDevice device = getDevice(physicalDevice, queueFamilyIndices);
    VkQueues queues = getQueues(device, queueFamilyIndices);
    VkDescriptorSetLayout descriptorSetLayout = createDescriptorSetLayout(device);
    VkPipelineLayout pipelineLayout = createPipelineLayout(device, descriptorSetLayout);
    VkFormat swapChainImageFormat = VK_FORMAT_B8G8R8A8_SRGB;
    VkExtent2D swapChainExtent = {800, 600};
    VkSwapchainKHR swapChain = createSwapChain(
        device,
        physicalDevice,
        surface,
        queueFamilyIndices,
        swapChainExtent,
        &swapChainImageFormat
    );
    uint32_t swapChainImageCount = 0;
    vkGetSwapchainImagesKHR(device, swapChain, &swapChainImageCount, NULL);
    VkImageView* swapChainImageViews = createImageViews(
        device,
        swapChain,
        swapChainImageFormat,
        swapChainImageCount
    );
    VkRenderPass renderPass = createRenderPass(device, swapChainImageFormat);
    VkFramebuffer* swapChainFramebuffers = createFramebuffers(
        device,
        renderPass,
        swapChainImageViews,
        swapChainImageCount,
        swapChainExtent
    );
    VkPipeline graphicsPipeline = createGraphicsPipeline(
        device,
        pipelineLayout,
        renderPass,
        swapChainExtent
    );
    VkDeviceMemory uniformBufferMemory;
    VkBuffer uniformBuffer = createUniformBuffer(device, physicalDevice, &uniformBufferMemory);
    VkDescriptorPool descriptorPool = createDescriptorPool(device);
    VkDescriptorSet descriptorSet = createDescriptorSet(
        device,
        descriptorPool,
        descriptorSetLayout,
        uniformBuffer
    );
    VkCommandPool commandPool = createCommandPool(device, queueFamilyIndices.graphics);
    VkDeviceMemory instanceBufferMemory;
    VkBuffer instanceBuffer = createInstanceBuffer(
        device,
        physicalDevice,
        (InstanceData*)all_instances,
        ALL_INSTANCE_COUNT,
        &instanceBufferMemory
    );
    VkCommandBuffer* commandBuffers = createCommandBuffers(
        device,
        commandPool,
        graphicsPipeline,
        pipelineLayout,
        renderPass,
        swapChainFramebuffers,
        swapChainImageCount,
        instanceBuffer,
        descriptorSet,
        swapChainExtent
    );
    VkSemaphore imageAvailableSemaphore = createSemaphore(device);
    VkSemaphore renderFinishedSemaphore = createSemaphore(device);
    VkFence inFlightFence = createFence(device);
    mainLoop(
        device,
        queues,
        swapChain,
        imageAvailableSemaphore, 
        renderFinishedSemaphore,
        inFlightFence,
        commandBuffers,
        swapChainImageCount,
        descriptorSet,
        swapChainExtent,
        uniformBuffer,
        uniformBufferMemory
    );
    cleanup(
        device,
        instance,
        surface,
        debugMessenger,
        graphicsPipeline,
        pipelineLayout,
        renderPass,
        descriptorSetLayout,
        descriptorPool,
        uniformBuffer,
        uniformBufferMemory,
        instanceBuffer,
        instanceBufferMemory,
        descriptorSet,
        swapChain,
        swapChainImageViews,
        swapChainImageCount,
        swapChainFramebuffers,
        commandPool,
        commandBuffers,
        imageAvailableSemaphore,
        renderFinishedSemaphore,
        inFlightFence,
        window
    );
}
