#include "vk.h"

Vk_Rendering Vk_Rendering_Create()
{
    Vk_Rendering rendering = {0};
    rendering.command_buffer_needs_recording = true;
    return rendering;
}

void Vk_Rendering_SetGraphicsPipeline(
    Vk_Rendering* p_rendering,
    Vk_GraphicsPipeline* p_pipeline, VkBuffer buffer, Image* p_image)
{
    VERIFY(p_rendering, "NULL pointer");
    VERIFY(p_pipeline, "NULL pointer");
    VERIFY(p_pipeline->p_vk, "NULL pointer");
    VERIFY(p_pipeline->p_shaders, "NULL pointer");
    VERIFY(p_pipeline->shaders_count > 0, "There are 0 shaders in pipeline");
    VERIFY(p_pipeline->pipeline_layout != VK_NULL_HANDLE, "pipeline_layout is VK_NULL_HANDLE");
    VERIFY(p_pipeline->graphics_pipeline != VK_NULL_HANDLE, "graphics_pipeline is VK_NULL_HANDLE");
    for (size_t i = 0; i < p_pipeline->desc_sets_count; ++i) {
        VERIFY(p_pipeline->p_desc_sets_layout[i] != VK_NULL_HANDLE, "Invalid VkDescriptorSetLayout handle at index %zu", i);
    }

    // Creating descriptor sets
    if (p_rendering->p_desc_sets) {
        TRACK(VkResult result = vkFreeDescriptorSets(p_pipeline->p_vk->device, p_pipeline->p_vk->descriptor_pool, p_rendering->desc_sets_count, p_rendering->p_desc_sets));
        VERIFY(result == VK_SUCCESS, "Failed to free descriptor sets");
        TRACK(free(p_rendering->p_desc_sets)); 
        p_rendering->p_desc_sets = NULL; 
    }

    TRACK(p_rendering->p_desc_sets = vk_DescriptorSet_Create_0(p_pipeline->p_vk, p_pipeline->p_desc_sets_layout, p_pipeline->desc_sets_count, buffer, p_image));
    p_rendering->desc_sets_count = p_pipeline->desc_sets_count;
    p_rendering->p_vk = p_pipeline->p_vk; 
    p_rendering->p_pipeline = p_pipeline; 
    p_rendering->command_buffer_needs_recording = true;
}

void Vk_Rendering_SetTargetImage(
    Vk_Rendering* p_rendering, 
    Image* p_target_image) 
{
    VERIFY(p_rendering, "NULL pointer");
    VERIFY(p_target_image, "NULL pointer");

    VERIFY(p_target_image, "NULL pointer passed to Vk_Rendering_Create");
    VERIFY(p_target_image->image != VK_NULL_HANDLE, "Target image handle is VK_NULL_HANDLE");
    VERIFY(p_target_image->extent.width != 0, "Target image width is zero");
    VERIFY(p_target_image->extent.height != 0, "Target image height is zero");
    VERIFY(p_target_image->format != VK_FORMAT_UNDEFINED, "Target image format is VK_FORMAT_UNDEFINED");

    if (p_rendering->command_buffer != VK_NULL_HANDLE) {
        TRACK(vkFreeCommandBuffers(p_rendering->p_vk->device, p_rendering->p_vk->command_pool, 1, &p_rendering->command_buffer));
    }

    p_rendering->p_target_image = p_target_image;
    p_rendering->command_buffer_needs_recording = true;
}

