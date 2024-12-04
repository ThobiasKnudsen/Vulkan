#include "vk.h"

VkCommandBuffer* createCommandBuffersForSwapchain(
    VK* vk,
    VkDescriptorSet desc_set,
    VkPipeline graphics_pipeline,
    VkPipelineLayout graphics_pipeline_layout,
    VkBuffer instance_buffer
) {
    VkCommandBufferAllocateInfo allocInfo = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = vk->command_pool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = vk->swap_chain.image_count
    };

    VkCommandBuffer* command_buffers = alloc(NULL, sizeof(VkCommandBuffer) * vk->swap_chain.image_count);
    if (command_buffers == NULL) {
        printf("Failed to allocate memory for command buffers\n");
        exit(EXIT_FAILURE);
    }

    if (vkAllocateCommandBuffers(vk->device, &allocInfo, command_buffers) != VK_SUCCESS) {
        printf("Failed to allocate command buffers\n");
        free(command_buffers);
        exit(EXIT_FAILURE);
    }

    for (uint32_t i = 0; i < vk->swap_chain.image_count; i++) {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(command_buffers[i], &beginInfo) != VK_SUCCESS) {
            printf("Failed to begin recording command buffer %u\n", i);
            exit(EXIT_FAILURE);
        }

        // Transition to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        VkImageMemoryBarrier barrierToColorAttachment = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = vk->swap_chain.images_p[i],
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };

        vkCmdPipelineBarrier(
            command_buffers[i],
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            0,
            0, NULL,
            0, NULL,
            1, &barrierToColorAttachment
        );

        VkRenderingInfo renderingInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea = { 
                .offset = {0, 0}, 
                .extent = vk->swap_chain.extent 
            },
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &(VkRenderingAttachmentInfo){
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .imageView = vk->swap_chain.image_views_p[i],
                .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .clearValue = { .color = {0.2f, 0.0f, 0.0f, 1.0f} },
            },
        };
            
        vkCmdBeginRendering(command_buffers[i], &renderingInfo);
        vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);
        vkCmdBindDescriptorSets(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline_layout, 0, 1, &desc_set, 0, NULL);
        vkCmdBindVertexBuffers(command_buffers[i], 0, 1, (VkBuffer[]){instance_buffer}, (VkDeviceSize[]){0});
        vkCmdDraw(command_buffers[i], 4, INDICES_COUNT, 0, 0);
        vkCmdEndRendering(command_buffers[i]);

        // Transition to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        VkImageMemoryBarrier barrierToPresent = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dstAccessMask = 0,
            .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = vk->swap_chain.images_p[i],
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };

        vkCmdPipelineBarrier(
            command_buffers[i],
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            0,
            0, NULL,
            0, NULL,
            1, &barrierToPresent
        );

        if (vkEndCommandBuffer(command_buffers[i]) != VK_SUCCESS) {
            printf("Failed to record command buffer %u\n", i);
            exit(EXIT_FAILURE);
        }
    }

    return command_buffers;
}