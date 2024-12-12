#include "vk.h"


Gui_Rendering Gui_Rendering_Create(
    Gui_GraphicsPipeline* p_pipeline,
    Image* p_target_image)
{
    VERIFY(p_pipeline, "Pipeline is a NULL pointer");
    VERIFY(p_pipeline->p_vk, "p_vk is a NULL pointer");
    VERIFY(p_pipeline->p_shaders, "Shaders pointer is NULL in pipeline");
    VERIFY(p_pipeline->shaders_count > 0, "There are 0 shaders in pipeline");
    VERIFY(p_pipeline->pipeline_layout != VK_NULL_HANDLE, "pipeline_layout is VK_NULL_HANDLE");
    VERIFY(p_pipeline->graphics_pipeline != VK_NULL_HANDLE, "graphics_pipeline is VK_NULL_HANDLE");

    VERIFY(p_target_image, "NULL pointer passed to Gui_Rendering_Create");
    VERIFY(p_target_image->image != VK_NULL_HANDLE, "Target image handle is VK_NULL_HANDLE");
    VERIFY(p_target_image->layout != VK_IMAGE_LAYOUT_UNDEFINED, "Target image layout is VK_IMAGE_LAYOUT_UNDEFINED");
    VERIFY(p_target_image->extent.width != 0, "Target image width is zero");
    VERIFY(p_target_image->extent.height != 0, "Target image height is zero");
    VERIFY(p_target_image->format != VK_FORMAT_UNDEFINED, "Target image format is VK_FORMAT_UNDEFINED");
    VERIFY(p_target_image->allocation != VK_NULL_HANDLE, "Target image allocation is VK_NULL_HANDLE");

    Gui_Rendering rendering = {0};
    rendering.p_vk = p_pipeline->p_vk; 
    rendering.p_pipeline = p_pipeline; 
    rendering.p_target_image = p_target_image; 

    // Creating descriptor sets
    TRACK(rendering.p_desc_sets = vk_DescriptorSet_Create(
        p_pipeline->p_vk,
        p_pipeline->p_desc_sets_layout_create_info, // Ensure correct parameter
        p_pipeline->desc_sets_count
    ));
    rendering.desc_sets_count = p_pipeline->desc_sets_count;

    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = p_pipeline->p_vk->command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    TRACK(VkResult result = vkAllocateCommandBuffers(p_pipeline->p_vk->device, &alloc_info, &rendering.command_buffer));
    VERIFY(result == VK_SUCCESS, "Failed to allocate command buffer for GUI rendering");

    VkCommandBufferBeginInfo begin_info = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT, .pInheritanceInfo = NULL };
    TRACK(result = vkBeginCommandBuffer(rendering.command_buffer, &begin_info));
    VERIFY(result == VK_SUCCESS, "Failed to begin command buffer for GUI rendering");

    // Transition the image layout to color attachment optimal
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
    TRACK(vkCmdPipelineBarrier(rendering.command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, NULL, 0, NULL, 1, &barrier_to_color_attachment ));

    // Begin dynamic rendering
    VkRenderingAttachmentInfo color_attachment = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = p_target_image->image_view,
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .resolveMode = VK_RESOLVE_MODE_NONE,
        .resolveImageView = VK_NULL_HANDLE,
        .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = { .color = {0.0f, 0.0f, 0.0f, 1.0f} },
    };

    VkRenderingInfo rendering_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = {
            .offset = {0, 0},
            .extent = p_target_image->extent
        },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment,
        .pDepthAttachment = VK_NULL_HANDLE,
        .pStencilAttachment = VK_NULL_HANDLE,
    };

    TRACK(vkCmdBeginRendering(rendering.command_buffer, &rendering_info));
    TRACK(vkCmdBindPipeline(rendering.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, p_pipeline->graphics_pipeline));
    TRACK(vkCmdBindDescriptorSets(rendering.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, p_pipeline->pipeline_layout, 0, (uint32_t)rendering.desc_sets_count, rendering.p_desc_sets, 0, NULL));

    // Set dynamic viewport based on target image extent
    VkViewport viewport = { .x = 0.0f, .y = 0.0f, .width = (float)p_target_image->extent.width, .height = (float)p_target_image->extent.height, .minDepth = 0.0f, .maxDepth = 1.0f };
    TRACK(vkCmdSetViewport(rendering.command_buffer, 0, 1, &viewport));

    // Set dynamic scissor based on target image extent
    VkRect2D scissor = { .offset = {0, 0},  .extent = p_target_image->extent };
    TRACK(vkCmdSetScissor(rendering.command_buffer, 0, 1, &scissor));

    // Bind the instance buffer for instanced rendering
    VkBuffer instance_buffers[] = { rendering.instance_buffer.buffer };
    VkDeviceSize instance_offsets[] = { 0 }; // Assuming no offset for simplicity
    TRACK(vkCmdBindVertexBuffers(rendering.command_buffer, 1, 1, instance_buffers, instance_offsets));
    TRACK(vkCmdDrawIndirect( rendering.command_buffer, rendering.indirect_buffer.buffer, 0, 1, sizeof(VkDrawIndirectCommand)));

    // End dynamic rendering
    TRACK(vkCmdEndRendering(rendering.command_buffer));

    // Transition the image layout to present
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

    TRACK(vkCmdPipelineBarrier(rendering.command_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 1, &barrier_to_present ));
    VERIFY(vkEndCommandBuffer(rendering.command_buffer) == VK_SUCCESS, "Failed to end command buffer for GUI rendering");

    return rendering;
}

