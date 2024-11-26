// main.c
#define VK_USE_PLATFORM_XLIB_KHR
#define USE_SDL2

#include "gui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <shaderc/shaderc.h>
#include <vk_mem_alloc.h>
#ifdef USE_SDL2
    #include <SDL2/SDL.h>
    #include <SDL2/SDL_vulkan.h>
#endif

#ifdef USE_GLFW
    #include <GLFW/glfw3.h>
#endif


#define DEBUG
#include "debug.h"

// Memory allocation utility function
void* alloc(void* ptr, size_t size) {
    void* tmp = (ptr == NULL) ? malloc(size) : realloc(ptr, size);
    if (!tmp) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    return tmp;
}

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
#define INDICES_COUNT 5
uint32_t indices[INDICES_COUNT] = { 0, 1, 2, 3, 4 }; // Indices of instances to draw

// Utility function for logging Vulkan errors
static void logVulkanError(const char* msg) {
    printf("%s\n", msg);
}

// Function Implementations
void cleanup(
    VmaAllocator allocator,                    // VMA allocator
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
    VmaAllocation uniformBufferAllocation,     // VmaAllocation for uniformBuffer
    VkBuffer instanceBuffer,
    VmaAllocation instanceBufferAllocation,    // VmaAllocation for instanceBuffer
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
    if (graphicsPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, graphicsPipeline, NULL);
    }

    if (pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, pipelineLayout, NULL);
    }

    if (renderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device, renderPass, NULL);
    }

    if (descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, NULL);
    }

    if (descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, descriptorPool, NULL);
    }

    // Destroy uniform buffer and its allocation using VMA
    if (uniformBuffer != VK_NULL_HANDLE) {
        vmaDestroyBuffer(allocator, uniformBuffer, uniformBufferAllocation);
    }

    // Destroy instance buffer and its allocation using VMA
    if (instanceBuffer != VK_NULL_HANDLE) {
        vmaDestroyBuffer(allocator, instanceBuffer, instanceBufferAllocation);
    }

    // Destroy swap chain framebuffers
    if (swapChainFramebuffers != NULL) {
        for (uint32_t i = 0; i < swapChainImageCount; i++) {
            if (swapChainFramebuffers[i] != VK_NULL_HANDLE) {
                vkDestroyFramebuffer(device, swapChainFramebuffers[i], NULL);
            }
        }
        free(swapChainFramebuffers);
    }

    // Destroy swap chain image views
    if (swapChainImageViews != NULL) {
        for (uint32_t i = 0; i < swapChainImageCount; i++) {
            if (swapChainImageViews[i] != VK_NULL_HANDLE) {
                vkDestroyImageView(device, swapChainImageViews[i], NULL);
            }
        }
        free(swapChainImageViews);
    }

    if (swapChain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device, swapChain, NULL);
    }

    if (commandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(device, commandPool, NULL);
    }

    if (imageAvailableSemaphore != VK_NULL_HANDLE) {
        vkDestroySemaphore(device, imageAvailableSemaphore, NULL);
    }

    if (renderFinishedSemaphore != VK_NULL_HANDLE) {
        vkDestroySemaphore(device, renderFinishedSemaphore, NULL);
    }

    if (inFlightFence != VK_NULL_HANDLE) {
        vkDestroyFence(device, inFlightFence, NULL);
    }

    if (device != VK_NULL_HANDLE) {
        vkDestroyDevice(device, NULL);
    }

    if (debugMessenger != VK_NULL_HANDLE) {
        PFN_vkDestroyDebugUtilsMessengerEXT funcDestroyDebugUtilsMessengerEXT = 
            (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (funcDestroyDebugUtilsMessengerEXT != NULL) {
            funcDestroyDebugUtilsMessengerEXT(instance, debugMessenger, NULL);
        }
    }

    if (surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(instance, surface, NULL);
    }

    if (swapChain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device, swapChain, NULL);
    }

    if (allocator != VK_NULL_HANDLE) {
        vmaDestroyAllocator(allocator); // Destroy the VMA allocator
    }

    #if defined(USE_SDL2)
        if (window != NULL) {
            SDL_DestroyWindow(window);
        }
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

    *buffer = (char*)alloc(NULL, fileSize + 1);
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
    VkPipelineLayout graphicsPipelineLayout,
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

    VkCommandBuffer* commandBuffers = alloc(NULL, sizeof(VkCommandBuffer) * imageCount);
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
        vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelineLayout, 0, 1, &descriptorSet, 0, NULL);
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

