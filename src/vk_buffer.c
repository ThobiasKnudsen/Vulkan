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

    // Determine the appropriate VmaMemoryUsage based on VkBufferUsageFlags
    VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY; // Default to GPU-only

    // Prioritize staging buffers if usage includes transfer source or destination
    if (usage & VK_BUFFER_USAGE_TRANSFER_SRC_BIT || usage & VK_BUFFER_USAGE_TRANSFER_DST_BIT) {
        // If the buffer is used for both transfer and other purposes, prioritize staging
        // Adjust the memory usage accordingly
        memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU; // Suitable for buffers that are written by the CPU and read by the GPU
    }
    // Further refine memory usage based on specific buffer usages
    if (usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) {
        memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU; // Host-visible for frequent updates
    }
    if (usage & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT || usage & VK_BUFFER_USAGE_INDEX_BUFFER_BIT) {
        memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY; // Optimal for GPU access
    }
    if (usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) {
        // Storage buffers can be accessed by both CPU and GPU; choose based on access patterns
        // Here, we assume GPU-only for optimal performance
        memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
    }
    if (usage & VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT) {
        memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY; // Indirect buffers are typically used by the GPU
    }

    VmaAllocationCreateInfo allocInfo = {
        .usage = memoryUsage,
    };

    TRACK(VkResult result = vmaCreateBuffer(p_vk->allocator, &bufferInfo, &allocInfo, &buffer.buffer, &buffer.allocation, NULL));
    VERIFY(result == VK_SUCCESS, "Failed to create buffer with VMA!\n");

    buffer.size = size;

    return buffer;
}
void vk_Buffer_Clear(Vk* p_vk, Buffer buffer, int clear_value) {
    VERIFY(p_vk, "given p_vk context is NULL\n");

    Buffer staging_buffer = vk_Buffer_Create(p_vk, buffer.size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

    void* p_dst_data;
    TRACK(VkResult result = vmaMapMemory(p_vk->allocator, staging_buffer.allocation, &p_dst_data));
    VERIFY(result == VK_SUCCESS && p_dst_data, "Failed to map staging buffer memory!\n");
    TRACK(memset(p_dst_data, clear_value, buffer.size));
    TRACK(vmaUnmapMemory(p_vk->allocator, staging_buffer.allocation));

    VkCommandBuffer command_buffer = vk_CommandBuffer_CreateAndBeginSingleTimeUsage(p_vk);
    VkBufferCopy copyRegion = {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = buffer.size,
    };

    TRACK(vkCmdCopyBuffer(command_buffer, staging_buffer.buffer, buffer.buffer, 1, &copyRegion));
    TRACK(vk_CommandBuffer_EndAndDestroySingleTimeUsage(p_vk, command_buffer));
    TRACK(vmaDestroyBuffer(p_vk->allocator, staging_buffer.buffer, staging_buffer.allocation));
}
void vk_Buffer_Update(Vk* p_vk, Buffer buffer, VkDeviceSize dst_offset, const void* p_src_data, VkDeviceSize size) {

    if (size == 0) {
        return;
    }

    VERIFY(p_vk, "given p_vk context is NULL\n");
    VERIFY(p_src_data, "given p_src_data is NULL\n");
    VERIFY(buffer.size >= dst_offset + size, // Corrected condition
        "given buffer.size, dst_offset and size implies that writing to buffer will go out of bounds\n");

    Buffer staging_buffer = vk_Buffer_Create(p_vk, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

    void* p_dst_data;
    TRACK(VkResult result = vmaMapMemory(p_vk->allocator, staging_buffer.allocation, &p_dst_data));
    VERIFY(result == VK_SUCCESS && p_dst_data, "Failed to map staging buffer memory!\n");
    memcpy(p_dst_data, p_src_data, size);
    TRACK(vmaUnmapMemory(p_vk->allocator, staging_buffer.allocation));

    VkCommandBuffer command_buffer = vk_CommandBuffer_CreateAndBeginSingleTimeUsage(p_vk);

    VkBufferCopy copyRegion = {
        .srcOffset = 0,
        .dstOffset = dst_offset,
        .size = size,
    };

    vkCmdCopyBuffer(command_buffer, staging_buffer.buffer, buffer.buffer, 1, &copyRegion);
    vk_CommandBuffer_EndAndDestroySingleTimeUsage(p_vk, command_buffer);
    TRACK(vmaDestroyBuffer(p_vk->allocator, staging_buffer.buffer, staging_buffer.allocation));
}