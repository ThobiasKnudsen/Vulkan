#include "vk.h"

VkCommandBuffer vk_CommandBuffer_RecordStaticRendering(
    Vk* p_vk,
    Image* p_target_image,
    VkDescriptorSet* p_desc_sets, 
    size_t desc_sets_count,
    VkPipeline graphics_pipeline,
    VkPipelineLayout graphics_pipeline_layout,
    VkBuffer instance_buffer,
    size_t instances_count) 
{

    VERIFY(p_vk, "NULL pointer");
    VERIFY(p_target_image, "NULL pointer");
    VERIFY(p_desc_sets, "NULL pointer");
    VERIFY(desc_sets_count>0, "desc_sets_count is 0");
    VERIFY(instance_buffer!=VK_NULL_HANDLE, "VK_NULL_HANDLE");

    VkCommandBufferAllocateInfo alloc_info = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = p_vk->command_pool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    VkCommandBuffer command_buffer;
    TRACK(VkResult result = vkAllocateCommandBuffers(p_vk->device, &alloc_info, &command_buffer));
    VERIFY(result == VK_SUCCESS, "Failed to allocate command buffers\n");

    VkCommandBufferBeginInfo begin_info = {0};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO; 

    TRACK(result = vkBeginCommandBuffer(command_buffer, &begin_info));
    VERIFY(result == VK_SUCCESS, "failed to begin command buffer");

    VkImageMemoryBarrier barrier_to_color_attachment = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = p_target_image->image,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };

    TRACK(vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, NULL, 0, NULL, 1, &barrier_to_color_attachment));

    VkRenderingInfo rendering_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = { 
            .offset = {0, 0}, 
            .extent = p_target_image->extent 
        },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &(VkRenderingAttachmentInfo){
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = p_target_image->view,
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = { .color = {0.0f, 0.0f, 0.0f, 1.0f} },
        },
    };
        
    TRACK( vkCmdBeginRendering(command_buffer, &rendering_info ) );
    TRACK( vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline ) );
    TRACK( vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline_layout, 0, desc_sets_count, p_desc_sets, 0, NULL ) );
    TRACK( vkCmdBindVertexBuffers(command_buffer, 0, 1, (VkBuffer[]){instance_buffer}, (VkDeviceSize[]){0} ) );
    TRACK( vkCmdDraw(command_buffer, 4, instances_count, 0, 0 ) );
    // vkCmdDrawIndirect
    TRACK( vkCmdEndRendering(command_buffer) );

    VkImageMemoryBarrier barrier_to_present = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = 0,
        .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = p_target_image->image,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };

    TRACK(vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 1, &barrier_to_present));
    VERIFY(vkEndCommandBuffer(command_buffer) == VK_SUCCESS, "failed to end command buffer");

    return command_buffer;
}

VkCommandBuffer* vk_CommandBuffer_CreateForSwapchain(
    Vk* p_vk,
    VkDescriptorSet* p_desc_sets, 
    size_t desc_sets_count,
    VkPipeline graphics_pipeline,
    VkPipelineLayout graphics_pipeline_layout,
    VkBuffer instance_buffer,
    Image* p_image
) {

    VkCommandBufferAllocateInfo alloc_info = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = p_vk->command_pool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = p_vk->images_count
    };

    TRACK(VkCommandBuffer* command_buffers = alloc( NULL, sizeof(VkCommandBuffer) * p_vk->images_count));
    VERIFY(command_buffers, "Failed to allocate memory for command buffers\n");

    TRACK(VkResult result = vkAllocateCommandBuffers( p_vk->device, &alloc_info, command_buffers));
    VERIFY(result == VK_SUCCESS, "Failed to allocate command buffers\n");

    for (unsigned int i = 0; i < p_vk->images_count; ++i) {
        if (p_image->layout != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            TRACK(vk_Image_TransitionLayoutWithoutCommandBuffer(p_vk, p_image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
        }
    }

    for (uint32_t i = 0; i < p_vk->images_count; i++) {
        command_buffers[i] = vk_CommandBuffer_RecordStaticRendering(
            p_vk, 
            &p_vk->p_images[i], 
            p_desc_sets, 
            desc_sets_count, 
            graphics_pipeline, 
            graphics_pipeline_layout, 
            instance_buffer, 
            5);
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
