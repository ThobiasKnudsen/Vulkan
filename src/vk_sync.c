#include "vk.h"

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