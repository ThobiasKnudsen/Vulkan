#pragma once

// Λόγος

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
    VmaMemoryUsage usage;
} Buffer;

typedef struct {
    VkImage image;
    VkImageLayout layout;
    VkExtent2D extent;
    VkFormat format;
    VmaAllocation allocation;
    VkImageView image_view;
    VkSampler sampler;
} Image;

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
        size_t                  image_count;
        VkExtent2D              extent;
        VkImage*                images_p;
        VkImageView*            image_views_p;
    }                           swap_chain;

} Vk;

typedef struct {
    const unsigned int*     p_spv_code;
    size_t                  spv_code_size;
    shaderc_shader_kind     shader_kind;
    SpvReflectShaderModule  reflect_shader_module;
    VkShaderModule          shader_module;
} Gui_Shader;

typedef struct {
    Vk*                                 p_vk; 
    Gui_Shader*                         p_shaders;
    size_t                              shaders_count;
    VkDescriptorSetLayoutCreateInfo*    p_desc_sets_layout_create_info;
    VkDescriptorSetLayout*              p_desc_sets_layout;
    size_t                              desc_sets_count;
    VkPipelineLayout                    pipeline_layout;
    VkPipeline                          graphics_pipeline;

} Gui_GraphicsPipeline;

typedef struct {

    Vk*                     p_vk;  
    Gui_GraphicsPipeline*   p_pipeline;
    Image*                  p_target_image;

    VkDescriptorSet*        p_desc_sets;
    size_t                  desc_sets_count;

    VkCommandBuffer         command_buffer;

    Buffer                  indirect_buffer; // containing VkDrawIndirectCommand
    Buffer                  instance_buffer; // containing InstanceData array

} Gui_Rendering;

// bedrock
Vk                          vk_Create(unsigned int width, unsigned int height, const char* title);
void                        vk_StartApp(Vk* p_vk,VkSemaphore imageAvailableSemaphore,VkSemaphore renderFinishedSemaphore,VkFence inFlightFence,VkCommandBuffer* commandBuffers,VkBuffer uniformBuffer,VmaAllocation uniformBufferAllocation,Buffer instance_buffer);
void                        vk_Destroy(Vk* p_vk,VkPipeline graphicsPipeline,VkPipelineLayout pipelineLayout,VkDescriptorSetLayout descriptorSetLayout,VkBuffer uniformBuffer,VmaAllocation uniformBufferAllocation,VkBuffer instanceBuffer,VmaAllocation instanceBufferAllocation,VkDescriptorSet descriptorSet,VkCommandBuffer* commandBuffers,VkSemaphore imageAvailableSemaphore,VkSemaphore renderFinishedSemaphore,VkFence inFlightFence);

// buffer
Buffer                      vk_Buffer_Create( Vk* p_vk, VkDeviceSize size, VkBufferUsageFlags usage );
void                        vk_Buffer_Clear( Vk* p_vk, Buffer buffer, int clear_value) ;
void                        vk_Buffer_Update( Vk* p_vk, Buffer buffer, VkDeviceSize dst_offset, const void* p_src_data, VkDeviceSize size );

// image
Image                       vk_Image_Create_ReadWrite( Vk* p_vk,  VkExtent2D extent,  VkFormat format );
Image                       vk_Image_CreateFromImageFile( Vk* p_vk, const char* filename, VkFormat format, VkImageLayout layout );
Image                       vk_Image_LoadFromFile( Vk* p_vk, const char* filename );
Image                       vk_Image_CreateAtlas( Vk* p_vk, const char** filenames, uint32_t imageCount );
void                        vk_Image_CopyData( Vk* p_vk, Image* p_image, VkImageLayout final_layout, const void* p_data, const VkRect2D rect, const size_t pixel_size );
void                        vk_Image_TransitionLayout(VkCommandBuffer command_buffer, Image* p_image, VkImageLayout new_layout);
void                        vk_Image_TransitionLayout_0(VkCommandBuffer command_buffer, VkImage* p_image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout);
void                        vk_Image_Destroy( VkDevice device, VkImage* vulkanImage );

// shader
size_t                              readFile(const char* filename, char** dst_buffer);
SpvShader                           vk_SpvShader_Create(Vk* p_vk, const char* p_glsl_code, size_t glsl_size, const char* glsl_filename, shaderc_shader_kind kind);
SpvShader                           vk_SpvShader_CreateFromGlslFile(Vk* p_vk, const char* filename, shaderc_shader_kind shader_kind);
void                                vk_SpvShader_CreateSpvFileFromGlslFile(Vk* p_vk, const char* glsl_filename, const char* spv_filename, shaderc_shader_kind shader_kind);
SpvReflectShaderModule              vk_SpvReflectShaderModule_Create(SpvShader spv_shader);
VkShaderModule                      vk_ShaderModule_Create(Vk* p_vk, SpvShader spv_shader);
VkShaderModule                      vk_ShaderModule_CreateFromGlslFile(Vk* p_vk, const char* filename, shaderc_shader_kind shader_kind);
VkVertexInputAttributeDescription*  vk_VertexInputAttributeDescriptions_CreateFromVertexShader( SpvShader spv_shader, uint32_t* p_attribute_count, uint32_t* p_binding_stride );

