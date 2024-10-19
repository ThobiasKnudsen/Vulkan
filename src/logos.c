// logos.c

#define VK_USE_PLATFORM_XLIB_KHR
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <math.h>

// Vertex structure
typedef struct {
    float position[2];
    float color[3];
} Vertex;

// Triangle vertices
const Vertex vertices[] = {
    // Positions        // Colors
    {{ 0.0f,  0.5f },  { 1.0f, 0.0f, 0.0f }}, // Bottom vertex (Red)
    {{ -0.5f,-0.5f },  { 0.0f, 0.0f, 1.0f }}, // Top-left vertex (Blue)
    {{ 0.5f, -0.5f },  { 0.0f, 1.0f, 0.0f }}, // Top-right vertex (Green)
};

// Transformation matrices (identity matrices for simplicity)
typedef struct {
    float model[16];
    float view[16];
    float proj[16];
} PushConstantsData;

// Initialize transformation matrices to identity
PushConstantsData pushConstantsData = {
    .model = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    },
    .view = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    },
    .proj = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f,-1.0f, 0.0f, 0.0f, // Note the -1.0f for Y to flip the coordinate system
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    }
};

// Global Vulkan handles
static VkInstance           logos_instance = VK_NULL_HANDLE;
static VkPhysicalDevice     logos_physical_device = VK_NULL_HANDLE;
static SDL_Window*          logos_window = NULL;
static VkSurfaceKHR         logos_surface = VK_NULL_HANDLE;

static VkDevice             logos_device = VK_NULL_HANDLE;
static VkSwapchainKHR       logos_swapChain = VK_NULL_HANDLE;
static VkFormat             logos_swapChainImageFormat;
static VkExtent2D           logos_swapChainExtent;

static VkPipeline           graphicsPipeline = VK_NULL_HANDLE;
static VkPipelineLayout     pipelineLayout = VK_NULL_HANDLE;
static VkRenderPass         renderPass = VK_NULL_HANDLE;
static VkQueue              graphicsQueue = VK_NULL_HANDLE;
static VkQueue              presentQueue = VK_NULL_HANDLE;
static uint32_t             graphicsQueueFamilyIndex = UINT32_MAX;
static uint32_t             presentQueueFamilyIndex = UINT32_MAX;

// Function to read a binary file (e.g., shader)
size_t Private_readFile(const char* filename, char** buffer)
{
    FILE* file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open file '%s'\n", filename);
        return 0;
    }

    fseek(file, 0, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    *buffer = (char*)malloc(fileSize);
    if (!*buffer) {
        fprintf(stderr, "Failed to allocate memory for file '%s'\n", filename);
        fclose(file);
        return 0;
    }

    size_t readSize = fread(*buffer, 1, fileSize, file);
    if (readSize != fileSize) {
        fprintf(stderr, "Failed to read file '%s'\n", filename);
        free(*buffer);
        fclose(file);
        return 0;
    }

    fclose(file);
    return fileSize;
}

// Function to create a shader module from a file
VkShaderModule Private_createShaderModule(const char* filename)
{
    char* code = NULL;
    size_t codeSize = Private_readFile(filename, &code);
    if (codeSize == 0 || code == NULL) {
        fprintf(stderr, "Failed to read shader file '%s'\n", filename);
        exit(EXIT_FAILURE);
    }

    if (codeSize % sizeof(uint32_t) != 0) {
        fprintf(stderr, "Shader file '%s' size is not a multiple of 4 bytes\n", filename);
        free(code);
        exit(EXIT_FAILURE);
    }

    VkShaderModuleCreateInfo createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = codeSize;
    createInfo.pCode = (uint32_t*)code;

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    if (vkCreateShaderModule(logos_device, &createInfo, NULL, &shaderModule) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create shader module from '%s'\n", filename);
        free(code);
        exit(EXIT_FAILURE);
    }

    free(code);
    return shaderModule;
}

// Function to find a suitable memory type
uint32_t Private_findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(logos_physical_device, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    fprintf(stderr, "Failed to find suitable memory type\n");
    return UINT32_MAX; // Unable to find suitable memory type
}

