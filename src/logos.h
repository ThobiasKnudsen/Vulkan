#define VK_USE_PLATFORM_XLIB_KHR
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <shaderc/shaderc.h>
#include <math.h>
#include "debug.h"

// Instance structure (packed to minimize size)
typedef struct {
    float pos[2];       // middle_x, middle_y
    float size[2];      // width, height
    float rotation;     // rotation angle
    float corner_radius;// pixels
    uint32_t color;     // background color packed as RGBA8
    uint32_t tex_info;  // texture ID and other info
    float tex_rect[4];  // texture rectangle (u, v, width, height)

} InstanceData;

typedef struct {
    float targetWidth;
    float targetHeight;
    // Padding to align to 16 bytes
    float padding[2];
} UniformBufferObject;

// Instances array
const InstanceData instances[] = {
    // Instance 1
    {
        .pos = {0.0f, 0.0f},
        .size = {100.0f, 100.0f},
        .rotation = 0.0f,
        .corner_radius = 10.f,
        .color = 0xFF0000FF, // Red color in RGBA8
        .tex_info = 0,       // Texture ID 0 (no texture)
        .tex_rect = {0.0f, 0.0f, 1.0f, 1.0f}
    },
    // Instance 2
    {
        .pos = {100.f, 0.0f},
        .size = {100.0f, 100.0f},
        .rotation = 45.0f,   // 45 degrees rotation
        .corner_radius = 10.f,
        .color = 0x00FF00FF, // Green color in RGBA8
        .tex_info = 0,
        .tex_rect = {0.0f, 0.0f, 1.0f, 1.0f}
    },
    // Add more instances as needed
};

size_t readShaderSource(const char* filename, char** buffer) {
    FILE* file = fopen(filename, "r");
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
    return fileSize;
}