void Vk_Rendering_UpdateInstanceBuffer(
    Vk_Rendering* p_rendering, 
    size_t dst_offset, 
    void* p_src_data, 
    size_t size) 
{
    VERIFY(p_rendering, "NULL pointer");
    VERIFY(p_rendering->p_vk, "NULL pointer");
    VERIFY(p_src_data, "NULL pointer passed as source data");
    VERIFY(size > 0, "Size to update must be greater than zero");

    if (p_rendering->instance_buffer.size < dst_offset + size || p_rendering->instance_buffer.buffer == VK_NULL_HANDLE) {
        Buffer tmp_buffer = p_rendering->instance_buffer;
        p_rendering->instance_buffer = vk_Buffer_Create(p_rendering->p_vk, dst_offset+size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        if (tmp_buffer.buffer != VK_NULL_HANDLE && tmp_buffer.size > 0) {
            TRACK(vk_Buffer_CopyBuffer(p_rendering->p_vk, p_rendering->instance_buffer, tmp_buffer, 0, 0, tmp_buffer.size));
        }
        TRACK(vmaDestroyBuffer(p_rendering->p_vk->allocator, tmp_buffer.buffer, tmp_buffer.allocation));
        p_rendering->command_buffer_needs_recording = true;
    }
    TRACK(vk_Buffer_Update(p_rendering->p_vk, p_rendering->instance_buffer, dst_offset, p_src_data, size));
}

void Vk_Rendering_UpdateInstanceBufferWithBuffer(
    Vk_Rendering* p_rendering, 
    size_t dst_offset, 
    Buffer src_buffer, 
    size_t src_offset, 
    size_t size) 
{
    VERIFY(p_rendering, "NULL pointer");
    VERIFY(p_rendering->p_vk, "NULL pointer");
    VERIFY(src_buffer.buffer != VK_NULL_HANDLE, "Source buffer not initialized");
    VERIFY(size > 0, "Size to update must be greater than zero");

    if (p_rendering->instance_buffer.size < dst_offset + size || p_rendering->instance_buffer.buffer == VK_NULL_HANDLE) {
        Buffer tmp_buffer = p_rendering->instance_buffer;
        TRACK(p_rendering->instance_buffer = vk_Buffer_Create(p_rendering->p_vk, dst_offset+size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT));
        if (tmp_buffer.buffer != VK_NULL_HANDLE && tmp_buffer.size > 0) {
            TRACK(vk_Buffer_CopyBuffer(p_rendering->p_vk, p_rendering->instance_buffer, tmp_buffer, 0, 0, tmp_buffer.size));
        }
        TRACK(vmaDestroyBuffer(p_rendering->p_vk->allocator, tmp_buffer.buffer, tmp_buffer.allocation));
        if (p_rendering->command_buffer != VK_NULL_HANDLE) {
            TRACK(vkFreeCommandBuffers(p_rendering->p_vk->device, p_rendering->p_vk->command_pool, 1, &p_rendering->command_buffer));
        }
        p_rendering->command_buffer_needs_recording = true;
    }
    TRACK(vk_Buffer_CopyBuffer(p_rendering->p_vk, src_buffer, p_rendering->instance_buffer, src_offset, dst_offset, size));
}

void Vk_Rendering_UpdateInstanceDrawRange(
    Vk_Rendering* p_rendering,
    unsigned int first_instance,
    unsigned int instance_count)
{
    VERIFY(p_rendering, "NULL pointer passed to Vk_Rendering_UpdateIndirectBuffer");

    if (p_rendering->indirect_buffer.size == 0 && p_rendering->indirect_buffer.buffer == VK_NULL_HANDLE) {
        TRACK(p_rendering->indirect_buffer = vk_Buffer_Create(p_rendering->p_vk, sizeof(VkDrawIndirectCommand), VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT));
        if (p_rendering->command_buffer == VK_NULL_HANDLE) {
            TRACK(vkFreeCommandBuffers(p_rendering->p_vk->device, p_rendering->p_vk->command_pool, 1, &p_rendering->command_buffer));
        }
        p_rendering->command_buffer_needs_recording = true;
    }

    VERIFY(p_rendering->indirect_buffer.size != 0, "buffer size cannot be 0 when VkBuffer is not NULL");
    VERIFY(p_rendering->indirect_buffer.buffer != VK_NULL_HANDLE, "VkBuffer cannot be NULL when buffer size is not 0");

    VkDrawIndirectCommand draw_cmd = {
        .vertexCount = 4, 
        .instanceCount = instance_count,
        .firstVertex = 0, 
        .firstInstance = first_instance
    };

    TRACK(vk_Buffer_Update(p_rendering->p_vk, p_rendering->indirect_buffer, 0, &draw_cmd, sizeof(VkDrawIndirectCommand)));
}

void Vk_Rendering_RecordCommandBuffer(
    Vk_Rendering* p_rendering)
{
    VERIFY(p_rendering, "Pipeline is a NULL pointer");
    if (!p_rendering->command_buffer_needs_recording) {
        printf("command_buffer in rendering is already recorded and there is no updates which should make you want to update it either");
        return;
    }
    VERIFY(p_rendering->p_pipeline, "Pipeline is a NULL pointer");
    VERIFY(p_rendering->p_pipeline->p_vk, "p_vk is a NULL pointer");
    VERIFY(p_rendering->p_pipeline->p_shaders, "Shaders pointer is NULL in pipeline");
    VERIFY(p_rendering->p_pipeline->shaders_count > 0, "There are 0 shaders in pipeline");
    VERIFY(p_rendering->p_pipeline->pipeline_layout != VK_NULL_HANDLE, "pipeline_layout is VK_NULL_HANDLE");
    VERIFY(p_rendering->p_pipeline->graphics_pipeline != VK_NULL_HANDLE, "graphics_pipeline is VK_NULL_HANDLE");
    VERIFY(p_rendering->instance_buffer.buffer != VK_NULL_HANDLE, "graphics_pipeline is VK_NULL_HANDLE");
    VERIFY(p_rendering->instance_buffer.size != 0, "instance_buffer size is zero"); // Changed from VK_NULL_HANDLE to 0
    VERIFY(p_rendering->indirect_buffer.buffer != VK_NULL_HANDLE, "graphics_pipeline is VK_NULL_HANDLE");
    VERIFY(p_rendering->indirect_buffer.size != 0, "indirect_buffer size is zero"); // Changed from VK_NULL_HANDLE to 0

    VERIFY(p_rendering->p_target_image, "NULL pointer passed to Vk_Rendering_Create");
    VERIFY(p_rendering->p_target_image->image != VK_NULL_HANDLE, "Target image handle is VK_NULL_HANDLE");
    VERIFY(p_rendering->p_target_image->extent.width != 0, "Target image width is zero");
    VERIFY(p_rendering->p_target_image->extent.height != 0, "Target image height is zero");
    VERIFY(p_rendering->p_target_image->format != VK_FORMAT_UNDEFINED, "Target image format is VK_FORMAT_UNDEFINED");

    bool should_rerecord = p_rendering->command_buffer != VK_NULL_HANDLE;
    
    if (!should_rerecord) {
        // Allocate if not done yet
        VkCommandBufferAllocateInfo alloc_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = p_rendering->p_pipeline->p_vk->command_pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1
        };

        TRACK(VkResult result = vkAllocateCommandBuffers(p_rendering->p_pipeline->p_vk->device, &alloc_info, &p_rendering->command_buffer));
        VERIFY(result == VK_SUCCESS, "Failed to allocate command buffer for GUI rendering");
    } else {
        // Reset the command buffer for re-recording
        VkCommandBufferResetFlags flags = VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT;
        VkResult result = vkResetCommandBuffer(p_rendering->command_buffer, flags);
        VERIFY(result == VK_SUCCESS, "Failed to reset command buffer for re-recording");
    }

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
        .pInheritanceInfo = NULL
    };
    TRACK(VkResult result = vkBeginCommandBuffer(p_rendering->command_buffer, &begin_info));
    VERIFY(result == VK_SUCCESS, "Failed to begin command buffer for GUI rendering");

    //TRACK(vk_Image_TransitionLayout(p_rendering->command_buffer, p_rendering->p_target_image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));

    VkImageMemoryBarrier barrier_to_color_attachment = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = p_rendering->p_target_image->image,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };

    TRACK(vkCmdPipelineBarrier(p_rendering->command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, NULL, 0, NULL, 1, &barrier_to_color_attachment));


    VkRenderingInfo rendering_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = {
            .offset = {0, 0},
            .extent = p_rendering->p_target_image->extent
        },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &(VkRenderingAttachmentInfo) {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = p_rendering->p_target_image->view,
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .resolveMode = VK_RESOLVE_MODE_NONE,
            .resolveImageView = VK_NULL_HANDLE,
            .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = { .color = {0.0f, 0.0f, 0.0f, 1.0f} },
        },
        .pDepthAttachment = VK_NULL_HANDLE,
        .pStencilAttachment = VK_NULL_HANDLE,
    };

    TRACK(vkCmdBeginRendering(p_rendering->command_buffer, &rendering_info));
    TRACK(vkCmdBindPipeline(p_rendering->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, p_rendering->p_pipeline->graphics_pipeline));
    TRACK(vkCmdBindDescriptorSets(p_rendering->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, p_rendering->p_pipeline->pipeline_layout, 0, (unsigned int)p_rendering->desc_sets_count, p_rendering->p_desc_sets, 0, NULL));

    VkViewport viewport = {
        .x = 0.0f, 
        .y = 0.0f, 
        .width = (float)p_rendering->p_target_image->extent.width, 
        .height = (float)p_rendering->p_target_image->extent.height, 
        .minDepth = 0.0f, 
        .maxDepth = 1.0f
    };
    TRACK(vkCmdSetViewport(p_rendering->command_buffer, 0, 1, &viewport));

    VkRect2D scissor = {
        .offset = {0, 0},  
        .extent = p_rendering->p_target_image->extent
    };
    TRACK(vkCmdSetScissor(p_rendering->command_buffer, 0, 1, &scissor));
    TRACK( vkCmdBindVertexBuffers(p_rendering->command_buffer, 0, 1, (VkBuffer[]){p_rendering->instance_buffer.buffer}, (VkDeviceSize[]){0} ) );
    TRACK(vkCmdDraw(p_rendering->command_buffer, 4,5,0,0));
    //TRACK(vkCmdDrawIndirect(p_rendering->command_buffer, p_rendering->indirect_buffer.buffer, 0, 1, sizeof(VkDrawIndirectCommand)));

    TRACK(vkCmdEndRendering(p_rendering->command_buffer));

    VkImageMemoryBarrier barrier_to_present = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = 0,
        .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = p_rendering->p_target_image->image,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };

    TRACK(vkCmdPipelineBarrier(p_rendering->command_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 1, &barrier_to_present));
    
    VERIFY(vkEndCommandBuffer(p_rendering->command_buffer) == VK_SUCCESS, "Failed to end command buffer for GUI rendering");

    p_rendering->command_buffer_needs_recording = false;
}

