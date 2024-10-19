#ifndef VK_SETUP_H
#define VK_SETUP_H

#include <vulkan/vulkan.h>
#include <stdint.h>

/* Constants */
#define MAX_FRAMES_IN_FLIGHT 2

/* Data structures */
typedef struct {
    /* Core Vulkan Objects */
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkSurfaceKHR surface;

    /* Swapchain and Image Views */
    VkSwapchainKHR swapchain;
    VkImage* swapchainImages;
    VkImageView* swapchainImageViews;
    uint32_t swapchainImageCount;
    VkFormat swapchainImageFormat;
    VkExtent2D swapchainExtent;

    /* Pipeline */
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    /* Framebuffers */
    VkFramebuffer* framebuffers;

    /* Command Pool and Buffers */
    VkCommandPool commandPool;
    VkCommandBuffer* commandBuffers;

    /* Synchronization */
    VkSemaphore imageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
    VkSemaphore renderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];
    VkFence inFlightFences[MAX_FRAMES_IN_FLIGHT];
    size_t currentFrame;

} VK;

/* Function declarations */

/* Initialization and Cleanup */
int vk_initialize(VK* vk, void* window);
void vk_cleanup(VK* vk);


void l_create_window(int width, int height, const char* title);
void l_destroy_window();

/* Drawing */
void vk_draw_frame(VK* vk);

#endif // VK_SETUP_H