// Modified mainLoop function using VMA
void mainLoop(
    VmaAllocator allocator,
    VkDevice device,
    VkQueue graphicsQueue,
    VkQueue presentQueue,
    VkSwapchainKHR swapChain,
    VkSemaphore imageAvailableSemaphore,
    VkSemaphore renderFinishedSemaphore,
    VkFence inFlightFence,
    VkCommandBuffer* commandBuffers,
    uint32_t imageCount,
    VkDescriptorSet descriptorSet,
    VkExtent2D swapChainExtent,
    VkBuffer uniformBuffer,
    VmaAllocation uniformBufferAllocation
){
    #if defined(USE_SDL2)
        int running = 1;
        SDL_Event event;
        
        while (running) {
            // Handle SDL Events
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    running = 0;
                }
            }

            // Update Uniform Buffer Data
            UniformBufferObject ubo = {};
            ubo.targetWidth = (float)swapChainExtent.width;
            ubo.targetHeight = (float)swapChainExtent.height;

            void* data;
            VkResult mapResult = vmaMapMemory(allocator, uniformBufferAllocation, &data);
            if (mapResult != VK_SUCCESS) {
                fprintf(stderr, "Failed to map uniform buffer memory: %d\n", mapResult);
                running = 0;
                continue;
            }
            memcpy(data, &ubo, sizeof(ubo));
            vmaUnmapMemory(allocator, uniformBufferAllocation);
            vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
            vkResetFences(device, 1, &inFlightFence);

            // Acquire the next image from the swap chain
            uint32_t imageIndex;
            VkResult acquireResult = vkAcquireNextImageKHR(
                device,
                swapChain,
                UINT64_MAX,
                imageAvailableSemaphore,
                VK_NULL_HANDLE,
                &imageIndex
            );

            if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR || acquireResult == VK_SUBOPTIMAL_KHR) {
                fprintf(stderr, "Swapchain out of date or suboptimal. Consider recreating swapchain.\n");
                running = 0;
                continue;
            }
            else if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR) {
                fprintf(stderr, "Failed to acquire swapchain image: %d\n", acquireResult);
                running = 0;
                continue;
            }

            // Submit the command buffer
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

            VkResult submitResult = vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence);
            if (submitResult != VK_SUCCESS) {
                fprintf(stderr, "Failed to submit draw command buffer: %d\n", submitResult);
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

            VkResult presentResult = vkQueuePresentKHR(presentQueue, &presentInfo);
            if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR) {
                fprintf(stderr, "Swapchain out of date or suboptimal during present. Consider recreating swapchain.\n");
                running = 0;
                continue;
            }
            else if (presentResult != VK_SUCCESS) {
                fprintf(stderr, "Failed to present swapchain image: %d\n", presentResult);
                running = 0;
                continue;
            }

            // Optionally wait for the present queue to be idle
            vkQueueWaitIdle(presentQueue);
        }
    #else
        fprintf(stderr, "SDL has to be included and it has to be included\n");
        exit(EXIT_FAILURE);
    #endif
}

typedef struct {

// STATIC
    void*                       window_p;
    VkInstance                  instance;
    VkDebugUtilsMessengerEXT    debug_messenger;
    VkSurfaceKHR                surface;
    VkPhysicalDevice            physical_device;
    VkDevice                    device;
    VkQueueFamilyIndices        queue_family_indices;
    VkQueues                    queues;

    struct {
        VkRenderPass            render_pass;
        VkSwapchainKHR          swap_chain;
        VkFormat                image_format;
        unsigned int            image_count;
        VkExtent2D              extent;
        VkImageView*            image_views_p;
        VkFramebuffer*          frame_buffers_p;
    }                           swap_chain;

    struct {
        VkCommandPool           pool;
        VkCommandPoolCreateInfo pool_info;
        VkCommandBuffer*        buffers_p;
        size_t                  buffers_p_size;
    }                           command;

    struct {
        VkDescriptorPool        pool;
        //VkDescriptorPoolCreateInfo        info;

        VkDescriptorSet*        sets_p;
        VkDescriptorSetLayout*  set_layouts_p;
        size_t                  sets_size;
    }                           descriptor;

    VmaAllocator allocator;
    
} VK;