void Vk_Rendering_RecordCommandBuffer_0(
    Vk_Rendering* p_rendering)
{
    VERIFY(p_rendering, "Pipeline is a NULL pointer");
    if (!p_rendering->command_buffer_needs_recording) {
        printf("command_buffer in rendering is already recorded and there is no updates which should make you want to update it either");
        return;
    }
    VERIFY(p_rendering->p_pipeline, "Pipeline is a NULL pointer");
    VERIFY(p_rendering->p_pipeline->p_vk, "p_vk is a NULL pointer");
    VERIFY(p_rendering->p_pipeline->p_shaders, "Shaders pointer is NULL in pipeline");
    VERIFY(p_rendering->p_pipeline->shaders_count > 0, "There are 0 shaders in pipeline");
    VERIFY(p_rendering->p_pipeline->pipeline_layout != VK_NULL_HANDLE, "pipeline_layout is VK_NULL_HANDLE");
    VERIFY(p_rendering->p_pipeline->graphics_pipeline != VK_NULL_HANDLE, "graphics_pipeline is VK_NULL_HANDLE");
    VERIFY(p_rendering->instance_buffer.buffer != VK_NULL_HANDLE, "graphics_pipeline is VK_NULL_HANDLE");
    VERIFY(p_rendering->instance_buffer.size != 0, "instance_buffer size is zero"); // Changed from VK_NULL_HANDLE to 0
    VERIFY(p_rendering->indirect_buffer.buffer != VK_NULL_HANDLE, "graphics_pipeline is VK_NULL_HANDLE");
    VERIFY(p_rendering->indirect_buffer.size != 0, "indirect_buffer size is zero"); // Changed from VK_NULL_HANDLE to 0

    VERIFY(p_rendering->p_target_image, "NULL pointer passed to Vk_Rendering_Create");
    VERIFY(p_rendering->p_target_image->image != VK_NULL_HANDLE, "Target image handle is VK_NULL_HANDLE");
    VERIFY(p_rendering->p_target_image->extent.width != 0, "Target image width is zero");
    VERIFY(p_rendering->p_target_image->extent.height != 0, "Target image height is zero");
    VERIFY(p_rendering->p_target_image->format != VK_FORMAT_UNDEFINED, "Target image format is VK_FORMAT_UNDEFINED");

    bool should_rerecord = p_rendering->command_buffer != VK_NULL_HANDLE;
    
    if (!should_rerecord) {
        // Allocate if not done yet
        VkCommandBufferAllocateInfo alloc_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = p_rendering->p_pipeline->p_vk->command_pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1
        };

        TRACK(VkResult result = vkAllocateCommandBuffers(p_rendering->p_pipeline->p_vk->device, &alloc_info, &p_rendering->command_buffer));
        VERIFY(result == VK_SUCCESS, "Failed to allocate command buffer for GUI rendering");
    } else {
        // Reset the command buffer for re-recording
        VkCommandBufferResetFlags flags = VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT;
        VkResult result = vkResetCommandBuffer(p_rendering->command_buffer, flags);
        VERIFY(result == VK_SUCCESS, "Failed to reset command buffer for re-recording");
    }

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
        .pInheritanceInfo = NULL
    };
    TRACK(VkResult result = vkBeginCommandBuffer(p_rendering->command_buffer, &begin_info));
    VERIFY(result == VK_SUCCESS, "Failed to begin command buffer for GUI rendering");

    // Image layout transitions remain the same
    VkImageMemoryBarrier barrier_to_color_attachment = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = p_rendering->p_target_image->image,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };

    TRACK(vkCmdPipelineBarrier(p_rendering->command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, NULL, 0, NULL, 1, &barrier_to_color_attachment));

    VkRenderingInfo rendering_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = {
            .offset = {0, 0},
            .extent = p_rendering->p_target_image->extent
        },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &(VkRenderingAttachmentInfo) {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = p_rendering->p_target_image->view,
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .resolveMode = VK_RESOLVE_MODE_NONE,
            .resolveImageView = VK_NULL_HANDLE,
            .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = { .color = {0.0f, 0.0f, 0.0f, 1.0f} },
        },
        .pDepthAttachment = VK_NULL_HANDLE,
        .pStencilAttachment = VK_NULL_HANDLE,
    };

    TRACK(vkCmdBeginRendering(p_rendering->command_buffer, &rendering_info));
    TRACK(vkCmdBindPipeline(p_rendering->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, p_rendering->p_pipeline->graphics_pipeline));
    TRACK(vkCmdBindDescriptorSets(p_rendering->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, p_rendering->p_pipeline->pipeline_layout, 0, (unsigned int)p_rendering->desc_sets_count, p_rendering->p_desc_sets, 0, NULL));

    // Removed dynamic state commands since they're now set in the pipeline
    //TRACK(vkCmdSetViewport(p_rendering->command_buffer, 0, 1, &viewport));
    //TRACK(vkCmdSetScissor(p_rendering->command_buffer, 0, 1, &scissor));

    TRACK(vkCmdBindVertexBuffers(p_rendering->command_buffer, 0, 1, (VkBuffer[]){p_rendering->instance_buffer.buffer}, (VkDeviceSize[]){0}));
    
    // Use either vkCmdDraw or vkCmdDrawIndirect based on your needs
    TRACK(vkCmdDraw(p_rendering->command_buffer, 4, 5, 0, 0));
    // or if using indirect drawing:
    // TRACK(vkCmdDrawIndirect(p_rendering->command_buffer, p_rendering->indirect_buffer.buffer, 0, 1, sizeof(VkDrawIndirectCommand)));

    TRACK(vkCmdEndRendering(p_rendering->command_buffer));

    // Image layout transitions remain the same
    VkImageMemoryBarrier barrier_to_present = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = 0,
        .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = p_rendering->p_target_image->image,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };

    TRACK(vkCmdPipelineBarrier(p_rendering->command_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 1, &barrier_to_present));
    
    VERIFY(vkEndCommandBuffer(p_rendering->command_buffer) == VK_SUCCESS, "Failed to end command buffer for GUI rendering");

    p_rendering->command_buffer_needs_recording = false;
}