// Function to create a buffer and allocate memory
VkResult Private_createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer* buffer, VkDeviceMemory* bufferMemory)
{
    if (buffer == NULL || bufferMemory == NULL) {
        fprintf(stderr, "Invalid buffer or bufferMemory pointer\n");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    VkBufferCreateInfo bufferInfo = {0};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateBuffer(logos_device, &bufferInfo, NULL, buffer);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create buffer\n");
        return result;
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(logos_device, *buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;

    uint32_t memoryTypeIndex = Private_findMemoryType(memRequirements.memoryTypeBits, properties);
    if (memoryTypeIndex == UINT32_MAX) {
        fprintf(stderr, "Failed to find suitable memory type\n");
        vkDestroyBuffer(logos_device, *buffer, NULL);
        return VK_ERROR_MEMORY_MAP_FAILED;
    }
    allocInfo.memoryTypeIndex = memoryTypeIndex;

    result = vkAllocateMemory(logos_device, &allocInfo, NULL, bufferMemory);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to allocate buffer memory\n");
        vkDestroyBuffer(logos_device, *buffer, NULL);
        return result;
    }

    vkBindBufferMemory(logos_device, *buffer, *bufferMemory, 0);

    return VK_SUCCESS;
}

// Function to create the Vulkan instance and surface
void Logos_Instance_Create(SDL_Window* window)
{
    VkResult result;

    VkApplicationInfo appInfo = {0};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Logos Application";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Logos Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    uint32_t sdlExtensionCount = 0;
    if (!SDL_Vulkan_GetInstanceExtensions(window, &sdlExtensionCount, NULL)) {
        fprintf(stderr, "Failed to get SDL Vulkan instance extensions: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    const char** sdlExtensions = (const char**)malloc(sizeof(const char*) * sdlExtensionCount);
    if (sdlExtensions == NULL) {
        fprintf(stderr, "Failed to allocate memory for SDL Vulkan extensions\n");
        exit(EXIT_FAILURE);
    }

    if (!SDL_Vulkan_GetInstanceExtensions(window, &sdlExtensionCount, sdlExtensions)) {
        fprintf(stderr, "Failed to get SDL Vulkan instance extensions: %s\n", SDL_GetError());
        free(sdlExtensions);
        exit(EXIT_FAILURE);
    }

    // Optional: Add VK_EXT_DEBUG_UTILS_EXTENSION_NAME for debugging
    const char* additionalExtensions[] = { "VK_EXT_debug_utils" };
    uint32_t totalExtensionCount = sdlExtensionCount + 1;
    const char** allExtensions = (const char**)malloc(sizeof(const char*) * totalExtensionCount);
    if (allExtensions == NULL) {
        fprintf(stderr, "Failed to allocate memory for all Vulkan extensions\n");
        free(sdlExtensions);
        exit(EXIT_FAILURE);
    }
    memcpy(allExtensions, sdlExtensions, sizeof(const char*) * sdlExtensionCount);
    allExtensions[sdlExtensionCount] = "VK_EXT_debug_utils";

    const char* validationLayers[] = {
        "VK_LAYER_KHRONOS_validation"
    };
    uint32_t validationLayerCount = 1;

    VkInstanceCreateInfo instanceCreateInfo = {0};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.enabledExtensionCount = totalExtensionCount;
    instanceCreateInfo.ppEnabledExtensionNames = allExtensions;
    instanceCreateInfo.enabledLayerCount = validationLayerCount;
    instanceCreateInfo.ppEnabledLayerNames = validationLayers;

    // Enable debug messenger if using VK_EXT_debug_utils
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {0};
    // You would typically set up a debug callback here
    // For simplicity, we're skipping it in this example
    // If you set it up, link it properly

    // instanceCreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;

    result = vkCreateInstance(&instanceCreateInfo, NULL, &logos_instance);
    free(sdlExtensions);
    free(allExtensions);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create Vulkan instance\n");
        exit(EXIT_FAILURE);
    }

    // Create Vulkan surface
    if (!SDL_Vulkan_CreateSurface(window, logos_instance, &logos_surface)) {
        fprintf(stderr, "Failed to create Vulkan surface: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
}

// Function to create the logical device with proper queue creation
void Logos_Device_Create()
{
    VkResult result;

    uint32_t deviceCount = 0;
    result = vkEnumeratePhysicalDevices(logos_instance, &deviceCount, NULL);
    if (result != VK_SUCCESS || deviceCount == 0) {
        fprintf(stderr, "Failed to find GPUs with Vulkan support\n");
        exit(EXIT_FAILURE);
    }

    VkPhysicalDevice* devices = malloc(sizeof(VkPhysicalDevice) * deviceCount);
    if (devices == NULL) {
        fprintf(stderr, "Failed to allocate memory for physical devices\n");
        exit(EXIT_FAILURE);
    }
    result = vkEnumeratePhysicalDevices(logos_instance, &deviceCount, devices);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to enumerate physical devices\n");
        free(devices);
        exit(EXIT_FAILURE);
    }
    logos_physical_device = devices[0]; // Select the first suitable device
    free(devices);

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(logos_physical_device, &queueFamilyCount, NULL);
    if (queueFamilyCount == 0) {
        fprintf(stderr, "Failed to find any queue families\n");
        exit(EXIT_FAILURE);
    }
    VkQueueFamilyProperties* queueFamilies = malloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
    if (queueFamilies == NULL) {
        fprintf(stderr, "Failed to allocate memory for queue family properties\n");
        exit(EXIT_FAILURE);
    }
    vkGetPhysicalDeviceQueueFamilyProperties(logos_physical_device, &queueFamilyCount, queueFamilies);

    graphicsQueueFamilyIndex = UINT32_MAX;
    presentQueueFamilyIndex = UINT32_MAX;

    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphicsQueueFamilyIndex = i;
        }

        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(logos_physical_device, i, logos_surface, &presentSupport);

        if (presentSupport == VK_TRUE) {
            presentQueueFamilyIndex = i;
        }

        if (graphicsQueueFamilyIndex != UINT32_MAX && presentQueueFamilyIndex != UINT32_MAX) {
            break;
        }
    }

    free(queueFamilies);

    if (graphicsQueueFamilyIndex == UINT32_MAX || presentQueueFamilyIndex == UINT32_MAX) {
        fprintf(stderr, "Failed to find suitable queue families\n");
        exit(EXIT_FAILURE);
    }

    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfos[2];
    uint32_t queueCreateInfoCount = 0;

    if (graphicsQueueFamilyIndex == presentQueueFamilyIndex) {
        // Zero-initialize the structure
        memset(&queueCreateInfos[0], 0, sizeof(VkDeviceQueueCreateInfo));
        queueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfos[0].pNext = NULL;
        queueCreateInfos[0].flags = 0;
        queueCreateInfos[0].queueFamilyIndex = graphicsQueueFamilyIndex;
        queueCreateInfos[0].queueCount = 1;
        queueCreateInfos[0].pQueuePriorities = &queuePriority;
        queueCreateInfoCount = 1;
    } else {
        // Zero-initialize both structures
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

    // Validation layers are deprecated in deviceCreateInfo for Vulkan 1.0 and later
    // Instead, they should be enabled during instance creation
    deviceCreateInfo.enabledLayerCount = 0;
    deviceCreateInfo.ppEnabledLayerNames = NULL;

    // Define Push Constant Range
    VkPushConstantRange pushConstantRange = {0};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushConstantsData);

    // Define Pipeline Layout Create Info
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {0};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = NULL;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    deviceCreateInfo.pNext = NULL; // Ensure pNext is NULL

    result = vkCreateDevice(logos_physical_device, &deviceCreateInfo, NULL, &logos_device);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create logical device\n");
        exit(EXIT_FAILURE);
    }

    vkGetDeviceQueue(logos_device, graphicsQueueFamilyIndex, 0, &graphicsQueue);
    vkGetDeviceQueue(logos_device, presentQueueFamilyIndex, 0, &presentQueue);

    // Create Pipeline Layout
    if (vkCreatePipelineLayout(logos_device, &pipelineLayoutInfo, NULL, &pipelineLayout) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create pipeline layout\n");
        exit(EXIT_FAILURE);
    }
}