// descriptor
VkDescriptorSetLayoutCreateInfo*    vk_DescriptorSetLayoutCreateInfo_Create( Vk* p_vk, const SpvReflectShaderModule* p_shader_modules, unsigned int shader_modules_count, size_t* p_desc_set_layout_count ); 
void                                vk_DescriptorSetLayoutCreateInfo_Print(const VkDescriptorSetLayoutCreateInfo* p_create_info, const size_t create_info_count);
VkDescriptorSetLayout*              vk_DescriptorSetLayout_Create(Vk* p_vk, const VkDescriptorSetLayoutCreateInfo* p_create_info, const size_t create_info_count);
VkDescriptorSetLayout               vk_DescriptorSetLayout_Create_0(Vk* p_vk);
VkDescriptorSet*                    vk_DescriptorSet_Create(Vk* p_vk, const VkDescriptorSetLayout* p_desc_set_layout, size_t desc_set_layouts_count);
VkDescriptorSet*                    vk_DescriptorSet_Create_0(Vk* p_vk, const VkDescriptorSetLayout* p_desc_set_layout, size_t desc_set_layouts_count, VkBuffer buffer, Image* p_image);

// pipeline
VkPipelineLayout            vk_PipelineLayout_Create(VkDevice device, VkDescriptorSetLayout* p_set_layouts, size_t set_layout_count);
VkPipeline                  vk_Pipeline_Graphics_Create(Vk* p_vk, VkPipelineLayout pipelineLayout);

// command buffer
VkCommandBuffer*            vk_CommandBuffer_CreateForSwapchain(Vk* p_vk, VkDescriptorSet* p_desc_set, size_t desc_set_count, VkPipeline graphics_pipeline,VkPipelineLayout graphics_pipeline_layout,VkBuffer instance_buffer, Image* p_image);
VkCommandBuffer*            vk_CommandBuffer_CreateForSwapchain_0( Vk* p_vk, Gui_Rendering* p_rendering, VkBuffer instance_buffer, size_t instance_count, Image* p_image);
VkCommandBuffer             vk_CommandBuffer_CreateAndBeginSingleTimeUsage(Vk* p_vk);
void                        vk_CommandBuffer_EndAndDestroySingleTimeUsage(Vk* p_vk, VkCommandBuffer command_buffer);
VkCommandBuffer             vk_CommandBuffer_CreateWithImageAttachment( Vk* p_vk, Image* p_image, VkDescriptorSet* p_desc_set, size_t desc_set_count, VkPipeline graphics_pipeline, VkPipelineLayout graphics_pipeline_layout, VkBuffer instance_buffer, uint32_t vertex_count, uint32_t instance_count); 
void                        vk_CommandBuffer_Submit(Vk* p_vk, VkCommandBuffer command_buffer);

// synchronization
VkSemaphore                 vk_Semaphore_Create(VkDevice device);
VkFence                     vk_Fence_Create(VkDevice device);

// gui_graphics_pipeline 
Gui_GraphicsPipeline        Gui_GraphicsPipeline_Initialize(Vk* p_vk);
void                        Gui_GraphicsPipeline_AddShader(Gui_GraphicsPipeline* p_pipeline, SpvShader spv_shader, shaderc_shader_kind shader_kind);
void                        Gui_GraphicsPipeline_CreatePipeline(Gui_GraphicsPipeline* p_pipeline);

// gui_rendering
Gui_Rendering               Gui_Rendering_Create(Gui_GraphicsPipeline* p_pipeline, Image* p_target_image);
void                        Gui_Rendering_SetTargetImage(Gui_Rendering* p_rendering, Image* p_target_image);
void                        Gui_Rendering_UpdateInstanceBuffer(Gui_Rendering* p_rendering, size_t dst_offset, void* p_src_data, size_t src_data_size);
void                        Gui_Rendering_UpdateIndirectBuffer(Gui_Rendering* p_rendering, uint32_t instance_count, uint32_t first_instance);

void                        Gui_Rendering_UpdateUniformBuffer(Gui_Rendering* p_rendering, Gui_GraphicsPipeline* p_pipeline, unsigned int set, unsigned int binding, unsigned int array_element, VkBuffer buffer, unsigned int offset, unsigned int range);
void                        Gui_Rendering_UpdateCombinedImageSampler(Gui_Rendering* p_rendering, Gui_GraphicsPipeline* p_pipeline, unsigned int set, unsigned int binding, unsigned int array_element, VkSampler sampler, VkImageView image_view, VkImageLayout image_layout);
void                        Gui_Rendering_UpdateSampledImage(Gui_Rendering* p_rendering, Gui_GraphicsPipeline* p_pipeline, unsigned int set, unsigned int binding, unsigned int array_element, VkImageView image_view, VkImageLayout image_layout);
void                        Gui_Rendering_UpdateStorageImage(Gui_Rendering* p_rendering, Gui_GraphicsPipeline* p_pipeline, unsigned int set, unsigned int binding, unsigned int array_element, VkImageView image_view, VkImageLayout image_layout);
void                        Gui_Rendering_UpdateStorageBuffer(Gui_Rendering* p_rendering, Gui_GraphicsPipeline* p_pipeline, unsigned int set, unsigned int binding, unsigned int array_element, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range);