void Vk_Rendering_UpdateUniformBuffer(
    Vk_Rendering* p_rendering,
    unsigned int set,
    unsigned int binding,
    unsigned int array_element,
    VkBuffer buffer,
    unsigned int offset,
    unsigned int range) 
{
    VERIFY(p_rendering, "NULL pointer");
    Vk_GraphicsPipeline* p_pipeline = p_rendering->p_pipeline;
    VERIFY(p_pipeline, "NULL pointer");
    VERIFY(p_pipeline->desc_sets_count == p_rendering->desc_sets_count, "not same desc sets size");
    VERIFY(set < p_pipeline->desc_sets_count, "Set index out of range");
    VERIFY(binding < p_pipeline->p_desc_sets_layout_create_info[set].bindingCount, "Binding index out of range");

    unsigned int binding_index = UINT32_MAX; // Sentinel value, assuming no binding will exceed this

    for (unsigned int i = 0; i < p_pipeline->p_desc_sets_layout_create_info[set].bindingCount; ++i) {
        if (binding == p_pipeline->p_desc_sets_layout_create_info[set].pBindings[i].binding) {
            binding_index = i;
            break;
        }
    }

    VERIFY(binding_index != UINT32_MAX, "Could not find the specified binding");
    VERIFY(p_pipeline->p_desc_sets_layout_create_info[set].pBindings[binding_index].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, "Binding is not a uniform buffer type");
    VERIFY(array_element < p_pipeline->p_desc_sets_layout_create_info[set].pBindings[binding_index].descriptorCount, "Array element is out of bounds for this binding");

    VkWriteDescriptorSet desc_write = {
        .sType              = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet             = p_rendering->p_desc_sets[set],
        .dstBinding         = binding,
        .dstArrayElement    = array_element,
        .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount    = 1,
        .pBufferInfo        = &(VkDescriptorBufferInfo){
            .buffer = buffer,
            .offset = offset,
            .range  = range,
        }
    };

    TRACK(vkUpdateDescriptorSets(p_rendering->p_vk->device, 1, &desc_write, 0, NULL));
}