// Function to create the render pass
VkRenderPass Logos_RenderPass_Create()
{
    VkAttachmentDescription colorAttachment = {0};
    colorAttachment.format = logos_swapChainImageFormat;
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

    VkRenderPass renderPass;
    if (vkCreateRenderPass(logos_device, &renderPassInfo, NULL, &renderPass) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create render pass\n");
        return VK_NULL_HANDLE;
    }

    return renderPass;
}

// Function to create the graphics pipeline
VkPipeline Logos_GraphicsPipeline_Create(VkRenderPass renderPass, VkPipelineLayout pipelineLayout, VkShaderModule vertShaderModule, VkShaderModule fragShaderModule)
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

    VkVertexInputBindingDescription bindingDescription = {0};
    bindingDescription.binding   = 0;
    bindingDescription.stride    = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attributeDescriptions[2] = {0};

    attributeDescriptions[0].binding  = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format   = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset   = offsetof(Vertex, position);

    attributeDescriptions[1].binding  = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset   = offsetof(Vertex, color);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {0};
    vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount   = 1;
    vertexInputInfo.pVertexBindingDescriptions      = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = 2;
    vertexInputInfo.pVertexAttributeDescriptions    = attributeDescriptions;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {0};
    inputAssembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {0};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = (float)logos_swapChainExtent.width;
    viewport.height   = (float)logos_swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {0};
    scissor.offset = (VkOffset2D){0, 0};
    scissor.extent = logos_swapChainExtent;


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
    rasterizer.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
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

    VkPipeline graphicsPipelineLocal;
    if (vkCreateGraphicsPipelines(logos_device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &graphicsPipelineLocal) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create graphics pipeline\n");
        exit(EXIT_FAILURE);
    }

    return graphicsPipelineLocal;
}