void Gui_Rendering_SetTargetImage(
    Gui_Rendering* p_rendering, 
    Image* p_target_image) 
{
    VERIFY(p_rendering, "NULL pointer passed to Gui_Rendering_SetTargetImage");
    VERIFY(p_target_image, "NULL pointer passed as target image");
    VERIFY(p_target_image->image != VK_NULL_HANDLE, "Target image handle is VK_NULL_HANDLE");
    VERIFY(p_target_image->layout != VK_IMAGE_LAYOUT_UNDEFINED, "Target image layout is VK_IMAGE_LAYOUT_UNDEFINED");
    VERIFY(p_target_image->extent.width != 0, "Target image width is zero");
    VERIFY(p_target_image->extent.height != 0, "Target image height is zero");
    VERIFY(p_target_image->format != VK_FORMAT_UNDEFINED, "Target image format is VK_FORMAT_UNDEFINED");
    VERIFY(p_target_image->allocation != VK_NULL_HANDLE, "Target image allocation is VK_NULL_HANDLE");

    p_rendering->p_target_image = p_target_image;
}

void Gui_Rendering_UpdateIndirectBuffer(
    Gui_Rendering* p_rendering,
    uint32_t instance_count,
    uint32_t first_instance)
{
    VERIFY(p_rendering, "NULL pointer passed to Gui_Rendering_UpdateIndirectBuffer");

    if (p_rendering->indirect_buffer.size == 0) {
        TRACK(p_rendering->indirect_buffer = vk_Buffer_Create(p_rendering->p_vk, sizeof(VkDrawIndirectCommand), VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT));
    }

    VkDrawIndirectCommand draw_cmd = {
        .vertexCount = 4, 
        .instanceCount = instance_count,
        .firstVertex = 0, 
        .firstInstance = first_instance
    };

    TRACK(vk_Buffer_Update(p_rendering->p_vk, p_rendering->indirect_buffer, 0, &draw_cmd, sizeof(VkDrawIndirectCommand)));
}

void Gui_Rendering_UpdateInstanceBuffer(
    Gui_Rendering* p_rendering,
    size_t dst_offset,
    void* p_src_data,
    size_t src_data_size)
{
    VERIFY(p_rendering, "NULL pointer passed to Gui_Rendering_UpdateInstanceBuffer");
    VERIFY(p_src_data, "Source data pointer is NULL");
    VERIFY(src_data_size > 0, "Source data size must be greater than zero");

    if (p_rendering->instance_buffer.size == 0 || p_rendering->instance_buffer.size < dst_offset + src_data_size) {
        VkDeviceSize new_size = p_rendering->instance_buffer.size > dst_offset + src_data_size ? 
            p_rendering->instance_buffer.size : 
            dst_offset + src_data_size;
        TRACK(p_rendering->instance_buffer = vk_Buffer_Create(p_rendering->p_vk, new_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT));
    }

    TRACK(vk_Buffer_Update(p_rendering->p_vk, p_rendering->instance_buffer, dst_offset, p_src_data, src_data_size));
}

void Gui_Rendering_UpdateUniformBuffer(
    Gui_Rendering* p_rendering,
    Gui_GraphicsPipeline* p_pipeline,
    unsigned int set,
    unsigned int binding,
    unsigned int array_element,
    VkBuffer buffer,
    unsigned int offset,
    unsigned int range) 
{
    VERIFY(p_rendering, "NULL pointer");
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

void Gui_Rendering_UpdateCombinedImageSampler(
    Gui_Rendering* p_rendering,
    Gui_GraphicsPipeline* p_pipeline,
    unsigned int set,
    unsigned int binding,
    unsigned int array_element,
    VkSampler sampler,
    VkImageView image_view,
    VkImageLayout image_layout)
{
    VERIFY(p_rendering, "NULL pointer");
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
        .sampler     = sampler,
        .imageView   = image_view,
        .imageLayout = image_layout
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

void Gui_Rendering_UpdateSampledImage(
    Gui_Rendering* p_rendering,
    Gui_GraphicsPipeline* p_pipeline,
    unsigned int set,
    unsigned int binding,
    unsigned int array_element,
    VkImageView image_view,
    VkImageLayout image_layout)
{
    VERIFY(p_rendering, "NULL pointer");
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
        .imageView   = image_view,
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

void Gui_Rendering_UpdateStorageImage(
    Gui_Rendering* p_rendering,
    Gui_GraphicsPipeline* p_pipeline,
    unsigned int set,
    unsigned int binding,
    unsigned int array_element,
    VkImageView image_view,
    VkImageLayout image_layout)
{
    VERIFY(p_rendering, "NULL pointer passed to Gui_Rendering_UpdateStorageImage");
    VERIFY(p_pipeline, "NULL pointer passed to Gui_Rendering_UpdateStorageImage");
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
        .imageView   = image_view,
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

void Gui_Rendering_UpdateStorageBuffer(
    Gui_Rendering* p_rendering,
    Gui_GraphicsPipeline* p_pipeline,
    unsigned int set,
    unsigned int binding,
    unsigned int array_element,
    VkBuffer buffer,
    VkDeviceSize offset,
    VkDeviceSize range)
{
    VERIFY(p_rendering, "NULL pointer passed to Gui_Rendering_UpdateStorageBuffer");
    VERIFY(p_pipeline, "NULL pointer passed to Gui_Rendering_UpdateStorageBuffer");
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