// Function to compile GLSL shader to SPIR-V and create a shader module
VkShaderModule createShaderModule(const char* filename, shaderc_shader_kind kind, VkDevice device) {
    char* sourceCode = NULL;
    size_t sourceSize = readShaderSource(filename, &sourceCode);
    if (sourceSize == 0 || sourceCode == NULL) {
        printf("Failed to read shader source '%s'\n", filename);
        exit(EXIT_FAILURE);
    }

    shaderc_compiler_t compiler = shaderc_compiler_initialize();
    shaderc_compile_options_t options = shaderc_compile_options_initialize();

    // Optional: Set compiler options, e.g., optimization level
    shaderc_compile_options_set_optimization_level(options, shaderc_optimization_level_performance);

    shaderc_compilation_result_t result = shaderc_compile_into_spv(
        compiler, sourceCode, sourceSize, kind, filename, "main", options);

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

uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for(uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if( (typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties ){
            return i;
        }
    }

    ("Failed to find suitable memory type!");
}

// Main function with all steps merged
void test() {

    SDL_Window*                 window                      = NULL;
    VkInstance                  instance                    = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT    debugMessenger              = VK_NULL_HANDLE;
    VkSurfaceKHR                surface                     = VK_NULL_HANDLE;
    VkPhysicalDevice            physicalDevice              = VK_NULL_HANDLE;
    uint32_t                    graphicsQueueFamilyIndex    = UINT32_MAX;
    uint32_t                    presentQueueFamilyIndex     = UINT32_MAX;
    VkDevice                    device                      = VK_NULL_HANDLE;
    VkQueue                     graphicsQueue               = VK_NULL_HANDLE;
    VkQueue                     presentQueue                = VK_NULL_HANDLE;
    VkPipelineLayout            pipelineLayout              = VK_NULL_HANDLE;
    VkRenderPass                renderPass                  = VK_NULL_HANDLE;
    VkPipeline                  graphicsPipelineHandle      = VK_NULL_HANDLE;
    VkSwapchainKHR              swapChain                   = VK_NULL_HANDLE;
    VkFormat                    swapChainImageFormat        = VK_FORMAT_B8G8R8A8_SRGB;
    VkExtent2D                  swapChainExtent             = {800, 600};
    uint32_t                    swapChainImageCount         = 0;
    VkImage*                    swapChainImages             = NULL;
    VkImageView*                swapChainImageViews         = NULL;
    VkFramebuffer*              swapChainFramebuffers       = NULL;
    VkCommandPool               commandPool                 = VK_NULL_HANDLE;
    VkCommandBuffer*            commandBuffers              = NULL;
    VkBuffer                    instanceBuffer              = VK_NULL_HANDLE;
    VkDeviceMemory              instanceBufferMemory        = VK_NULL_HANDLE;
    VkBuffer                    uniformBuffer               = VK_NULL_HANDLE;
    VkDeviceMemory              uniformBufferMemory         = VK_NULL_HANDLE;
    VkDescriptorSetLayout       descriptorSetLayout         = {};
    VkDescriptorPool            descriptorPool              = VK_NULL_HANDLE;
    VkDescriptorSet             descriptorSet               = VK_NULL_HANDLE;
    VkSemaphore                 imageAvailableSemaphore     = VK_NULL_HANDLE;
    VkSemaphore                 renderFinishedSemaphore     = VK_NULL_HANDLE;
    VkFence                     inFlightFence               = VK_NULL_HANDLE;

    // Initialize SDL
    {
        if (SDL_Init(SDL_INIT_VIDEO) != 0) {
            printf( "ERROR: SDL_Init: %s\n", SDL_GetError());
            exit(EXIT_FAILURE);
        }
        window = SDL_CreateWindow(
            "Logos Vulkan Window",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            800,
            600,
            SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN
        );
        if (!window) {
            printf( "ERROR: SDL_CreateWindow: %s\n", SDL_GetError());
            SDL_Quit();
            exit(EXIT_FAILURE);
        }
    }

    // Create Vulkan instance with debugging
    {
        #define VK_DEBUG_CALLBACK                                               \
            VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(                       \
                VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,         \
                VkDebugUtilsMessageTypeFlagsEXT messageTypes,                   \
                const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,      \
                void* pUserData)                                                \
            {                                                                   \
                fprintf(stderr, "Vulkan Debug: %s\n", pCallbackData->pMessage); \
                return VK_FALSE;                                                \
            }
        VK_DEBUG_CALLBACK

        VkApplicationInfo appInfo = {0};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Logos Application";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "Logos Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_2;

        uint32_t sdlExtensionCount = 0;
        if (!SDL_Vulkan_GetInstanceExtensions(window, &sdlExtensionCount, NULL)) {
            printf("Failed to get SDL Vulkan instance extensions: %s\n", SDL_GetError());
            goto cleanup;
        }

        const char** sdlExtensions = (const char**)malloc(sizeof(const char*) * sdlExtensionCount);
        if (sdlExtensions == NULL) {
            printf("Failed to allocate memory for SDL Vulkan extensions\n");
            goto cleanup;
        }

        if (!SDL_Vulkan_GetInstanceExtensions(window, &sdlExtensionCount, sdlExtensions)) {
            printf("Failed to get SDL Vulkan instance extensions: %s\n", SDL_GetError());
            free(sdlExtensions);
            goto cleanup;
        }

        // Optional: Add VK_EXT_debug_utils_EXTENSION_NAME for debugging
        const char* additionalExtensions[] = { VK_EXT_DEBUG_UTILS_EXTENSION_NAME };
        uint32_t totalExtensionCount = sdlExtensionCount + 1;
        const char** allExtensions = (const char**)malloc(sizeof(const char*) * totalExtensionCount);
        if (allExtensions == NULL) {
            printf("Failed to allocate memory for all Vulkan extensions\n");
            free(sdlExtensions);
            goto cleanup;
        }
        memcpy(allExtensions, sdlExtensions, sizeof(const char*) * sdlExtensionCount);
        allExtensions[sdlExtensionCount] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

        const char* validationLayers[] = { "VK_LAYER_KHRONOS_validation" };
        uint32_t validationLayerCount = 1;

        VkInstanceCreateInfo instanceCreateInfo = {0};
        instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceCreateInfo.pApplicationInfo = &appInfo;
        instanceCreateInfo.enabledExtensionCount = totalExtensionCount;
        instanceCreateInfo.ppEnabledExtensionNames = allExtensions;
        instanceCreateInfo.enabledLayerCount = validationLayerCount;
        instanceCreateInfo.ppEnabledLayerNames = validationLayers;

        // Set up the debug messenger creation info
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {0};
        debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugCreateInfo.pfnUserCallback = debugCallback;
        debugCreateInfo.pUserData = NULL; // Optional

        // Chain the debug messenger to the instance creation
        instanceCreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;

        VkResult result = vkCreateInstance(&instanceCreateInfo, NULL, &instance);
        free(sdlExtensions);
        free(allExtensions);
        if (result != VK_SUCCESS) {
            printf("Failed to create Vulkan instance\n");
            goto cleanup;
        }

        // Function pointer for vkCreateDebugUtilsMessengerEXT
        PFN_vkCreateDebugUtilsMessengerEXT funcCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (funcCreateDebugUtilsMessengerEXT != NULL) {
            if (funcCreateDebugUtilsMessengerEXT(instance, &debugCreateInfo, NULL, &debugMessenger) != VK_SUCCESS) {
                printf("Failed to set up debug messenger\n");
                goto cleanup;
            }
        } 
        else {
            printf("vkCreateDebugUtilsMessengerEXT not available\n");
        }
        #undef VK_DEBUG_CALLBACK
    }

    // Create Vulkan surface
    if (!SDL_Vulkan_CreateSurface(window, instance, &surface)) {
        printf( "Failed to create Vulkan surface: %s\n", SDL_GetError());
        goto cleanup;
    }

    // Select physical device
    {
        uint32_t deviceCount = 0;
        VkResult result = vkEnumeratePhysicalDevices(instance, &deviceCount, NULL);
        if (result != VK_SUCCESS || deviceCount == 0) {
            printf( "Failed to find GPUs with Vulkan support\n");
            vkDestroySurfaceKHR(instance, surface, NULL);
            vkDestroyInstance(instance, NULL);
            SDL_DestroyWindow(window);
            SDL_Quit();
            exit(EXIT_FAILURE);
        }

        VkPhysicalDevice* devices = malloc(sizeof(VkPhysicalDevice) * deviceCount);
        if (devices == NULL) {
            printf( "Failed to allocate memory for physical devices\n");
            goto cleanup;
        }
        result = vkEnumeratePhysicalDevices(instance, &deviceCount, devices);
        if (result != VK_SUCCESS) {
            printf( "Failed to enumerate physical devices\n");
            free(devices);
            goto cleanup;
        }
        physicalDevice = devices[0]; // Select the first suitable device
        free(devices);
    }

    // Find queue family indices
    {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, NULL);
        if (queueFamilyCount == 0) {
            printf( "Failed to find any queue families\n");
            goto cleanup;
        }
        VkQueueFamilyProperties* queueFamilies = malloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
        if (queueFamilies == NULL) {
            printf( "Failed to allocate memory for queue family properties\n");
            goto cleanup;
        }
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies);

        for (uint32_t i = 0; i < queueFamilyCount; i++) {
            if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                graphicsQueueFamilyIndex = i;
            }

            VkBool32 presentSupport = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);

            if (presentSupport == VK_TRUE) {
                presentQueueFamilyIndex = i;
            }

            if (graphicsQueueFamilyIndex != UINT32_MAX && presentQueueFamilyIndex != UINT32_MAX) {
                break;
            }
        }

        free(queueFamilies);

        if (graphicsQueueFamilyIndex == UINT32_MAX || presentQueueFamilyIndex == UINT32_MAX) {
            printf( "Failed to find suitable queue families\n");
            goto cleanup;
        }
    }

    // Create logical device and retrieve queues
    {
        VkResult result;

        float queuePriority = 1.0f;
        VkDeviceQueueCreateInfo queueCreateInfos[2];
        uint32_t queueCreateInfoCount = 0;

        if (graphicsQueueFamilyIndex == presentQueueFamilyIndex) {
            memset(&queueCreateInfos[0], 0, sizeof(VkDeviceQueueCreateInfo));
            queueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfos[0].pNext = NULL;
            queueCreateInfos[0].flags = 0;
            queueCreateInfos[0].queueFamilyIndex = graphicsQueueFamilyIndex;
            queueCreateInfos[0].queueCount = 1;
            queueCreateInfos[0].pQueuePriorities = &queuePriority;
            queueCreateInfoCount = 1;
        } else {
            memset(&queueCreateInfos[0], 0, sizeof(VkDeviceQueueCreateInfo));
            memset(&queueCreateInfos[1], 0, sizeof(VkDeviceQueueCreateInfo));

            // First queue for graphics
            queueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfos[0].pNext = NULL;
            queueCreateInfos[0].flags = 0;
            queueCreateInfos[0].queueFamilyIndex = graphicsQueueFamilyIndex;
            queueCreateInfos[0].queueCount = 1;
            queueCreateInfos[0].pQueuePriorities = &queuePriority;

            // Second queue for presentation
            queueCreateInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfos[1].pNext = NULL;
            queueCreateInfos[1].flags = 0;
            queueCreateInfos[1].queueFamilyIndex = presentQueueFamilyIndex;
            queueCreateInfos[1].queueCount = 1;
            queueCreateInfos[1].pQueuePriorities = &queuePriority;

            queueCreateInfoCount = 2;
        }

        const char* deviceExtensions[] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };
        uint32_t deviceExtensionCount = sizeof(deviceExtensions) / sizeof(deviceExtensions[0]);

        VkDeviceCreateInfo deviceCreateInfo = {0};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.pQueueCreateInfos = queueCreateInfos;
        deviceCreateInfo.queueCreateInfoCount = queueCreateInfoCount;
        deviceCreateInfo.enabledExtensionCount = deviceExtensionCount;
        deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;
        deviceCreateInfo.enabledLayerCount = 0;
        deviceCreateInfo.ppEnabledLayerNames = NULL;
        deviceCreateInfo.pNext = NULL; // Ensure pNext is NULL

        result = vkCreateDevice(physicalDevice, &deviceCreateInfo, NULL, &device);
        if (result != VK_SUCCESS) {
            printf( "Failed to create logical device\n");
            goto cleanup;
        }

        vkGetDeviceQueue(device, graphicsQueueFamilyIndex, 0, &graphicsQueue);
        vkGetDeviceQueue(device, presentQueueFamilyIndex, 0, &presentQueue);

        // desciptor set layout
        VkDescriptorSetLayoutBinding uboLayoutBinding = {};
        uboLayoutBinding.binding = 0; // Binding 0 in the shader
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; // Used in vertex shader
        uboLayoutBinding.pImmutableSamplers = NULL;

        VkDescriptorSetLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &uboLayoutBinding;

        if (vkCreateDescriptorSetLayout(device, &layoutInfo, NULL, &descriptorSetLayout) != VK_SUCCESS) {
            printf("Failed to create descriptor set layout\n");
            goto cleanup;
        }

        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1; // One descriptor set layout
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges = NULL;

        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL, &pipelineLayout) != VK_SUCCESS) {
            printf("Failed to create pipeline layout\n");
            goto cleanup;
        }

        // Create Render Pass
        VkAttachmentDescription colorAttachment = {0};
        colorAttachment.format = VK_FORMAT_B8G8R8A8_SRGB;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef = {0};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {0};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkSubpassDependency dependency = {0};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo = {0};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(device, &renderPassInfo, NULL, &renderPass) != VK_SUCCESS) {
            printf( "Failed to create render pass\n");
            goto cleanup;
        }

        // Create shader modules
        VkShaderModule vertShaderModule = createShaderModule("shaders/shader.vert.glsl", shaderc_glsl_vertex_shader, device);
        VkShaderModule fragShaderModule = createShaderModule("shaders/shader.frag.glsl", shaderc_glsl_fragment_shader, device);

        // Create Graphics Pipeline
        VkPipeline graphicsPipeline = VK_NULL_HANDLE;
        {
            VkPipelineShaderStageCreateInfo vertShaderStageInfo = {0};
            vertShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            vertShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
            vertShaderStageInfo.module = vertShaderModule;
            vertShaderStageInfo.pName  = "main";

            VkPipelineShaderStageCreateInfo fragShaderStageInfo = {0};
            fragShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            fragShaderStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
            fragShaderStageInfo.module = fragShaderModule;
            fragShaderStageInfo.pName  = "main";

            VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

            // Vertex input bindings and attributes for instance data
            VkVertexInputBindingDescription bindingDescriptions[1] = {0};
            bindingDescriptions[0].binding   = 0;
            bindingDescriptions[0].stride    = sizeof(InstanceData);
            bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

            VkVertexInputAttributeDescription attributeDescriptions[6] = {0};

            // Instance data attributes
            // inPos (vec2)
            attributeDescriptions[0].binding  = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format   = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[0].offset   = offsetof(InstanceData, pos);

            // inSize (vec2)
            attributeDescriptions[1].binding  = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format   = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[1].offset   = offsetof(InstanceData, size);

            // inRotation (float)
            attributeDescriptions[2].binding  = 0;
            attributeDescriptions[2].location = 2;
            attributeDescriptions[2].format   = VK_FORMAT_R32_SFLOAT;
            attributeDescriptions[2].offset   = offsetof(InstanceData, rotation);

            // inCornerRadius (float)
            attributeDescriptions[3].binding  = 0;
            attributeDescriptions[3].location = 3;
            attributeDescriptions[3].format   = VK_FORMAT_R32_SFLOAT;
            attributeDescriptions[3].offset   = offsetof(InstanceData, corner_radius);

            // inColor (uint)
            attributeDescriptions[4].binding  = 0;
            attributeDescriptions[4].location = 4;
            attributeDescriptions[4].format   = VK_FORMAT_R8G8B8A8_UNORM;
            attributeDescriptions[4].offset   = offsetof(InstanceData, color);

            // inTexInfo (uint)
            attributeDescriptions[5].binding  = 0;
            attributeDescriptions[5].location = 5;
            attributeDescriptions[5].format   = VK_FORMAT_R32_UINT;
            attributeDescriptions[5].offset   = offsetof(InstanceData, tex_info);

            // inTexRect (vec4)
            attributeDescriptions[6].binding  = 0;
            attributeDescriptions[6].location = 6;
            attributeDescriptions[6].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
            attributeDescriptions[6].offset   = offsetof(InstanceData, tex_rect);

            VkPipelineVertexInputStateCreateInfo vertexInputInfo = {0};
            vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertexInputInfo.vertexBindingDescriptionCount   = 1;
            vertexInputInfo.pVertexBindingDescriptions      = bindingDescriptions;
            vertexInputInfo.vertexAttributeDescriptionCount = 7;
            vertexInputInfo.pVertexAttributeDescriptions    = attributeDescriptions;

            VkPipelineInputAssemblyStateCreateInfo inputAssembly = {0};
            inputAssembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            inputAssembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
            inputAssembly.primitiveRestartEnable = VK_FALSE;

            VkViewport viewport = {0};
            viewport.x        = 0.0f;
            viewport.y        = 0.0f;
            viewport.width    = (float)swapChainExtent.width;
            viewport.height   = (float)swapChainExtent.height;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;

            VkRect2D scissor = {0};
            scissor.offset = (VkOffset2D){0, 0};
            scissor.extent = swapChainExtent;

            VkPipelineViewportStateCreateInfo viewportState = {0};
            viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewportState.viewportCount = 1;
            viewportState.pViewports    = &viewport;
            viewportState.scissorCount  = 1;
            viewportState.pScissors     = &scissor;

            VkPipelineRasterizationStateCreateInfo rasterizer = {0};
            rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rasterizer.depthClampEnable        = VK_FALSE;
            rasterizer.rasterizerDiscardEnable = VK_FALSE;
            rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
            rasterizer.lineWidth               = 1.0f;
            rasterizer.cullMode                = VK_CULL_MODE_BACK_BIT;
            rasterizer.frontFace               = VK_FRONT_FACE_CLOCKWISE;
            rasterizer.depthBiasEnable         = VK_FALSE;

            VkPipelineMultisampleStateCreateInfo multisampling = {0};
            multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            multisampling.sampleShadingEnable  = VK_FALSE;
            multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

            VkPipelineColorBlendAttachmentState colorBlendAttachment = {0};
            colorBlendAttachment.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            colorBlendAttachment.blendEnable         = VK_FALSE;

            VkPipelineColorBlendStateCreateInfo colorBlending = {0};
            colorBlending.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            colorBlending.logicOpEnable     = VK_FALSE;
            colorBlending.logicOp           = VK_LOGIC_OP_COPY;
            colorBlending.attachmentCount   = 1;
            colorBlending.pAttachments      = &colorBlendAttachment;
            colorBlending.blendConstants[0] = 0.0f;
            colorBlending.blendConstants[1] = 0.0f;
            colorBlending.blendConstants[2] = 0.0f;
            colorBlending.blendConstants[3] = 0.0f;

            VkGraphicsPipelineCreateInfo pipelineInfo = {0};
            pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            pipelineInfo.stageCount          = 2;
            pipelineInfo.pStages             = shaderStages;
            pipelineInfo.pVertexInputState   = &vertexInputInfo;
            pipelineInfo.pInputAssemblyState = &inputAssembly;
            pipelineInfo.pViewportState      = &viewportState;
            pipelineInfo.pRasterizationState = &rasterizer;
            pipelineInfo.pMultisampleState   = &multisampling;
            pipelineInfo.pDepthStencilState  = NULL;
            pipelineInfo.pColorBlendState    = &colorBlending;
            pipelineInfo.pDynamicState       = NULL;
            pipelineInfo.layout              = pipelineLayout;
            pipelineInfo.renderPass          = renderPass;
            pipelineInfo.subpass             = 0;
            pipelineInfo.basePipelineHandle  = VK_NULL_HANDLE;
            pipelineInfo.basePipelineIndex   = -1;

            if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &graphicsPipeline) != VK_SUCCESS) {
                printf( "Failed to create graphics pipeline\n");
                goto cleanup;
            }

            graphicsPipelineHandle = graphicsPipeline;

            // Clean up shader modules as they're no longer needed after pipeline creation
            vkDestroyShaderModule(device, vertShaderModule, NULL);
            vkDestroyShaderModule(device, fragShaderModule, NULL);
        }
    }

    // Create Swapchain
    {
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities);
        if (result != VK_SUCCESS) {
            printf( "Failed to get surface capabilities\n");
            goto cleanup;
        }

        if (surfaceCapabilities.currentExtent.width != UINT32_MAX) {
            swapChainExtent = surfaceCapabilities.currentExtent;
        } else {
            swapChainExtent.width = 800;
            swapChainExtent.height = 600;
        }

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, NULL);
        if (formatCount == 0) {
            printf( "Failed to find surface formats\n");
            goto cleanup;
        }
        VkSurfaceFormatKHR* formats = malloc(sizeof(VkSurfaceFormatKHR) * formatCount);
        if (formats == NULL) {
            printf( "Failed to allocate memory for surface formats\n");
            goto cleanup;
        }
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats);
        swapChainImageFormat = formats[0].format; // Choose the first format
        free(formats);

        VkSwapchainCreateInfoKHR swapchainCreateInfo = {0};
        swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainCreateInfo.surface = surface;
        swapchainCreateInfo.minImageCount = surfaceCapabilities.minImageCount + 1;
        if (surfaceCapabilities.maxImageCount > 0 && swapchainCreateInfo.minImageCount > surfaceCapabilities.maxImageCount) {
            swapchainCreateInfo.minImageCount = surfaceCapabilities.maxImageCount;
        }
        swapchainCreateInfo.imageFormat = swapChainImageFormat;
        swapchainCreateInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        swapchainCreateInfo.imageExtent = swapChainExtent;
        swapchainCreateInfo.imageArrayLayers = 1;
        swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        if (graphicsQueueFamilyIndex != presentQueueFamilyIndex) {
            uint32_t queueFamilyIndices[] = { graphicsQueueFamilyIndex, presentQueueFamilyIndex };
            swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            swapchainCreateInfo.queueFamilyIndexCount = 2;
            swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            swapchainCreateInfo.queueFamilyIndexCount = 0; // Optional
            swapchainCreateInfo.pQueueFamilyIndices = NULL; // Optional
        }
        swapchainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
        swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR; // Guaranteed to be available
        swapchainCreateInfo.clipped = VK_TRUE;
        swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

        result = vkCreateSwapchainKHR(device, &swapchainCreateInfo, NULL, &swapChain);
        if (result != VK_SUCCESS) {
            printf( "ERROR: Failed to create swapchain\n");
            goto cleanup;
        }
    }

    // Get swapchain images
    {
        VkResult result = vkGetSwapchainImagesKHR(device, swapChain, &swapChainImageCount, NULL);
        if (result != VK_SUCCESS || swapChainImageCount == 0) {
            printf( "Failed to get swapchain images\n");
            goto cleanup;
        }

        swapChainImages = malloc(sizeof(VkImage) * swapChainImageCount);
        if (swapChainImages == NULL) {
            printf( "Failed to allocate memory for swapchain images\n");
            goto cleanup;
        }

        result = vkGetSwapchainImagesKHR(device, swapChain, &swapChainImageCount, swapChainImages);
        if (result != VK_SUCCESS) {
            printf( "Failed to get swapchain images\n");
            goto cleanup;
        }
    }

    // Create image views
    {
        swapChainImageViews = malloc(sizeof(VkImageView) * swapChainImageCount);
        if (swapChainImageViews == NULL) {
            printf( "Failed to allocate memory for image views\n");
            goto cleanup;
        }

        for (uint32_t i = 0; i < swapChainImageCount; i++) {
            VkImageViewCreateInfo viewInfo = {0};
            viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image                           = swapChainImages[i];
            viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format                          = swapChainImageFormat;
            viewInfo.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel   = 0;
            viewInfo.subresourceRange.levelCount     = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount     = 1;

            if (vkCreateImageView(device, &viewInfo, NULL, &swapChainImageViews[i]) != VK_SUCCESS) {
                printf( "Failed to create image views\n");
                goto cleanup;
            }
        }
    }

    // Create framebuffers
    {
        swapChainFramebuffers = malloc(sizeof(VkFramebuffer) * swapChainImageCount);
        if (swapChainFramebuffers == NULL) {
            printf( "Failed to allocate memory for framebuffers\n");
            goto cleanup;
        }

        for (uint32_t i = 0; i < swapChainImageCount; i++) {
            VkImageView attachments[] = { swapChainImageViews[i] };

            VkFramebufferCreateInfo framebufferInfo = {0};
            framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass      = renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments    = attachments;
            framebufferInfo.width           = swapChainExtent.width;
            framebufferInfo.height          = swapChainExtent.height;
            framebufferInfo.layers          = 1;

            if (vkCreateFramebuffer(device, &framebufferInfo, NULL, &swapChainFramebuffers[i]) != VK_SUCCESS) {
                printf( "Failed to create framebuffer\n");
                goto cleanup;
            }
        }
    }

    // Create uniform buffer 
    {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        // Create the buffer
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = bufferSize;
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device, &bufferInfo, NULL, &uniformBuffer) != VK_SUCCESS) {
            printf("Failed to create uniform buffer!");
            goto cleanup;
        }

        // Get memory requirements
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, uniformBuffer, &memRequirements);

        // Allocate memory
        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(
            physicalDevice, memRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );

        if (vkAllocateMemory(device, &allocInfo, NULL, &uniformBufferMemory) != VK_SUCCESS) {
            printf("Failed to allocate uniform buffer memory!");
            goto cleanup;
        }

        // Bind buffer with memory
        vkBindBufferMemory(device, uniformBuffer, uniformBufferMemory, 0);
    }

    // Create Descriptor Pool
    {
        VkDescriptorPoolSize poolSize = {};
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = 1;

        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = 1;

        if (vkCreateDescriptorPool(device, &poolInfo, NULL, &descriptorPool) != VK_SUCCESS) {
            printf("Failed to create descriptor pool\n");
            goto cleanup;
        }
    }
    
    // Allocate Descriptor Set
    {
        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &descriptorSetLayout;

        if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
            printf("Failed to allocate descriptor set\n");
            goto cleanup;
        }

        // Update Descriptor Set
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = uniformBuffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkWriteDescriptorSet descriptorWrite = {};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSet;
        descriptorWrite.dstBinding = 0; // Must match binding in shader
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, NULL);
    }

    // Create command pool
    {
        VkCommandPoolCreateInfo poolInfo = {0};
        poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
        poolInfo.flags            = 0; // Optional flags

        if (vkCreateCommandPool(device, &poolInfo, NULL, &commandPool) != VK_SUCCESS) {
            printf( "Failed to create command pool\n");
            goto cleanup;
        }
    }

    // Create command buffers
    {
        VkCommandBufferAllocateInfo allocInfo = {0};
        allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool        = commandPool;
        allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = swapChainImageCount;

        commandBuffers = malloc(sizeof(VkCommandBuffer) * swapChainImageCount);
        if (commandBuffers == NULL) {
            printf( "Failed to allocate command buffer handles\n");
            goto cleanup;
        }

        if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers) != VK_SUCCESS) {
            printf( "Failed to allocate command buffers\n");
            goto cleanup;
        }
    }

    // Create instance buffer
    {
        VkDeviceSize bufferSize = sizeof(instances);

        VkBufferCreateInfo bufferInfo = {0};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = bufferSize;
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device, &bufferInfo, NULL, &instanceBuffer) != VK_SUCCESS) {
            printf("Failed to create instance buffer\n");
            goto cleanup;
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, instanceBuffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo = {0};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;

        VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
        allocInfo.memoryTypeIndex = findMemoryType(
            physicalDevice, memRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );

        if (vkAllocateMemory(device, &allocInfo, NULL, &instanceBufferMemory) != VK_SUCCESS) {
            printf("Failed to allocate instance buffer memory\n");
            goto cleanup;
        }

        vkBindBufferMemory(device, instanceBuffer, instanceBufferMemory, 0);

        // Map memory and copy instance data
        void* data;
        vkMapMemory(device, instanceBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, instances, (size_t)bufferSize);
        vkUnmapMemory(device, instanceBufferMemory);
    }

    // Record command buffers
    {
        for (uint32_t i = 0; i < swapChainImageCount; i++) {
            VkCommandBufferBeginInfo beginInfo = {0};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

            if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
                printf( "Failed to begin recording command buffer\n");
                goto cleanup;
            }

            VkRenderPassBeginInfo renderPassInfo = {0};
            renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass        = renderPass;
            renderPassInfo.framebuffer       = swapChainFramebuffers[i];
            renderPassInfo.renderArea.offset = (VkOffset2D){0, 0};
            renderPassInfo.renderArea.extent = swapChainExtent;

            VkClearValue clearColor = { .color = {{0.0f, 0.0f, 0.0f, 1.0f}} };
            renderPassInfo.clearValueCount = 1;
            renderPassInfo.pClearValues    = &clearColor;

            vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelineHandle);

            vkCmdBindDescriptorSets(commandBuffers[i],VK_PIPELINE_BIND_POINT_GRAPHICS,pipelineLayout,0, 1, &descriptorSet, 0, NULL);

            VkBuffer buffers[] = { instanceBuffer };
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, buffers, offsets);

            uint32_t instanceCount = sizeof(instances) / sizeof(instances[0]);
            vkCmdDraw(commandBuffers[i], 4, instanceCount, 0, 0);

            vkCmdEndRenderPass(commandBuffers[i]);

            if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
                printf( "Failed to record command buffer\n");
                goto cleanup;
            }
        }
    }

    // Create synchronization objects
    {
        VkSemaphoreCreateInfo semaphoreInfo = {0};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo = {0};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        if (vkCreateSemaphore(device, &semaphoreInfo, NULL, &imageAvailableSemaphore) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, NULL, &renderFinishedSemaphore) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, NULL, &inFlightFence) != VK_SUCCESS)
        {
            printf( "Failed to create synchronization objects\n");
            goto cleanup;
        }
    }

    // Rendering loop
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
        vkMapMemory(device, uniformBufferMemory, 0, sizeof(ubo), 0, &data);
        memcpy(data, &ubo, sizeof(ubo));
        vkUnmapMemory(device, uniformBufferMemory);

        // Wait for previous frame
        vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
        vkResetFences(device, 1, &inFlightFence);

        // Acquire next image
        uint32_t imageIndex;
        VkResult acquireResult = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
        if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR || acquireResult == VK_SUBOPTIMAL_KHR)
        {
            // Handle swapchain recreation here if needed
            printf( "Swapchain out of date or suboptimal. Consider recreating swapchain.\n");
            running = 0;
            continue;
        }
        else if (acquireResult != VK_SUCCESS)
        {
            printf( "Failed to acquire swapchain image\n");
            running = 0;
            continue;
        }

        // Submit command buffer
        VkSubmitInfo submitInfo = {0};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        VkSemaphore waitSemaphores[] = { imageAvailableSemaphore };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores    = waitSemaphores;
        submitInfo.pWaitDstStageMask  = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers    = &commandBuffers[imageIndex];
        VkSemaphore signalSemaphores[] = { renderFinishedSemaphore };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores    = signalSemaphores;

        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence) != VK_SUCCESS)
        {
            printf( "Failed to submit draw command buffer\n");
            running = 0;
            continue;
        }

        // Present the image
        VkSwapchainKHR swapChains[] = { swapChain };
        VkPresentInfoKHR presentInfo = {0};
        presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores    = signalSemaphores;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains    = swapChains;
        presentInfo.pImageIndices  = &imageIndex;

        VkResult presentResult = vkQueuePresentKHR(presentQueue, &presentInfo);
        if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR)
        {
            // Handle swapchain recreation here if needed
            printf( "Swapchain out of date or suboptimal during present. Consider recreating swapchain.\n");
            goto cleanup;
        }
        else if (presentResult != VK_SUCCESS)
        {
            printf( "Failed to present swapchain image\n");
            goto cleanup;
        }

        // Optionally, you can add vkQueueWaitIdle here for simplicity
        vkQueueWaitIdle(presentQueue);
    }

    cleanup:
    {
        if (device != VK_NULL_HANDLE)
            vkDeviceWaitIdle(device);
        if (inFlightFence != VK_NULL_HANDLE) 
            vkDestroyFence(device, inFlightFence, NULL);
        if (renderFinishedSemaphore != VK_NULL_HANDLE) 
            vkDestroySemaphore(device, renderFinishedSemaphore, NULL);
        if (imageAvailableSemaphore != VK_NULL_HANDLE)
            vkDestroySemaphore(device, imageAvailableSemaphore, NULL);
        if (commandBuffers != NULL) {
            vkFreeCommandBuffers(device, commandPool, swapChainImageCount, commandBuffers);
            free(commandBuffers);
        }
        if (commandPool != VK_NULL_HANDLE) 
            vkDestroyCommandPool(device, commandPool, NULL);
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
        if (graphicsPipelineHandle != VK_NULL_HANDLE) 
            vkDestroyPipeline(device, graphicsPipelineHandle, NULL);
        if (renderPass != VK_NULL_HANDLE)
            vkDestroyRenderPass(device, renderPass, NULL);
        if (pipelineLayout != VK_NULL_HANDLE)
            vkDestroyPipelineLayout(device, pipelineLayout, NULL);
        if (instanceBuffer != VK_NULL_HANDLE) 
            vkDestroyBuffer(device, instanceBuffer, NULL);
        if (instanceBufferMemory != VK_NULL_HANDLE) 
            vkFreeMemory(device, instanceBufferMemory, NULL);
        if (descriptorPool != VK_NULL_HANDLE)
            vkDestroyDescriptorPool(device, descriptorPool, NULL);
        if (descriptorSetLayout != VK_NULL_HANDLE)
            vkDestroyDescriptorSetLayout(device, descriptorSetLayout, NULL);
        if (uniformBuffer != VK_NULL_HANDLE)
            vkDestroyBuffer(device, uniformBuffer, NULL);
        if (uniformBufferMemory != VK_NULL_HANDLE)
            vkFreeMemory(device, uniformBufferMemory, NULL);
        if (device != VK_NULL_HANDLE)
            vkDestroyDevice(device, NULL);
        if (surface != VK_NULL_HANDLE)
            vkDestroySurfaceKHR(instance, surface, NULL);
        if (debugMessenger != VK_NULL_HANDLE) {
            PFN_vkDestroyDebugUtilsMessengerEXT funcDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
            if (funcDestroyDebugUtilsMessengerEXT != NULL)
                funcDestroyDebugUtilsMessengerEXT(instance, debugMessenger, NULL);
        }
        if (instance != VK_NULL_HANDLE)
            vkDestroyInstance(instance, NULL);
        if (window != NULL)
            SDL_DestroyWindow(window);
        SDL_Quit();
    }
}
