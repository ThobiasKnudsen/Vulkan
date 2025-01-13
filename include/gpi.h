#pragma once

#include <vulkan/vulkan.h>
#include <shaderc/shaderc.h>
#include <vk_mem_alloc.h>
#include "spirv_reflect.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <libgen.h>

#define VK_USE_PLATFORM_XLIB_KHR
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

typedef struct {
    uint32_t vertex_count;
    uint32_t instance_count;
    uint32_t first_vertex;
    uint32_t first_instance;
} Gpi_DrawIndirectCommand;

typedef struct {
    VkBuffer        buffer;             
    VmaAllocation   allocation;    
    VkDeviceSize    size;                 
    VmaMemoryUsage  usage;
} Gpi_Buffer;

typedef struct {
    VkImage         image;
    VkImageLayout   layout;
    VkExtent2D      extent;
    VkFormat        format;
    VmaAllocation   allocation;
    VkImageView     view;
    VkSampler       sampler;
} Gpi_Image;

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
    void*                           window_p;
    VkSurfaceKHR                    surface;
    VkSwapchainKHR                  swap_chain;
    Gpi_Image*                      p_images;
    unsigned int                    images_count;
} Gpi_Window;

typedef struct {
    shaderc_compiler_t              shaderc_compiler;
    shaderc_compile_options_t       shaderc_options;
    void*                           window_p;
    VkInstance                      instance;
    VkDebugUtilsMessengerEXT        debug_messenger;
    VkSurfaceKHR                    surface;
    VkPhysicalDevice                physical_device;
    VkDevice                        device;
    Gpi_QueueFamilyIndices          queue_family_indices;
    Gpi_Queues                      queues;
    VkCommandPool                   command_pool;
    VkDescriptorPool                descriptor_pool;
    VmaAllocator                    allocator;
    VkSwapchainKHR                  swap_chain;
    Gpi_Image*                      p_images;
    unsigned int                    images_count;
} Gpi_Context;

typedef struct {
    void*                           p_spv_code;
    unsigned int                    spv_code_size;
    shaderc_shader_kind             shader_kind;
    SpvReflectShaderModule          reflect_shader_module;
    VkShaderModule                  shader_module;
} Gpi_Shader;

typedef struct {
    Gpi_Context*                        p_ctx; 
    Gpi_Shader*                         p_shaders;
    unsigned int                        shaders_count;
    VkDescriptorSetLayoutCreateInfo*    p_desc_sets_layout_create_info;
    VkDescriptorSetLayout*              p_desc_sets_layout;
    unsigned int                        desc_sets_count;
    VkPipelineLayout                    pipeline_layout;
    VkPipeline                          graphics_pipeline;
} Gpi_Pipeline_Graphics;

typedef struct {
    Gpi_Pipeline_Graphics*          p_pipeline;
    VkDescriptorSet*                p_desc_sets;
    unsigned int                    desc_sets_count;
} Gpi_DescriptorSets;

typedef struct {
    Gpi_DescriptorSets*             p_descriptor_sets;
    Gpi_Image*                      p_target_image;
    VkCommandBuffer                 command_buffer;
    bool                            command_buffer_needs_recording;
    Gpi_Buffer                      indirect_buffer; // containing DrawIndirectCommand
    Gpi_Buffer                      instance_buffer; // containing InstanceData array
} Gpi_Rendering;

typedef struct {
    VkCommandBuffer                 command_buffer;
} Gpi_Cmd;

typedef struct {
    VkSemaphore                     semaphore;
} Gpi_Semaphore;

typedef struct {
    VkFence                         fence;
} Gpi_Fence;

// context ===============================================================================================================================
Gpi_Context                         gpi_Context_Create(unsigned int width, unsigned int height, const char* title);
void                                gpi_Context_StartApplication(Gpi_Context* p_ctx, Gpi_Cmd* p_cmd, Gpi_Buffer indirect_buffer);
void                                gpi_Context_StopApplication(Gpi_Context* p_ctx);
void                                gpi_Context_Destroy(Gpi_Context* p_ctx);

// image =================================================================================================================================
Gpi_Image                           gpi_Image_Create_ReadWrite(Gpi_Context* p_ctx, VkExtent2D extent, VkFormat format);
Gpi_Image                           gpi_Image_Create_FromImageFile(Gpi_Context* p_ctx, const char* filename, VkFormat format, VkImageLayout layout);
void                                gpi_Image_TransitionLayout(Gpi_Cmd* p_cmd, Gpi_Image* p_image, VkImageLayout new_layout);
void                                gpi_Image_CopyData(Gpi_Context* p_ctx, Gpi_Image* p_image, VkImageLayout final_layout, const void* p_data, const VkRect2D rect, const size_t pixel_size);
void                                gpi_Image_Destroy(Gpi_Context* p_ctx, Gpi_Image* p_image);

// buffer ================================================================================================================================
Gpi_Buffer                          gpi_Buffer_Create(Gpi_Context* p_ctx, VkDeviceSize size, VkBufferUsageFlags usage);
void                                gpi_Buffer_Clear(Gpi_Context* p_ctx, Gpi_Buffer buffer, int clear_value);
void                                gpi_Buffer_Update( Gpi_Context* p_ctx, Gpi_Buffer buffer, VkDeviceSize dst_offset, const void* p_src_data, VkDeviceSize size);
void                                gpi_Buffer_CopyOtherBuffer(Gpi_Context* p_ctx, Gpi_Buffer src_buffer, Gpi_Buffer dst_buffer, VkDeviceSize src_offset, VkDeviceSize dst_offset, VkDeviceSize size);
void                                gpi_Buffer_Destroy(Gpi_Context* p_ctx, Gpi_Buffer* p_buffer);
void                                gpi_Buffer_Destroy(Gpi_Context* p_ctx, Gpi_Buffer* p_buffer);

