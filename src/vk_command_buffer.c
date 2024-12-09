#include "vk.h"

VkCommandBuffer* vk_CommandBuffer_CreateForSwapchain(
    Vk* p_vk,
    VkDescriptorSet* p_desc_set, 
    size_t desc_set_count,
    VkPipeline graphics_pipeline,
    VkPipelineLayout graphics_pipeline_layout,
    VkBuffer instance_buffer,
    Image* p_image
) {

    VkCommandBufferAllocateInfo alloc_info = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = p_vk->command_pool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = p_vk->swap_chain.image_count
    };

    TRACK( VkCommandBuffer* command_buffers = alloc( NULL, sizeof(VkCommandBuffer) * p_vk->swap_chain.image_count ) );
    VERIFY( command_buffers, "Failed to allocate memory for command buffers\n" );

    TRACK( VkResult result = vkAllocateCommandBuffers( p_vk->device, &alloc_info, command_buffers ) );
    VERIFY( result == VK_SUCCESS, "Failed to allocate command buffers\n" );

    for (uint32_t i = 0; i < p_vk->swap_chain.image_count; i++) {

        VkCommandBufferBeginInfo begin_info = {0};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO; 

        TRACK( result = vkBeginCommandBuffer(command_buffers[i], &begin_info) );
        VERIFY( result == VK_SUCCESS, "failed to begin command buffer" );

        VkImageMemoryBarrier barrier_to_color_attachment = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = p_vk->swap_chain.images_p[i],
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };

        TRACK( vkCmdPipelineBarrier( command_buffers[i], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, NULL, 0, NULL, 1, &barrier_to_color_attachment ) );
        if (p_image->layout != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            TRACK( vk_Image_TransitionLayout( command_buffers[i], p_image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ) );
        }

        VkRenderingInfo rendering_info = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea = { 
                .offset = {0, 0}, 
                .extent = p_vk->swap_chain.extent 
            },
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &(VkRenderingAttachmentInfo){
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .imageView = p_vk->swap_chain.image_views_p[i],
                .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .clearValue = { .color = {0.2f, 0.0f, 0.0f, 1.0f} },
            },
        };
            
        TRACK( vkCmdBeginRendering( command_buffers[i], &rendering_info ) );
        TRACK( vkCmdBindPipeline( command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline ) );
        TRACK( vkCmdBindDescriptorSets( command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline_layout, 0, desc_set_count, p_desc_set, 0, NULL ) );
        TRACK( vkCmdBindVertexBuffers( command_buffers[i], 0, 1, (VkBuffer[]){instance_buffer}, (VkDeviceSize[]){0} ) );
        TRACK( vkCmdDraw( command_buffers[i], 4, INDICES_COUNT, 0, 0 ) );
        TRACK( vkCmdEndRendering( command_buffers[i] ) );

        VkImageMemoryBarrier barrier_to_present = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dstAccessMask = 0,
            .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = p_vk->swap_chain.images_p[i],
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };

        TRACK(vkCmdPipelineBarrier( command_buffers[i], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 1, &barrier_to_present));
        VERIFY(vkEndCommandBuffer( command_buffers[i]) == VK_SUCCESS, "failed to end command buffer");
    }

    return command_buffers;
}

VkCommandBuffer vk_CommandBuffer_CreateAndBeginSingleTimeUsage(Vk* p_vk) {
    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandPool = p_vk->command_pool,
        .commandBufferCount = 1,
    };
    
    VkCommandBuffer command_buffer;
    TRACK(vkAllocateCommandBuffers(p_vk->device, &alloc_info, &command_buffer));
    
    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    
    TRACK(vkBeginCommandBuffer(command_buffer, &beginInfo));
    
    return command_buffer;
}

void vk_CommandBuffer_EndAndDestroySingleTimeUsage(Vk* p_vk, VkCommandBuffer command_buffer) {
    TRACK(vkEndCommandBuffer(command_buffer));
    
    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer,
    };
    
    TRACK(vkQueueSubmit(p_vk->queues.graphics, 1, &submit_info, VK_NULL_HANDLE));
    TRACK(vkQueueWaitIdle(p_vk->queues.graphics));
    
    TRACK(vkFreeCommandBuffers(p_vk->device, p_vk->command_pool, 1, &command_buffer));
}