// Function to create a command pool
VkCommandPool Logos_CommandPool_Create(uint32_t queueFamilyIndex)
{
    VkCommandPoolCreateInfo poolInfo = {0};
    poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndex;
    poolInfo.flags            = 0; // Optional flags

    VkCommandPool commandPool;
    if (vkCreateCommandPool(logos_device, &poolInfo, NULL, &commandPool) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create command pool\n");
        return VK_NULL_HANDLE;
    }

    return commandPool;
}

// Function to create command buffers
VkCommandBuffer* Logos_CommandBuffers_Create(VkCommandPool commandPool, uint32_t commandBufferCount)
{
    VkCommandBufferAllocateInfo allocInfo = {0};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool        = commandPool;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = commandBufferCount;

    VkCommandBuffer* commandBuffers = malloc(sizeof(VkCommandBuffer) * commandBufferCount);
    if (commandBuffers == NULL) {
        fprintf(stderr, "Failed to allocate command buffer handles\n");
        return NULL;
    }

    if (vkAllocateCommandBuffers(logos_device, &allocInfo, commandBuffers) != VK_SUCCESS) {
        fprintf(stderr, "Failed to allocate command buffers\n");
        free(commandBuffers);
        return NULL;
    }

    return commandBuffers;
}

// Function to record command buffers
void Logos_CommandBuffers_Record(VkCommandBuffer* commandBuffers, uint32_t commandBufferCount, VkRenderPass renderPass, VkPipeline graphicsPipeline, VkFramebuffer* framebuffers, VkExtent2D extent, VkBuffer vertexBuffer)
{
    for (uint32_t i = 0; i < commandBufferCount; i++) {
        VkCommandBufferBeginInfo beginInfo = {0};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
            fprintf(stderr, "Failed to begin recording command buffer\n");
            exit(EXIT_FAILURE);
        }

        VkRenderPassBeginInfo renderPassInfo = {0};
        renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass        = renderPass;
        renderPassInfo.framebuffer       = framebuffers[i];
        renderPassInfo.renderArea.offset = (VkOffset2D){0, 0};
        renderPassInfo.renderArea.extent = extent;

        VkClearValue clearColor = { .color = {{0.0f, 0.0f, 0.0f, 1.0f}} };
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues    = &clearColor;

        vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        // Set Push Constants here
        vkCmdPushConstants(
            commandBuffers[i],
            pipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(PushConstantsData),
            &pushConstantsData
        );

        VkBuffer vertexBuffersArray[] = { vertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffersArray, offsets);

        vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);

        vkCmdEndRenderPass(commandBuffers[i]);

        if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
            fprintf(stderr, "Failed to record command buffer\n");
            exit(EXIT_FAILURE);
        }
    }
}

