// vulkan_gui.h
#ifndef VULKAN_GUI_H
#define VULKAN_GUI_H

#include <vulkan/vulkan.h>
#include <shaderc/shaderc.h>
#include <vk_mem_alloc.h>
#include "spirv_reflect.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h> 

#define DEBUG
#include "debug.h"
void* alloc(void* ptr, size_t size);

#define ALL_INSTANCE_COUNT 5
#define INDICES_COUNT 5

// Structures

typedef struct {
    const uint32_t* code;
    size_t          size;
} SpvShader;

typedef struct {
    float pos[2];        // middle_x, middle_y
    float size[2];       // width, height
    float rotation;      // rotation angle
    float corner_radius; // pixels
    uint32_t color;      // background color packed as RGBA8
    uint32_t tex_index;  // texture ID and other info
    float tex_rect[4];   // texture rectangle (u, v, width, height)
} InstanceData;

typedef struct {
    float targetWidth;
    float targetHeight;
    // Padding to align to 16 bytes
    float padding[2];
} UniformBufferObject;

typedef struct {
    uint32_t graphics;
    uint32_t present;
    uint32_t compute;
    uint32_t transfer;
} VkQueueFamilyIndices;

typedef struct {
    VkQueue graphics;
    VkQueue present;
    VkQueue compute;
    VkQueue transfer;
} VkQueues;

typedef struct {
    VkBuffer buffer;
    VmaAllocation allocation;
    size_t size;
} Buffer;

typedef struct {

    shaderc_compiler_t          shaderc_compiler;
    shaderc_compile_options_t   shaderc_options;
    void*                       window_p;
    VkInstance                  instance;
    VkDebugUtilsMessengerEXT    debug_messenger;
    VkSurfaceKHR                surface;
    VkPhysicalDevice            physical_device;
    VkDevice                    device;
    VkQueueFamilyIndices        queue_family_indices;
    VkQueues                    queues;
    VkCommandPool               command_pool;
    VkDescriptorPool            descriptor_pool;
    VmaAllocator                allocator;

    struct {
        VkSwapchainKHR          swap_chain;
        VkFormat                image_format;
        unsigned int            image_count;
        VkExtent2D              extent;
        VkImage*                images_p;
        VkImageView*            image_views_p;
    }                           swap_chain;


} VK;

// bedrock
VK                          VK_create(unsigned int width, unsigned int height, const char* title);
void                        VK_startApp(VK* p_vk,VkSemaphore imageAvailableSemaphore,VkSemaphore renderFinishedSemaphore,VkFence inFlightFence,VkCommandBuffer* commandBuffers,VkBuffer uniformBuffer,VmaAllocation uniformBufferAllocation,Buffer instance_buffer);
void                        VK_destroy(VK* p_vk,VkPipeline graphicsPipeline,VkPipelineLayout pipelineLayout,VkDescriptorSetLayout descriptorSetLayout,VkBuffer uniformBuffer,VmaAllocation uniformBufferAllocation,VkBuffer instanceBuffer,VmaAllocation instanceBufferAllocation,VkDescriptorSet descriptorSet,VkCommandBuffer* commandBuffers,VkSemaphore imageAvailableSemaphore,VkSemaphore renderFinishedSemaphore,VkFence inFlightFence);

// buffer
Buffer                      createBuffer( VK* p_vk, VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage );
void                        clearBuffer( VK* p_vk, Buffer buffer, int clear_value) ;
void                        updateBuffer( VK* p_vk, Buffer buffer, VkDeviceSize dst_offset, const void* p_src_data, VkDeviceSize size );

// image
VkImage                     createImage( VK* p_vk );
VkImageView                 createImageView( VK* p_vk );
VkSampler                   createImageSampler( VK* p_vk );
VkImage                     loadImageFromFile( VK* p_vk, const char* filename );
VkImage                     createImageAtlas( VK* p_vk, const char** filenames, uint32_t imageCount );
void                        copyBufferToImage( VK* p_vk, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height );
void                        transitionImageLayout( VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout );
void                        destroyImage( VkDevice device, VkImage* vulkanImage );

// shader
size_t                      readFile(const char* filename, char** dst_buffer);
SpvShader                   createSpvShader(VK* p_vk, const char* p_glsl_code, size_t glsl_size, const char* glsl_filename, shaderc_shader_kind kind);
SpvReflectShaderModule      createSpvReflectShaderModule(SpvShader spv_shader);
VkShaderModule              createShaderModule(VK* p_vk, SpvShader spv_shader);
VkShaderModule              createShaderModuleFromGlslFile(VK* p_vk, const char* filename, shaderc_shader_kind shader_kind);

// descriptor
VkDescriptorSetLayout       createDescriptorSetLayout_0(VK* p_vk);
VkDescriptorSet             createDescriptorSet( VK* p_vk, VkDescriptorSetLayout desc_set_layout, VkBuffer buffer );

// pipeline
VkPipelineLayout            createPipelineLayout(VkDevice device, VkDescriptorSetLayout* p_set_layouts, size_t set_layout_count);
VkPipeline                  createGraphicsPipeline(VK* p_vk, VkPipelineLayout pipelineLayout);

// command buffer
VkCommandBuffer*            createCommandBuffersForSwapchain(VK* p_vk, VkDescriptorSet desc_set, VkPipeline graphics_pipeline,VkPipelineLayout graphics_pipeline_layout,VkBuffer instance_buffer);

// synchronization
VkSemaphore                 createSemaphore(VkDevice device);
VkFence                     createFence(VkDevice device);


void                        test();

#endif // VULKAN_GUI_H