// graphics pipeline ======================================================================================================================
Gpi_Pipeline_Graphics               gpi_Pipeline_Graphics_Create(Gpi_Context* p_ctx, Gpi_Shader* p_shaders, unsigned int shaders_count, VkFormat format);
void                                gpi_Pipeline_Graphics_Destroy(Gpi_Context* p_ctx, Gpi_Pipeline_Graphics* p_pipeline);

// shader =================================================================================================================================
void                                gpi_Shader_CreateSpvFileFromGlslFile(Gpi_Context* p_ctx, const char* glsl_filename, const char* spv_filename, shaderc_shader_kind shader_kind);
Gpi_Shader                          gpi_Shader_CreateFromGlslFile(Gpi_Context* p_ctx, const char* glsl_file_path, shaderc_shader_kind shader_kind);
VkVertexInputAttributeDescription*  gpi_Shader_Create_VertexInputAttribDesc(const Gpi_Shader shader, unsigned int* p_attribute_count, unsigned int* p_binding_stride);
void                                gpi_Shader_Destroy(Gpi_Context* p_ctx, Gpi_Shader* p_shader);

// descriptor sets ========================================================================================================================
VkDescriptorSetLayoutCreateInfo*    _gpi_DescriptorSetLayoutCreateInfo_Create(Gpi_Context* p_ctx, const SpvReflectShaderModule* p_shader_modules, unsigned int shader_modules_count, size_t* p_create_info_count);
VkDescriptorSetLayout*              _gpi_DescriptorSetLayout_Create(Gpi_Context* p_ctx, const VkDescriptorSetLayoutCreateInfo* p_create_info, const size_t create_info_count);
Gpi_DescriptorSets                  gpi_DescriptorSets_Create(Gpi_Pipeline_Graphics* p_pipeline);
void                                gpi_DescriptorSets_UpdateUniformBuffer(Gpi_DescriptorSets* p_desc_sets, const unsigned int set, const unsigned int binding, const unsigned int array_element, const Gpi_Buffer* p_buffer, const VkDeviceSize offset, const VkDeviceSize range);
void                                gpi_DescriptorSets_UpdateCombinedImageSampler(Gpi_DescriptorSets* p_desc_sets, const unsigned int set, const unsigned int binding, const unsigned int array_element, const Gpi_Image* p_image);
void                                gpi_DescriptorSets_UpdateSampledImage(Gpi_DescriptorSets* p_desc_sets, const unsigned int set, const unsigned int binding, const unsigned int array_element, const VkImageView image_view, const VkImageLayout image_layout);
void                                gpi_DescriptorSets_UpdateStorageImage(Gpi_DescriptorSets* p_desc_sets, const unsigned int set, const unsigned int binding, const unsigned int array_element, const VkImageView image_view, const VkImageLayout image_layout);
void                                gpi_DescriptorSets_UpdateStorageBuffer(Gpi_DescriptorSets* p_desc_sets, const unsigned int set, const unsigned int binding, const unsigned int array_element, const Gpi_Buffer buffer, const VkDeviceSize offset, const VkDeviceSize range);
void                                gpi_DescriptorSets_Destroy(Gpi_Context* p_ctx, Gpi_DescriptorSets* p_desc_sets);

// command buffer =========================================================================================================================
Gpi_Cmd                             gpi_Cmd_CreateAndStartRecording();
void                                gpi_Cmd_StopRecording(Gpi_Cmd* p_cmd);
void                                gpi_Cmd_Destroy(Gpi_Context* p_ctx, Gpi_Cmd* p_cmd);
void                                gpi_Cmd_RecordStaticRendering(Gpi_Cmd* p_cmd, Gpi_Context* p_ctx, Gpi_Image* p_target_image, VkDescriptorSet* p_desc_sets,  size_t desc_sets_count, VkPipeline graphics_pipeline, VkPipelineLayout graphics_pipeline_layout, VkBuffer instance_buffer, size_t instances_count);
void                                gpi_Cmd_RecordStaticRendering_Indirect(Gpi_Cmd* p_cmd, Gpi_Context* p_ctx, Gpi_Image* p_target_image, VkDescriptorSet* p_desc_sets,  size_t desc_sets_count, VkPipeline graphics_pipeline, VkPipelineLayout graphics_pipeline_layout, VkBuffer instance_buffer, size_t instances_count, Gpi_Buffer indirect_buffer);
Gpi_Cmd                             gpi_Cmd_CreateAndBeginSingleTimeUsage(Gpi_Context* p_ctx);
void                                gpi_Cmd_EndAndDestroySingleTimeUsage(Gpi_Context* p_ctx, Gpi_Cmd* p_cmd);

// semaphore ==============================================================================================================================
Gpi_Semaphore                       gpi_Semaphore_Create(Gpi_Context* p_ctx);

// fence ==================================================================================================================================
Gpi_Fence                           gpi_Fence_Create(Gpi_Context* p_ctx);

// TEMPORARY ==============================================================================================================================