void Vk_Rendering_UpdateCombinedImageSampler(
    Vk_Rendering* p_rendering,
    unsigned int set,
    unsigned int binding,
    unsigned int array_element,
    Image* p_image)
{
    VERIFY(p_rendering, "NULL pointer");
    VERIFY(p_image, "NULL pointer");
    Vk_GraphicsPipeline* p_pipeline = p_rendering->p_pipeline;
    VERIFY(p_pipeline, "NULL pointer");
    VERIFY(p_pipeline->desc_sets_count == p_rendering->desc_sets_count, "not same desc sets size");
    VERIFY(set < p_pipeline->desc_sets_count, "Set index out of range");
    VERIFY(binding < p_pipeline->p_desc_sets_layout_create_info[set].bindingCount, "Binding index out of range");

    unsigned int binding_index = UINT32_MAX;

    for (unsigned int i = 0; i < p_pipeline->p_desc_sets_layout_create_info[set].bindingCount; ++i) {
        if (binding == p_pipeline->p_desc_sets_layout_create_info[set].pBindings[i].binding) {
            binding_index = i;
            break;
        }
    }

    VERIFY(binding_index != UINT32_MAX, "Could not find the specified binding");
    VERIFY(p_pipeline->p_desc_sets_layout_create_info[set].pBindings[binding_index].descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, "Binding is not of type COMBINED_IMAGE_SAMPLER");
    VERIFY(array_element < p_pipeline->p_desc_sets_layout_create_info[set].pBindings[binding_index].descriptorCount, "Array element is out of bounds");

    VkDescriptorImageInfo image_info = {
        .sampler     = p_image->sampler,
        .imageView   = p_image->view,
        .imageLayout = p_image->layout
    };

    VkWriteDescriptorSet desc_write = {
        .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet           = p_rendering->p_desc_sets[set],
        .dstBinding       = binding,
        .dstArrayElement  = array_element,
        .descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount  = 1,
        .pImageInfo       = &image_info
    };

    TRACK(vkUpdateDescriptorSets(p_rendering->p_vk->device, 1, &desc_write, 0, NULL));
}