VkPipelineLayout Logos_PipelineLayout_Create()
{
    // Define Push Constant Range
    VkPushConstantRange pushConstantRange = {0};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushConstantsData);

    // Define Pipeline Layout Create Info
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {0};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = NULL;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    VkPipelineLayout pipelineLayout;
    if (vkCreatePipelineLayout(logos_device, &pipelineLayoutInfo, NULL, &pipelineLayout) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create pipeline layout\n");
        return VK_NULL_HANDLE;
    }

    return pipelineLayout;
}

// Function to create the SDL window
void Logos_Window_Create(int width, int height, const char* title)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "ERROR: SDL_Init: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    logos_window = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN
    );
    if (!logos_window) {
        fprintf(stderr, "ERROR: SDL_CreateWindow: %s\n", SDL_GetError());
        SDL_Quit();
        exit(EXIT_FAILURE);
    }
}

// Function to create the swapchain
void Logos_Swapchain_Create()
{
    if (!logos_window) {
        fprintf(stderr, "ERROR: To create swap chain, the window must be created first\n");
        exit(EXIT_FAILURE);
    }

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(logos_physical_device, logos_surface, &surfaceCapabilities);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to get surface capabilities\n");
        exit(EXIT_FAILURE);
    }

    if (surfaceCapabilities.currentExtent.width != UINT32_MAX) {
        logos_swapChainExtent = surfaceCapabilities.currentExtent;
    } else {
        logos_swapChainExtent.width = 800;
        logos_swapChainExtent.height = 600;
    }

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(logos_physical_device, logos_surface, &formatCount, NULL);
    if (formatCount == 0) {
        fprintf(stderr, "Failed to find surface formats\n");
        exit(EXIT_FAILURE);
    }
    VkSurfaceFormatKHR* formats = malloc(sizeof(VkSurfaceFormatKHR) * formatCount);
    if (formats == NULL) {
        fprintf(stderr, "Failed to allocate memory for surface formats\n");
        exit(EXIT_FAILURE);
    }
    vkGetPhysicalDeviceSurfaceFormatsKHR(logos_physical_device, logos_surface, &formatCount, formats);
    logos_swapChainImageFormat = formats[0].format;
    free(formats);

    VkSwapchainCreateInfoKHR swapchainCreateInfo = {0};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.surface = logos_surface;
    swapchainCreateInfo.minImageCount = surfaceCapabilities.minImageCount + 1;
    // Ensure minImageCount does not exceed maxImageCount (if maxImageCount > 0)
    if (surfaceCapabilities.maxImageCount > 0 && swapchainCreateInfo.minImageCount > surfaceCapabilities.maxImageCount) {
        swapchainCreateInfo.minImageCount = surfaceCapabilities.maxImageCount;
    }
    swapchainCreateInfo.imageFormat = logos_swapChainImageFormat;
    swapchainCreateInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    swapchainCreateInfo.imageExtent = logos_swapChainExtent;
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

    result = vkCreateSwapchainKHR(logos_device, &swapchainCreateInfo, NULL, &logos_swapChain);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "ERROR: Failed to create swapchain\n");
        exit(EXIT_FAILURE);
    }
}

