// vulkan_gui.h
#ifndef VULKAN_GUI_H
#define VULKAN_GUI_H

#include <vulkan/vulkan.h>
#include <shaderc/shaderc.h> // Added to define shaderc_shader_kind
#include <vk_mem_alloc.h>

// Constants
#define ALL_INSTANCE_COUNT 5
#define INDICES_COUNT 3

//typedef struct VK VK;

// Structures
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


// Structure to represent any type of image
typedef struct {
    VkImage image;
    VkDeviceMemory memory;
    VkImageView imageView;
    VkSampler sampler;
    VkFormat format;
    VkExtent3D extent;
    VkImageLayout layout;
} VulkanImage;

VulkanImage                 createVulkanImage( VkDevice device, VkPhysicalDevice physicalDevice, VkFormat format, VkExtent3D extent, VkImageUsageFlags usage, VkMemoryPropertyFlags properties );
VkImageView                 createImageView( VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags );
VkSampler                   createImageSampler( VkDevice device );
VulkanImage                 loadImageFromFile( VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue, const char* filename );
VulkanImage                 createImageAtlas( VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue, const char** filenames, uint32_t imageCount );
void                        copyBufferToImage( VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height );
void                        transitionImageLayout( VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout );
void                        destroyVulkanImage( VkDevice device, VulkanImage* vulkanImage );

void                        cleanup(VmaAllocator allocator,VkDevice device,VkInstance instance,VkSurfaceKHR surface,VkDebugUtilsMessengerEXT debugMessenger,VkPipeline graphicsPipeline,VkPipelineLayout pipelineLayout,VkRenderPass renderPass,VkDescriptorSetLayout descriptorSetLayout,VkDescriptorPool descriptorPool,VkBuffer uniformBuffer,VmaAllocation uniformBufferAllocation,VkBuffer instanceBuffer,VmaAllocation instanceBufferAllocation,VkDescriptorSet descriptorSet,VkSwapchainKHR swapChain,VkImageView* swapChainImageViews,uint32_t swapChainImageCount,VkFramebuffer* swapChainFramebuffers,VkCommandPool commandPool,VkCommandBuffer* commandBuffers,VkSemaphore imageAvailableSemaphore,VkSemaphore renderFinishedSemaphore,VkFence inFlightFence,void* window);
unsigned int                readShaderSource(const char* filename, char** buffer);
VkShaderModule              createShaderModule(const char* filename, shaderc_shader_kind kind, VkDevice device);
VkInstance                  createVulkanInstance(void* window);
VkDebugUtilsMessengerEXT    setupDebugMessenger(VkInstance instance);
VkSurfaceKHR                createSurface(void* window, VkInstance instance);
VkPhysicalDevice            getPhysicalDevice(VkInstance instance);
VkQueueFamilyIndices        getQueueFamilyIndices(VkSurfaceKHR surface, VkPhysicalDevice physicalDevice);
VkDevice                    getDevice(VkPhysicalDevice physicalDevice, VkQueueFamilyIndices indices);
VkQueues                    getQueues(VkDevice device, VkQueueFamilyIndices indices);
VkDescriptorSetLayout       createDescriptorSetLayout(VkDevice device);
VkPipelineLayout            createPipelineLayout(VkDevice device, VkDescriptorSetLayout layout);
VkSwapchainKHR              createSwapChain(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkQueueFamilyIndices indices, VkExtent2D extent, VkFormat* swapChainImageFormat);
VkRenderPass                createRenderPass(VkDevice device, VkFormat imageFormat);
VkPipeline                  createGraphicsPipeline(VkDevice device, VkPipelineLayout pipelineLayout, VkRenderPass renderPass, VkExtent2D swapChainExtent);
VkImageView*                createImageViews(VkDevice device, VkSwapchainKHR swapChain, VkFormat imageFormat, uint32_t imageCount);
VkFramebuffer*              createFramebuffers(VkDevice device, VkRenderPass renderPass, VkImageView* imageViews, uint32_t imageCount, VkExtent2D extent);
VkBuffer                    createUniformBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceMemory* bufferMemory);
VkDescriptorPool            createDescriptorPool(VkDevice device);
VkDescriptorSet             createDescriptorSet(VkDevice device, VkDescriptorPool pool, VkDescriptorSetLayout layout, VkBuffer uniformBuffer);
VkCommandPool               createCommandPool(VkDevice device, uint32_t graphicsFamily);
VkBuffer                    createInstanceBuffer(VkDevice device, VkPhysicalDevice physicalDevice, InstanceData* instances, size_t instanceCount, VkDeviceMemory* bufferMemory);
VkCommandBuffer*            createCommandBuffers(VkDevice device, VkCommandPool commandPool, VkPipeline graphicsPipeline, VkPipelineLayout pipelineLayout, VkRenderPass renderPass, VkFramebuffer* framebuffers, uint32_t imageCount, VkBuffer instanceBuffer, VkDescriptorSet descriptorSet, VkExtent2D swapChainExtent);
VkSemaphore                 createSemaphore(VkDevice device);
VkFence                     createFence(VkDevice device);
void                        mainLoop(VmaAllocator allocator,VkDevice device,VkQueue graphicsQueue,VkQueue presentQueue,VkSwapchainKHR swapChain,VkSemaphore imageAvailableSemaphore,VkSemaphore renderFinishedSemaphore,VkFence inFlightFence,VkCommandBuffer* commandBuffers,uint32_t imageCount,VkDescriptorSet descriptorSet,VkExtent2D swapChainExtent,VkBuffer uniformBuffer,VmaAllocation uniformBufferAllocation);
void                        test();

#endif // VULKAN_GUI_H