VK VK_Create(unsigned int width, unsigned int height, const char* title) {
    VK vk;
    memset(&vk, 0, sizeof(VK));
    vk.window_p                     = NULL;
    vk.swap_chain.image_views_p     = NULL;
    vk.swap_chain.frame_buffers_p   = NULL;
    vk.command.buffers_p            = NULL;
    vk.descriptor.sets_p            = NULL;
    vk.descriptor.set_layouts_p     = NULL;

    VkResult result;

    // createWindow
    {
        #ifdef USE_SDL2
            if (SDL_Init(SDL_INIT_VIDEO) != 0) {
                printf("ERROR: SDL_Init: %s\n", SDL_GetError());
                exit(EXIT_FAILURE);
            }
            vk.window_p = SDL_CreateWindow(title,SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,width,height,SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN);
            if (!vk.window_p) {
                printf("ERROR: SDL_CreateWindow: %s\n", SDL_GetError());
                SDL_Quit();
                exit(EXIT_FAILURE);
            }
        #elif defined(USE_GLFW)
            if (!glfwInit()) {
                fprintf(stderr, "ERROR: Failed to initialize GLFW\n");
                exit(EXIT_FAILURE);
            }
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
            vk.window_p = glfwCreateWindow(width, height, title, NULL, NULL);
            if (!vk.window_p) {
                fprintf(stderr, "ERROR: Failed to create GLFW window\n");
                glfwTerminate();
                exit(EXIT_FAILURE);
            }
        #else
            printf("there is no supported windowing API!\n");
            exit(EXIT_FAILURE);
        #endif
    }
    // createVulkanInstance
    {
        uint32_t totalExtensionCount = 1;
        const char** allExtensions = NULL;
        uint32_t validationLayerCount = 0;
        const char* validationLayers[] = { "VK_LAYER_KHRONOS_validation" };

        #ifdef USE_SDL2
            uint32_t sdlExtensionCount = 0;
            if (!SDL_Vulkan_GetInstanceExtensions((SDL_Window*)vk.window_p, &sdlExtensionCount, NULL)) {
                printf("Failed to get SDL Vulkan instance extensions: %s\n", SDL_GetError());
                exit(EXIT_FAILURE);
            }

            const char** sdlExtensions = (const char**)alloc(NULL, sizeof(const char*) * sdlExtensionCount);
            if (sdlExtensions == NULL) {
                printf("Failed to allocate memory for SDL Vulkan extensions\n");
                exit(EXIT_FAILURE);
            }

            if (!SDL_Vulkan_GetInstanceExtensions((SDL_Window*)vk.window_p, &sdlExtensionCount, sdlExtensions)) {
                printf("Failed to get SDL Vulkan instance extensions: %s\n", SDL_GetError());
                free(sdlExtensions);
                exit(EXIT_FAILURE);
            }

            // Optional: Add VK_EXT_debug_utils_EXTENSION_NAME for debugging
            totalExtensionCount = sdlExtensionCount + 1;
            allExtensions = (const char**)alloc(NULL, sizeof(const char*) * totalExtensionCount);
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

        result = vkCreateInstance(&instanceCreateInfo, NULL, &vk.instance);
        #if defined(USE_SDL2)
            free(sdlExtensions);
        #endif
        free(allExtensions);
        if (result != VK_SUCCESS) {
            printf("Failed to create Vulkan instance\n");
            exit(EXIT_FAILURE);
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
                exit(EXIT_FAILURE);
            }
        } else {
            printf("vkCreateDebugUtilsMessengerEXT not available\n");
            vk.debug_messenger = VK_NULL_HANDLE;
        }
    }
    // createSurface
    {
        vk.surface = VK_NULL_HANDLE;
        #if defined(USE_SDL2)
            if (!SDL_Vulkan_CreateSurface((SDL_Window*)vk.window_p, vk.instance, &vk.surface)) {
                printf( "Failed to create Vulkan surface: %s\n", SDL_GetError());
                exit(EXIT_FAILURE);
            }
        #else
            printf("SDL2 has to be included\n");
            exit(EXIT_FAILURE);
        #endif
    }
    // getPhysicalDevice
    {
        uint32_t device_count = 0;
        result = vkEnumeratePhysicalDevices(vk.instance, &device_count, NULL);
        if (result != VK_SUCCESS || device_count == 0) {
            printf( "Failed to find GPUs with Vulkan support\n");
            exit(EXIT_FAILURE);
        }

        VkPhysicalDevice* devices = alloc(NULL, sizeof(VkPhysicalDevice) * device_count);
        if (devices == NULL) {
            printf( "Failed to allocate memory for physical devices\n");
            exit(EXIT_FAILURE);
        }
        result = vkEnumeratePhysicalDevices(vk.instance, &device_count, devices);
        if (result != VK_SUCCESS) {
            printf( "Failed to enumerate physical devices\n");
            free(devices);
            exit(EXIT_FAILURE);
        }

        // Select the first suitable device
        vk.physical_device = devices[0];
        free(devices);
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
            exit(EXIT_FAILURE);
        }

        VkQueueFamilyProperties* queue_families = alloc(NULL, sizeof(VkQueueFamilyProperties) * queue_family_count);
        if (queue_families == NULL) {
            printf("Failed to allocate memory for queue family properties\n");
            exit(EXIT_FAILURE);
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
            exit(EXIT_FAILURE);
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
        result = vkCreateDevice(vk.physical_device, &device_create_info, NULL, &vk.device);
        if (result != VK_SUCCESS) {
            printf("Failed to create logical vk.device. Error code: %d\n", result);
            free(queue_create_infos);
            exit(EXIT_FAILURE);
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
            fprintf(stderr, "Failed to create VMA allocator\n");
            exit(EXIT_FAILURE);
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
    vk.swap_chain.image_format = VK_FORMAT_B8G8R8A8_SRGB;
    vk.swap_chain.extent = (VkExtent2D){width, height};
    // createSwapChain
    {
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk.physical_device, vk.surface, &surfaceCapabilities);
        if (result != VK_SUCCESS) {
            printf("Failed to get vk.surface capabilities\n");
            exit(EXIT_FAILURE);
        }

        VkExtent2D actualExtent = surfaceCapabilities.currentExtent.width != UINT32_MAX ?
                                  surfaceCapabilities.currentExtent :
                                  vk.swap_chain.extent;

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(vk.physical_device, vk.surface, &formatCount, NULL);
        if (formatCount == 0) {
            printf("Failed to find vk.surface formats\n");
            exit(EXIT_FAILURE);
        }
        VkSurfaceFormatKHR* formats = alloc(NULL, sizeof(VkSurfaceFormatKHR) * formatCount);
        if (formats == NULL) {
            printf("Failed to allocate memory for vk.surface formats\n");
            exit(EXIT_FAILURE);
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
            exit(EXIT_FAILURE);
        }
    }
    {
        debug(vk.swap_chain.image_count = 0);
        debug(VkResult result = vkGetSwapchainImagesKHR(vk.device, vk.swap_chain.swap_chain, &vk.swap_chain.image_count, NULL));
        if (result != VK_SUCCESS || vk.swap_chain.image_count == 0) {
            printf("Failed to get swapchain images or image count is zero\n");
            exit(EXIT_FAILURE);
        }
    }
    // createImageViews
    {
        VkImage* swapChainImages = alloc(NULL, sizeof(VkImage) * vk.swap_chain.image_count);
        if (swapChainImages == NULL) {
            printf("Failed to allocate memory for swapchain images\n");
            exit(EXIT_FAILURE);
        }

        VkResult result = vkGetSwapchainImagesKHR(vk.device, vk.swap_chain.swap_chain, &vk.swap_chain.image_count, swapChainImages);
        if (result != VK_SUCCESS) {
            printf("Failed to get swapchain images\n");
            free(swapChainImages);
            exit(EXIT_FAILURE);
        }

        vk.swap_chain.image_views_p = alloc(vk.swap_chain.image_views_p, sizeof(VkImageView) * vk.swap_chain.image_count);
        if (vk.swap_chain.image_views_p == NULL) {
            printf("Failed to allocate memory for image views\n");
            free(swapChainImages);
            exit(EXIT_FAILURE);
        }

        for (uint32_t i = 0; i < vk.swap_chain.image_count; i++) {
            VkImageViewCreateInfo viewInfo = {
                .sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image                           = swapChainImages[i],
                .viewType                        = VK_IMAGE_VIEW_TYPE_2D,
                .format                          = vk.swap_chain.image_format,
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

            if (vkCreateImageView(vk.device, &viewInfo, NULL, &vk.swap_chain.image_views_p[i]) != VK_SUCCESS) {
                printf("Failed to create image view %u\n", i);
                free(swapChainImages);
                for (uint32_t j = 0; j < i; j++) {
                    vkDestroyImageView(vk.device, vk.swap_chain.image_views_p[j], NULL);
                }
                free(vk.swap_chain.image_views_p);
                exit(EXIT_FAILURE);
            }
        }
        free(swapChainImages);
    }
    // createRenderPass
    {
        VkRenderPassCreateInfo renderPassInfo   = {
            .sType                              = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount                    = 1,
            .pAttachments                       = (VkAttachmentDescription[]){{
                .format                         = vk.swap_chain.image_format,
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
        if (vkCreateRenderPass(vk.device, &renderPassInfo, NULL, &vk.swap_chain.render_pass) != VK_SUCCESS) {
            printf("Failed to create render pass\n");
            exit(EXIT_FAILURE);
        }
    }
    // createFramebuffers
    {
        vk.swap_chain.frame_buffers_p = alloc(vk.swap_chain.frame_buffers_p, sizeof(VkFramebuffer) * vk.swap_chain.image_count);
        if (vk.swap_chain.frame_buffers_p == NULL) {
            printf("Failed to allocate memory for vk.swap_chain.frame_buffers_p\n");
            exit(EXIT_FAILURE);
        }
        for (uint32_t i = 0; i < vk.swap_chain.image_count; i++) {
            VkImageView attachments[] = { vk.swap_chain.image_views_p[i] };
            VkFramebufferCreateInfo framebufferInfo = {
                .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass      = vk.swap_chain.render_pass,
                .attachmentCount = 1,
                .pAttachments    = attachments,
                .width           = vk.swap_chain.extent.width,
                .height          = vk.swap_chain.extent.height,
                .layers          = 1
            };
            if (vkCreateFramebuffer(vk.device, &framebufferInfo, NULL, &vk.swap_chain.frame_buffers_p[i]) != VK_SUCCESS) {
                printf("Failed to create framebuffer %u\n", i);
                for (uint32_t j = 0; j < i; j++) {
                    vkDestroyFramebuffer(vk.device, vk.swap_chain.frame_buffers_p[j], NULL);
                }
                free(vk.swap_chain.frame_buffers_p);
                exit(EXIT_FAILURE);
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
        if (vkCreateDescriptorPool(vk.device, &pool_info, NULL, &vk.descriptor.pool) != VK_SUCCESS) {
            printf("Failed to create descriptor pool\n");
            exit(EXIT_FAILURE);
        }
    }
    // createCommandPool
    {
        VkCommandPoolCreateInfo poolInfo = {
            .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .queueFamilyIndex = vk.queue_family_indices.graphics,
            .flags            = 0 // Optional flags
        };
        if (vkCreateCommandPool(vk.device, &poolInfo, NULL, &vk.command.pool) != VK_SUCCESS) {
            printf("Failed to create command pool\n");
            exit(EXIT_FAILURE);
        }
    }

    return vk;
}

VkBuffer createBufferWithVma(VK* vk, VkBufferCreateInfo* bufferInfo, VmaAllocationCreateInfo* allocInfo, VmaAllocation* allocation) {
    VkBuffer buffer;
    if (vmaCreateBuffer(vk->allocator, bufferInfo, allocInfo, &buffer, allocation, NULL) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create buffer with VMA\n");
        exit(EXIT_FAILURE);
    }
    return buffer;
}

void test() {
    VK vk = VK_Create(800, 600, "Vulkan GUI");

    // Create Uniform Buffer with VMA
    VkBufferCreateInfo uniformBufferInfo = { 
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = sizeof(UniformBufferObject),
        .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };
    VmaAllocationCreateInfo uniformAllocInfo = {
        .usage = VMA_MEMORY_USAGE_CPU_TO_GPU
    };
    VmaAllocation uniformBufferAllocation;
    debug(VkBuffer uniformBuffer = createBufferWithVma(&vk, &uniformBufferInfo, &uniformAllocInfo, &uniformBufferAllocation));

    vk.descriptor.sets_size++;
    debug(vk.descriptor.sets_p = alloc(vk.descriptor.sets_p, sizeof(VkDescriptorSet) * vk.descriptor.sets_size));
    debug(vk.descriptor.set_layouts_p = alloc(vk.descriptor.set_layouts_p, sizeof(VkDescriptorSetLayout) * vk.descriptor.sets_size));
    debug(vk.descriptor.set_layouts_p[vk.descriptor.sets_size - 1] = createDescriptorSetLayout(vk.device));
    debug(vk.descriptor.sets_p[vk.descriptor.sets_size - 1] = createDescriptorSet(
        vk.device,
        vk.descriptor.pool,
        vk.descriptor.set_layouts_p[vk.descriptor.sets_size - 1],
        uniformBuffer
    ));

    // Create Instance Buffer with VMA
    VkBufferCreateInfo instanceBufferInfo = { 
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = sizeof(InstanceData) * ALL_INSTANCE_COUNT,
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };
    VmaAllocationCreateInfo instanceAllocInfo = {
        .usage = VMA_MEMORY_USAGE_CPU_TO_GPU
    };
    VmaAllocation instanceBufferAllocation;
    debug(VkBuffer instanceBuffer = createBufferWithVma(&vk, &instanceBufferInfo, &instanceAllocInfo, &instanceBufferAllocation));

    void* instanceData;
    VkResult mapResult = vmaMapMemory(vk.allocator, instanceBufferAllocation, &instanceData);
    if (mapResult != VK_SUCCESS) {
        fprintf(stderr, "Failed to map instance buffer memory: %d\n", mapResult);
        exit(EXIT_FAILURE);
    }
    memcpy(instanceData, all_instances, sizeof(InstanceData) * ALL_INSTANCE_COUNT);
    vmaUnmapMemory(vk.allocator, instanceBufferAllocation);

    debug(VkPipelineLayout graphicsPipelineLayout = createPipelineLayout(vk.device, vk.descriptor.set_layouts_p[vk.descriptor.sets_size - 1]));
    debug(VkPipeline graphicsPipeline = createGraphicsPipeline(
        vk.device,
        graphicsPipelineLayout,
        vk.swap_chain.render_pass,
        vk.swap_chain.extent
    ));
    debug(VkCommandBuffer* commandBuffers = createCommandBuffers(
        vk.device,
        vk.command.pool,
        graphicsPipeline,
        graphicsPipelineLayout,
        vk.swap_chain.render_pass,
        vk.swap_chain.frame_buffers_p,
        vk.swap_chain.image_count,
        instanceBuffer,
        vk.descriptor.sets_p[vk.descriptor.sets_size - 1],
        vk.swap_chain.extent
    ));
    debug(VkSemaphore imageAvailableSemaphore = createSemaphore(vk.device));
    debug(VkSemaphore renderFinishedSemaphore = createSemaphore(vk.device));
    debug(VkFence inFlightFence = createFence(vk.device));
    debug(mainLoop(
        vk.allocator,
        vk.device,
        vk.queues.graphics,
        vk.queues.present,
        vk.swap_chain.swap_chain,
        imageAvailableSemaphore, 
        renderFinishedSemaphore,
        inFlightFence,
        commandBuffers,
        vk.swap_chain.image_count,
        vk.descriptor.sets_p[vk.descriptor.sets_size-1],
        vk.swap_chain.extent,
        uniformBuffer,
        uniformBufferAllocation
    ));
    debug(cleanup(
        vk.allocator,
        vk.device,
        vk.instance,
        vk.surface,
        vk.debug_messenger,
        graphicsPipeline,
        graphicsPipelineLayout,
        vk.swap_chain.render_pass,
        vk.descriptor.set_layouts_p[vk.descriptor.sets_size-1],
        vk.descriptor.pool,
        uniformBuffer,
        uniformBufferAllocation,
        instanceBuffer,
        instanceBufferAllocation,
        vk.descriptor.sets_p[vk.descriptor.sets_size-1],
        vk.swap_chain.swap_chain,
        vk.swap_chain.image_views_p,
        vk.swap_chain.image_count,
        vk.swap_chain.frame_buffers_p,
        vk.command.pool,
        commandBuffers,
        imageAvailableSemaphore,
        renderFinishedSemaphore,
        inFlightFence,
        vk.window_p
    ));
}