// Function to create image views
VkImageView* Logos_ImageViews_Create(VkImage* swapChainImages, uint32_t imageCount, VkFormat format)
{
    VkImageView* imageViews = malloc(sizeof(VkImageView) * imageCount);
    if (imageViews == NULL) {
        fprintf(stderr, "Failed to allocate memory for image views\n");
        return NULL;
    }

    for (uint32_t i = 0; i < imageCount; i++) {
        VkImageViewCreateInfo viewInfo = {0};
        viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image                           = swapChainImages[i];
        viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format                          = format;
        viewInfo.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel   = 0;
        viewInfo.subresourceRange.levelCount     = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount     = 1;

        if (vkCreateImageView(logos_device, &viewInfo, NULL, &imageViews[i]) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create image views\n");
            for (uint32_t j = 0; j < i; j++) {
                vkDestroyImageView(logos_device, imageViews[j], NULL);
            }
            free(imageViews);
            return NULL;
        }
    }

    return imageViews;
}

// Function to create framebuffers
VkFramebuffer* Logos_Framebuffers_Create(VkImageView* imageViews, uint32_t imageCount, VkRenderPass renderPass, VkExtent2D extent)
{
    VkFramebuffer* framebuffers = malloc(sizeof(VkFramebuffer) * imageCount);
    if (framebuffers == NULL) {
        fprintf(stderr, "Failed to allocate memory for framebuffers\n");
        return NULL;
    }

    for (uint32_t i = 0; i < imageCount; i++) {
        VkImageView attachments[] = { imageViews[i] };

        VkFramebufferCreateInfo framebufferInfo = {0};
        framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass      = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments    = attachments;
        framebufferInfo.width           = extent.width;
        framebufferInfo.height          = extent.height;
        framebufferInfo.layers          = 1;

        if (vkCreateFramebuffer(logos_device, &framebufferInfo, NULL, &framebuffers[i]) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create framebuffer\n");
            for (uint32_t j = 0; j < i; j++) {
                vkDestroyFramebuffer(logos_device, framebuffers[j], NULL);
            }
            free(framebuffers);
            return NULL;
        }
    }

    return framebuffers;
}

