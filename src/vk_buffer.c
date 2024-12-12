#include "vk.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

Buffer vk_Buffer_Create(Vk* p_vk, VkDeviceSize size, VkBufferUsageFlags usage) {
    VERIFY(p_vk, "given p_vk context is NULL\n");

    Buffer buffer = {0};

    VkBufferCreateInfo bufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_AUTO; // Default to auto selection

    // Prioritize memory usage based on buffer type
    if (usage & (VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT)) {
        // For staging or transfer buffers, prefer CPU memory for better CPU-GPU transfer
        memoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
    } else if (usage & (VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)) {
        // Uniform and storage buffers might benefit from being in device memory for performance
        memoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    } else if (usage & (VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT)) {
        // These are typically GPU-only for optimal performance
        memoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    }

    VmaAllocationCreateInfo allocInfo = {
        .usage = memoryUsage,
        // Add appropriate flags if mapping is needed
        .flags = 0
    };

    // Ensure host access for transfer buffers used in staging operations
    if (usage & (VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT)) {
        allocInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    } else if (usage & (VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)) {
        allocInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    }

    TRACK(VkResult result = vmaCreateBuffer(p_vk->allocator, &bufferInfo, &allocInfo, &buffer.buffer, &buffer.allocation, NULL));
    VERIFY(result == VK_SUCCESS, "Failed to create buffer with VMA!\n");

    buffer.size = size;
    buffer.usage = memoryUsage; // Store the usage for later operations

    return buffer;
}

void vk_Buffer_Clear(Vk* p_vk, Buffer buffer, int clear_value) {
    VERIFY(p_vk, "given p_vk context is NULL\n");

    VkDeviceSize size = buffer.size;
    void* p_dst_data = NULL;
    VkResult result;

    // Decide if direct mapping is preferable or if staging is needed
    if (buffer.usage == VMA_MEMORY_USAGE_AUTO_PREFER_HOST) {
        // If the buffer is host-preferred, we can map directly
        result = vmaMapMemory(p_vk->allocator, buffer.allocation, &p_dst_data);
        VERIFY(result == VK_SUCCESS && p_dst_data, "Failed to map buffer memory!\n");
        memset(p_dst_data, clear_value, size);
        vmaUnmapMemory(p_vk->allocator, buffer.allocation);
    } else {
        // For device-preferred memory, use staging
        Buffer staging_buffer = vk_Buffer_Create(p_vk, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

        result = vmaMapMemory(p_vk->allocator, staging_buffer.allocation, &p_dst_data);
        VERIFY(result == VK_SUCCESS && p_dst_data, "Failed to map staging buffer memory!\n");
        memset(p_dst_data, clear_value, size);
        vmaUnmapMemory(p_vk->allocator, staging_buffer.allocation);

        VkCommandBuffer command_buffer = vk_CommandBuffer_CreateAndBeginSingleTimeUsage(p_vk);
        VkBufferCopy copyRegion = {
            .srcOffset = 0,
            .dstOffset = 0,
            .size = size,
        };

        vkCmdCopyBuffer(command_buffer, staging_buffer.buffer, buffer.buffer, 1, &copyRegion);
        vk_CommandBuffer_EndAndDestroySingleTimeUsage(p_vk, command_buffer);
        vmaDestroyBuffer(p_vk->allocator, staging_buffer.buffer, staging_buffer.allocation);
    }
}

void vk_Buffer_Update(Vk* p_vk, Buffer buffer, VkDeviceSize dst_offset, const void* p_src_data, VkDeviceSize size) {
    if (size == 0) {
        return;
    }

    VERIFY(p_vk, "given p_vk context is NULL\n");
    VERIFY(p_src_data, "given p_src_data is NULL\n");
    VERIFY(buffer.size >= dst_offset + size, "Writing to buffer will go out of bounds\n");

    void* p_dst_data = NULL;
    VkResult result;

    if (buffer.usage == VMA_MEMORY_USAGE_AUTO_PREFER_HOST) {
        // Directly map if buffer is host-preferred
        result = vmaMapMemory(p_vk->allocator, buffer.allocation, &p_dst_data);
        VERIFY(result == VK_SUCCESS && p_dst_data, "Failed to map buffer memory!\n");
        memcpy((uint8_t*)p_dst_data + dst_offset, p_src_data, size);
        vmaUnmapMemory(p_vk->allocator, buffer.allocation);
    } else {
        // Use staging for device-preferred buffers
        Buffer staging_buffer = vk_Buffer_Create(p_vk, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

        result = vmaMapMemory(p_vk->allocator, staging_buffer.allocation, &p_dst_data);
        VERIFY(result == VK_SUCCESS && p_dst_data, "Failed to map staging buffer memory!\n");
        memcpy(p_dst_data, p_src_data, size);
        vmaUnmapMemory(p_vk->allocator, staging_buffer.allocation);

        VkCommandBuffer command_buffer = vk_CommandBuffer_CreateAndBeginSingleTimeUsage(p_vk);

        VkBufferCopy copyRegion = {
            .srcOffset = 0,
            .dstOffset = dst_offset,
            .size = size,
        };

        vkCmdCopyBuffer(command_buffer, staging_buffer.buffer, buffer.buffer, 1, &copyRegion);
        vk_CommandBuffer_EndAndDestroySingleTimeUsage(p_vk, command_buffer);
        vmaDestroyBuffer(p_vk->allocator, staging_buffer.buffer, staging_buffer.allocation);
    }
}

void vk_Buffer_CopyBuffer(Vk* p_vk, Buffer src_buffer, Buffer dst_buffer, VkDeviceSize src_offset, VkDeviceSize dst_offset, VkDeviceSize size) {
    VERIFY(p_vk, "given p_vk context is NULL\n");
    
    // Check if the copy operation would go out of bounds
    VERIFY(src_buffer.size >= src_offset + size, "Source buffer copy operation would go out of bounds\n");
    VERIFY(dst_buffer.size >= dst_offset + size, "Destination buffer copy operation would go out of bounds\n");
    
    VkCommandBuffer command_buffer = vk_CommandBuffer_CreateAndBeginSingleTimeUsage(p_vk);

    VkBufferCopy copyRegion = {
        .srcOffset = src_offset,
        .dstOffset = dst_offset,
        .size = size,
    };

    // Execute the copy command
    TRACK(vkCmdCopyBuffer(command_buffer, src_buffer.buffer, dst_buffer.buffer, 1, &copyRegion));

    // End and submit the command buffer for execution
    vk_CommandBuffer_EndAndDestroySingleTimeUsage(p_vk, command_buffer);
}