void Vk_Rendering_UpdateSampledImage(
    Vk_Rendering* p_rendering,
    unsigned int set,
    unsigned int binding,
    unsigned int array_element,
    VkImageView view,
    VkImageLayout image_layout)
{
    VERIFY(p_rendering, "NULL pointer");
    Vk_GraphicsPipeline* p_pipeline = p_rendering->p_pipeline;
    VERIFY(p_pipeline, "NULL pointer");
    VERIFY(p_pipeline->desc_sets_count == p_rendering->desc_sets_count, "not same desc sets size");
    VERIFY(set < p_pipeline->desc_sets_count, "Set index out of range");
    VERIFY(binding < p_pipeline->p_desc_sets_layout_create_info[set].bindingCount, "Binding index out of range");

    unsigned int binding_index = UINT32_MAX;

    for (unsigned int i = 0; i < p_pipeline->p_desc_sets_layout_create_info[set].bindingCount; ++i) {
        if (binding == p_pipeline->p_desc_sets_layout_create_info[set].pBindings[i].binding) {
            binding_index = i;
            break;
        }
    }

    VERIFY(binding_index != UINT32_MAX, "Could not find the specified binding");
    VERIFY(
        p_pipeline->p_desc_sets_layout_create_info[set].pBindings[binding_index].descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        "Binding is not of type SAMPLED_IMAGE"
    );
    VERIFY(array_element < p_pipeline->p_desc_sets_layout_create_info[set].pBindings[binding_index].descriptorCount,
           "Array element is out of bounds");

    VkDescriptorImageInfo image_info = {
        .sampler     = VK_NULL_HANDLE,
        .imageView   = view,
        .imageLayout = image_layout
    };

    VkWriteDescriptorSet desc_write = {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet          = p_rendering->p_desc_sets[set],
        .dstBinding      = binding,
        .dstArrayElement = array_element,
        .descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        .descriptorCount = 1,
        .pImageInfo      = &image_info
    };

    TRACK(vkUpdateDescriptorSets(p_rendering->p_vk->device, 1, &desc_write, 0, NULL));
}

