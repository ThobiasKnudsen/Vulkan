#pragma once

#include <stdint.h>

#define VK_USE_PLATFORM_XLIB_KHR
#include <vulkan/vulkan.h>

void                Logos_Instance_Create();
void                Logos_Device_Create();
VkPipelineLayout    Logos_PipelineLayout_Create();
VkRenderPass        Logos_RenderPass_Create();
VkPipeline          Logos_GraphicsPipeline_Create(VkRenderPass renderPass, VkPipelineLayout pipelineLayout, VkShaderModule vertShaderModule, VkShaderModule fragShaderModule);
void                Logos_Window_Create(int width, int height, const char* title);
void                Logos_Swapchain_Create();
void                Logos_Window_Show();
void                Logos_Window_Delete();

void test();


