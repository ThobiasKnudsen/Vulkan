#include "vk.h"

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    VK vk = VK_create(800, 600, "Vulkan GUI");

    Buffer                  uniform_buffer = createBuffer(&vk, sizeof(UniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    Buffer                  instance_buffer = createBuffer(&vk, sizeof(InstanceData) * ALL_INSTANCE_COUNT, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    VkDescriptorSetLayout   desc_set_layout = createDescriptorSetLayout_0(&vk);
    VkDescriptorSet         desc_set = createDescriptorSet(&vk, desc_set_layout, uniform_buffer.buffer);
    
    debug(VkPipelineLayout graphicsPipelineLayout = createPipelineLayout(vk.device, &desc_set_layout, 1));
    debug(VkPipeline graphicsPipeline = createGraphicsPipeline(&vk, graphicsPipelineLayout));
    debug(VkCommandBuffer* swapChainCommandBuffers = createCommandBuffersForSwapchain(
        &vk,
        desc_set, 
        graphicsPipeline,
        graphicsPipelineLayout,
        instance_buffer.buffer
    ));
    debug(VkSemaphore imageAvailableSemaphore = createSemaphore(vk.device));
    debug(VkSemaphore renderFinishedSemaphore = createSemaphore(vk.device));
    debug(VkFence inFlightFence = createFence(vk.device));
    debug(VK_startApp(
        &vk,
        imageAvailableSemaphore,
        renderFinishedSemaphore,
        inFlightFence,
        swapChainCommandBuffers,
        uniform_buffer.buffer,
        uniform_buffer.allocation,
        instance_buffer
    ));
    debug(VK_destroy(
        &vk,
        graphicsPipeline,
        graphicsPipelineLayout,
        desc_set_layout,
        uniform_buffer.buffer,
        uniform_buffer.allocation,
        instance_buffer.buffer,
        instance_buffer.allocation,
        desc_set,
        swapChainCommandBuffers,
        imageAvailableSemaphore,
        renderFinishedSemaphore,
        inFlightFence
    ));

    return 0;
}