void Vk_Rendering_UpdateStorageImage(
    Vk_Rendering* p_rendering,
    unsigned int set,
    unsigned int binding,
    unsigned int array_element,
    VkImageView view,
    VkImageLayout image_layout)
{
    VERIFY(p_rendering, "NULL pointer");
    Vk_GraphicsPipeline* p_pipeline = p_rendering->p_pipeline;
    VERIFY(p_pipeline, "NULL pointer");
    VERIFY(p_pipeline->desc_sets_count == p_rendering->desc_sets_count, "Descriptor set counts do not match");
    VERIFY(set < p_pipeline->desc_sets_count, "Set index out of range");
    VERIFY(binding < p_pipeline->p_desc_sets_layout_create_info[set].bindingCount, "Binding index out of range");

    unsigned int binding_index = UINT32_MAX; // Sentinel value, assuming no binding will exceed this

    for (unsigned int i = 0; i < p_pipeline->p_desc_sets_layout_create_info[set].bindingCount; ++i) {
        if (binding == p_pipeline->p_desc_sets_layout_create_info[set].pBindings[i].binding) {
            binding_index = i;
            break;
        }
    }

    VERIFY(binding_index != UINT32_MAX, "Could not find the specified binding");
    VERIFY(
        p_pipeline->p_desc_sets_layout_create_info[set].pBindings[binding_index].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        "Binding is not of type STORAGE_IMAGE"
    );
    VERIFY(array_element < p_pipeline->p_desc_sets_layout_create_info[set].pBindings[binding_index].descriptorCount,
           "Array element is out of bounds");

    VkDescriptorImageInfo image_info = {
        .sampler     = VK_NULL_HANDLE,
        .imageView   = view,
        .imageLayout = image_layout
    };

    VkWriteDescriptorSet desc_write = {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet          = p_rendering->p_desc_sets[set],
        .dstBinding      = binding,
        .dstArrayElement = array_element,
        .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .descriptorCount = 1,
        .pImageInfo      = &image_info
    };

    TRACK(vkUpdateDescriptorSets(p_rendering->p_vk->device, 1, &desc_write, 0, NULL));
}