// Function to show the window and handle the rendering loop
void Logos_Window_Show()
{
    if (!logos_window) {
        fprintf(stderr, "ERROR: You tried to show the window before creating it.\n");
        exit(EXIT_FAILURE);
    }

    // Get swapchain images
    uint32_t swapChainImageCount;
    VkResult result = vkGetSwapchainImagesKHR(logos_device, logos_swapChain, &swapChainImageCount, NULL);
    if (result != VK_SUCCESS || swapChainImageCount == 0) {
        fprintf(stderr, "Failed to get swapchain images\n");
        exit(EXIT_FAILURE);
    }
    VkImage* swapChainImages = malloc(sizeof(VkImage) * swapChainImageCount);
    if (swapChainImages == NULL) {
        fprintf(stderr, "Failed to allocate memory for swapchain images\n");
        exit(EXIT_FAILURE);
    }
    result = vkGetSwapchainImagesKHR(logos_device, logos_swapChain, &swapChainImageCount, swapChainImages);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to get swapchain images\n");
        free(swapChainImages);
        exit(EXIT_FAILURE);
    }

    // Create image views
    VkImageView* swapChainImageViews = Logos_ImageViews_Create(swapChainImages, swapChainImageCount, logos_swapChainImageFormat);
    if (swapChainImageViews == NULL) {
        fprintf(stderr, "Failed to create swap chain image views\n");
        free(swapChainImages);
        exit(EXIT_FAILURE);
    }

    // Create framebuffers
    VkFramebuffer* swapChainFramebuffers = Logos_Framebuffers_Create(swapChainImageViews, swapChainImageCount, renderPass, logos_swapChainExtent);
    if (swapChainFramebuffers == NULL) {
        fprintf(stderr, "Failed to create swap chain framebuffers\n");
        free(swapChainImageViews);
        free(swapChainImages);
        exit(EXIT_FAILURE);
    }

    // Create command pool
    VkCommandPool commandPool = Logos_CommandPool_Create(graphicsQueueFamilyIndex);
    if (commandPool == VK_NULL_HANDLE) {
        fprintf(stderr, "Failed to create command pool\n");
        free(swapChainFramebuffers);
        free(swapChainImageViews);
        free(swapChainImages);
        exit(EXIT_FAILURE);
    }

    // Create command buffers
    VkCommandBuffer* commandBuffers = Logos_CommandBuffers_Create(commandPool, swapChainImageCount);
    if (commandBuffers == NULL) {
        fprintf(stderr, "Failed to create command buffers\n");
        vkDestroyCommandPool(logos_device, commandPool, NULL);
        free(swapChainFramebuffers);
        free(swapChainImageViews);
        free(swapChainImages);
        exit(EXIT_FAILURE);
    }

    // Create vertex buffer
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkDeviceSize bufferSize = sizeof(vertices);

    if (Private_createBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &vertexBuffer, &vertexBufferMemory) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create vertex buffer\n");
        free(commandBuffers);
        vkDestroyCommandPool(logos_device, commandPool, NULL);
        free(swapChainFramebuffers);
        free(swapChainImageViews);
        free(swapChainImages);
        exit(EXIT_FAILURE);
    }

    // Map memory and copy vertex data
    void* data;
    vkMapMemory(logos_device, vertexBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices, (size_t)bufferSize);
    vkUnmapMemory(logos_device, vertexBufferMemory);

    // Record command buffers with push constants
    Logos_CommandBuffers_Record(commandBuffers, swapChainImageCount, renderPass, graphicsPipeline, swapChainFramebuffers, logos_swapChainExtent, vertexBuffer);

    // Create synchronization objects
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;

    VkSemaphoreCreateInfo semaphoreInfo = {0};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {0};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (vkCreateSemaphore(logos_device, &semaphoreInfo, NULL, &imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(logos_device, &semaphoreInfo, NULL, &renderFinishedSemaphore) != VK_SUCCESS ||
        vkCreateFence(logos_device, &fenceInfo, NULL, &inFlightFence) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create synchronization objects\n");
        vkDestroyBuffer(logos_device, vertexBuffer, NULL);
        vkFreeMemory(logos_device, vertexBufferMemory, NULL);
        free(commandBuffers);
        vkDestroyCommandPool(logos_device, commandPool, NULL);
        free(swapChainFramebuffers);
        free(swapChainImageViews);
        free(swapChainImages);
        exit(EXIT_FAILURE);
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

        // Wait for previous frame
        vkWaitForFences(logos_device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
        vkResetFences(logos_device, 1, &inFlightFence);

        // Acquire next image
        uint32_t imageIndex;
        VkResult acquireResult = vkAcquireNextImageKHR(logos_device, logos_swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
        if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR || acquireResult == VK_SUBOPTIMAL_KHR) {
            // Handle swapchain recreation here if needed
            fprintf(stderr, "Swapchain out of date or suboptimal. Consider recreating swapchain.\n");
            running = 0;
            continue;
        } else if (acquireResult != VK_SUCCESS) {
            fprintf(stderr, "Failed to acquire swapchain image\n");
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

        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence) != VK_SUCCESS) {
            fprintf(stderr, "Failed to submit draw command buffer\n");
            running = 0;
            continue;
        }

        // Present the image
        VkPresentInfoKHR presentInfo = {0};
        presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores    = signalSemaphores;

        VkSwapchainKHR swapChains[] = { logos_swapChain };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains    = swapChains;
        presentInfo.pImageIndices  = &imageIndex;

        VkResult presentResult = vkQueuePresentKHR(presentQueue, &presentInfo);
        if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR) {
            // Handle swapchain recreation here if needed
            fprintf(stderr, "Swapchain out of date or suboptimal during present. Consider recreating swapchain.\n");
            running = 0;
            continue;
        } else if (presentResult != VK_SUCCESS) {
            fprintf(stderr, "Failed to present swapchain image\n");
            running = 0;
            continue;
        }

        // Optionally, you can add vkQueueWaitIdle here for simplicity
        vkQueueWaitIdle(presentQueue);
    }

    // Wait for device to finish operations before cleanup
    vkDeviceWaitIdle(logos_device);

    // Cleanup synchronization objects
    vkDestroySemaphore(logos_device, renderFinishedSemaphore, NULL);
    vkDestroySemaphore(logos_device, imageAvailableSemaphore, NULL);
    vkDestroyFence(logos_device, inFlightFence, NULL);

    // Free command buffers and destroy command pool
    vkFreeCommandBuffers(logos_device, commandPool, swapChainImageCount, commandBuffers);
    vkDestroyCommandPool(logos_device, commandPool, NULL);

    // Destroy framebuffers and image views
    for (uint32_t i = 0; i < swapChainImageCount; i++) {
        vkDestroyFramebuffer(logos_device, swapChainFramebuffers[i], NULL);
        vkDestroyImageView(logos_device, swapChainImageViews[i], NULL);
    }

    free(swapChainFramebuffers);
    free(swapChainImageViews);
    free(swapChainImages);
    free(commandBuffers);

    // Cleanup vertex buffer
    vkDestroyBuffer(logos_device, vertexBuffer, NULL);
    vkFreeMemory(logos_device, vertexBufferMemory, NULL);

    // Cleanup pipeline and layout
    vkDestroyPipeline(logos_device, graphicsPipeline, NULL);
    vkDestroyPipelineLayout(logos_device, pipelineLayout, NULL);

    // Cleanup render pass
    vkDestroyRenderPass(logos_device, renderPass, NULL);
}