VkCommandBuffer vk_CommandBuffer_CreateWithImageAttachment(
    Vk* p_vk, 
    Image* p_image,
    VkDescriptorSet* p_desc_set, 
    size_t desc_set_count,
    VkPipeline graphics_pipeline,
    VkPipelineLayout graphics_pipeline_layout,
    VkBuffer instance_buffer,
    uint32_t vertex_count,
    uint32_t instance_count) 
{

    VERIFY(p_vk, "Vk pointer is NULL.");
    VERIFY(p_image, "Image pointer is NULL.");
    VERIFY(p_desc_set, "Descriptor Set pointer is NULL.");
    
    // Allocate a command buffer from the command pool
    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = p_vk->command_pool, // Ensure p_vk has a valid command_pool
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    
    VkCommandBuffer command_buffer;
    TRACK(vkAllocateCommandBuffers(p_vk->device, &alloc_info, &command_buffer));
    VERIFY(command_buffer != VK_NULL_HANDLE, "Failed to allocate command buffer.");
    
    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = NULL, // Optional for secondary command buffers
    };
    
    TRACK(vkBeginCommandBuffer(command_buffer, &begin_info));
    TRACK(vk_Image_TransitionLayout(command_buffer, p_image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
    
    // Setup color attachment for dynamic rendering
    VkRenderingAttachmentInfo color_attachment_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = p_image->image_view,
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = (VkClearValue) {
            .color = {0.0f, 0.0f, 0.0f, 1.0f} // Adjust as needed
        },
    };
    
    // Define the rendering area and attachments
    VkRenderingInfo rendering_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = {
            .offset = (VkOffset2D){0, 0},
            .extent = p_image->extent,
        },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_info,
        .pDepthAttachment = NULL,    // Optional: Add if using depth
        .pStencilAttachment = NULL,  // Optional: Add if using stencil
    };

    TRACK(vkCmdBeginRendering(command_buffer, &rendering_info));
    TRACK(vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline));
    TRACK(vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,  graphics_pipeline_layout, 0, (uint32_t)desc_set_count, p_desc_set, 0, NULL));
    
    VkDeviceSize offsets[] = { 0 };
    TRACK(vkCmdBindVertexBuffers(command_buffer, 0, 1, &instance_buffer, offsets));
    TRACK(vkCmdDraw(command_buffer, vertex_count, instance_count, 0, 0));
    TRACK(vkCmdEndRendering(command_buffer));
    TRACK(vk_Image_TransitionLayout(command_buffer, p_image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
    TRACK(vkEndCommandBuffer(command_buffer));

    return command_buffer;
}

void vk_CommandBuffer_Submit(Vk* p_vk, VkCommandBuffer command_buffer) {

    VERIFY(p_vk, "Vk pointer is NULL.");
    VERIFY(command_buffer != VK_NULL_HANDLE, "Command buffer is NULL.");

    VkFenceCreateInfo fence_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = 0, // No flags; create a standard fence
    };
    
    VkFence fence;
    TRACK(vkCreateFence(p_vk->device, &fence_info, NULL, &fence));
    VERIFY(fence != VK_NULL_HANDLE, "Failed to create fence.");
    
    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer,
        .waitSemaphoreCount = 0, // No semaphores to wait on
        .pWaitSemaphores = NULL,
        .pWaitDstStageMask = NULL,
        .signalSemaphoreCount = 0, // No semaphores to signal
        .pSignalSemaphores = NULL,
    };
    
    TRACK(vkQueueSubmit(p_vk->queues.graphics, 1, &submit_info, fence));
    VkResult result = vkWaitForFences(p_vk->device, 1, &fence, VK_TRUE, UINT64_MAX);
    VERIFY(result == VK_SUCCESS, "Failed to wait for fence.");
    TRACK(vkDestroyFence(p_vk->device, fence, NULL));
    
    // Optionally, reset the command buffer if you intend to reuse it
    // TRACK(vkResetCommandBuffer(command_buffer, 0));
}