void Vk_Rendering_UpdateStorageBuffer(
    Vk_Rendering* p_rendering,
    unsigned int set,
    unsigned int binding,
    unsigned int array_element,
    VkBuffer buffer,
    VkDeviceSize offset,
    VkDeviceSize range)
{
    VERIFY(p_rendering, "NULL pointer");
    Vk_GraphicsPipeline* p_pipeline = p_rendering->p_pipeline;
    VERIFY(p_pipeline, "NULL pointer");
    VERIFY(p_pipeline->desc_sets_count == p_rendering->desc_sets_count, "Descriptor set counts do not match");
    VERIFY(set < p_pipeline->desc_sets_count, "Set index out of range");
    VERIFY(binding < p_pipeline->p_desc_sets_layout_create_info[set].bindingCount, "Binding index out of range");

    unsigned int binding_index = UINT32_MAX; // Sentinel value, assuming no binding will exceed this

    for (unsigned int i = 0; i < p_pipeline->p_desc_sets_layout_create_info[set].bindingCount; ++i) {
        if (binding == p_pipeline->p_desc_sets_layout_create_info[set].pBindings[i].binding) {
            binding_index = i;
            break;
        }
    }

    VERIFY(binding_index != UINT32_MAX, "Could not find the specified binding");
    VERIFY(
        p_pipeline->p_desc_sets_layout_create_info[set].pBindings[binding_index].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        "Binding is not of type STORAGE_BUFFER"
    );
    VERIFY(array_element < p_pipeline->p_desc_sets_layout_create_info[set].pBindings[binding_index].descriptorCount,
           "Array element is out of bounds");

    VkDescriptorBufferInfo buffer_info = {
        .buffer = buffer,
        .offset = offset,
        .range  = range,
    };

    VkWriteDescriptorSet desc_write = {
        .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet           = p_rendering->p_desc_sets[set],
        .dstBinding       = binding,
        .dstArrayElement  = array_element,
        .descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .descriptorCount  = 1,
        .pBufferInfo      = &buffer_info
    };

    TRACK(vkUpdateDescriptorSets(p_rendering->p_vk->device, 1, &desc_write, 0, NULL));
}