// Function to clean up all resources
void Logos_Window_Delete() {
    if (logos_swapChain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(logos_device, logos_swapChain, NULL);
        logos_swapChain = VK_NULL_HANDLE;
    }
    if (logos_device != VK_NULL_HANDLE) {
        vkDestroyDevice(logos_device, NULL);
        logos_device = VK_NULL_HANDLE;
    }
    if (logos_surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(logos_instance, logos_surface, NULL);
        logos_surface = VK_NULL_HANDLE;
    }
    if (logos_instance != VK_NULL_HANDLE) {
        vkDestroyInstance(logos_instance, NULL);
        logos_instance = VK_NULL_HANDLE;
    }
    if (logos_window) {
        SDL_DestroyWindow(logos_window);
        logos_window = NULL;
    }
    SDL_Quit();
}

// Main function
void test() {

    Logos_Window_Create(800, 600, "Logos Vulkan Window");
    Logos_Instance_Create(logos_window);
    Logos_Device_Create();
    Logos_Swapchain_Create();

    // Create Render Pass
    renderPass = Logos_RenderPass_Create();
    if (renderPass == VK_NULL_HANDLE) {
        fprintf(stderr, "Failed to create render pass\n");
        exit(EXIT_FAILURE);
    }

    // Create Pipeline Layout
    pipelineLayout = Logos_PipelineLayout_Create();
    if (pipelineLayout == VK_NULL_HANDLE) {
        fprintf(stderr, "Failed to create pipeline layout\n");
        exit(EXIT_FAILURE);
    }

    // Create Shader Modules
    VkShaderModule vertShaderModule = Private_createShaderModule("shaders/shader.vert.spv");
    VkShaderModule fragShaderModule = Private_createShaderModule("shaders/shader.frag.spv");

    // Create Graphics Pipeline
    graphicsPipeline = Logos_GraphicsPipeline_Create(renderPass, pipelineLayout, vertShaderModule, fragShaderModule);
    if (graphicsPipeline == VK_NULL_HANDLE) {
        fprintf(stderr, "Failed to create graphics pipeline\n");
        exit(EXIT_FAILURE);
    }

    // Destroy Shader Modules after pipeline creation
    vkDestroyShaderModule(logos_device, vertShaderModule, NULL);
    vkDestroyShaderModule(logos_device, fragShaderModule, NULL);

    Logos_Window_Show();
    Logos_Window_Delete();

}
