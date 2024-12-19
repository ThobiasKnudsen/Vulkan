#pragma once

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

typedef struct {
    unsigned int graphics;
    unsigned int present;
    unsigned int compute;
    unsigned int transfer;
} Gpi_QueueFamilyIndices;

typedef struct {
    VkQueue graphics;
    VkQueue present;
    VkQueue compute;
    VkQueue transfer;
} Gpi_Queues;

typedef struct {
    VkBuffer buffer;             
    VmaAllocation allocation;    
    size_t size;                 
    VmaMemoryUsage usage;
} Gpi_Buffer;

typedef struct {
    VkImage image;
    VkImageLayout layout;
    VkExtent2D extent;
    VkFormat format;
    VmaAllocation allocation;
    VkImageView view;
    VkSampler sampler;
} Gpi_Image;

typedef struct {
    shaderc_compiler_t          shaderc_compiler;
    shaderc_compile_options_t   shaderc_options;
    void*                       window_p;
    VkInstance                  instance;
    VkDebugUtilsMessengerEXT    debug_messenger;
    VkSurfaceKHR                surface;
    VkPhysicalDevice            physical_device;
    VkDevice                    device;
    Gpi_QueueFamilyIndices      queue_family_indices;
    Gpi_Queues                  queues;
    VkCommandPool               command_pool;
    VkDescriptorPool            descriptor_pool;
    VmaAllocator                allocator;
    VkSwapchainKHR              swap_chain;
    Gpi_Image*                  p_images;
    size_t                      images_count;
} Gpi_Context;

typedef struct {
    const unsigned int*     p_spv_code;
    size_t                  spv_code_size;
    shaderc_shader_kind     shader_kind;
    SpvReflectShaderModule  reflect_shader_module;
    VkShaderModule          shader_module;
} Gpi_Shader;

typedef struct {
    Gpi_Context*                        p_ctx; 
    Gpi_Shader*                         p_shaders;
    size_t                              shaders_count;
    VkDescriptorSetLayoutCreateInfo*    p_desc_sets_layout_create_info;
    VkDescriptorSetLayout*              p_desc_sets_layout;
    size_t                              desc_sets_count;
    VkPipelineLayout                    pipeline_layout;
    VkPipeline                          graphics_pipeline;
} Gpi_Pipeline_Graphics;

typedef struct {
    Gpi_GraphicsPipeline*   p_pipeline;
    VkDescriptorSet*        p_desc_sets;
    size_t                  desc_sets_count;
} Gpi_DescriptorSets;

typedef struct {
    Gpi_DescriptorSets*     p_descriptor_sets;
    Gpi_Image*              p_target_image;
    VkCommandBuffer         command_buffer;
    bool                    command_buffer_needs_recording;
    Gpi_Buffer              indirect_buffer; // containing VkDrawIndirectCommand
    Gpi_Buffer              instance_buffer; // containing InstanceData array
} Gpi_Rendering;

// graphics pipeline
Gpi_Pipeline_Graphics        Gpi_Pipeline_Graphics_Create();