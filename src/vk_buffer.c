#include "vk.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


Buffer createBuffer( VK* p_vk, VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage ) {
	if (p_vk == NULL) {
		printf("given p_vk context is NULL\n");
		exit(-1);
	}

    Buffer buffer = {0};

    VkBufferCreateInfo bufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    VmaAllocationCreateInfo allocInfo = {
        .usage = memoryUsage,
    };

    // Create buffer and allocate memory
    if (vmaCreateBuffer(p_vk->allocator, &bufferInfo, &allocInfo, &buffer.buffer, &buffer.allocation, NULL) != VK_SUCCESS) {
        printf("Failed to create buffer with VMA!\n");
        exit(-1);
    }

    buffer.size = size;

    return buffer;
}
void clearBuffer( VK* p_vk, Buffer buffer, int clear_value) {
	if (p_vk == NULL) {
		printf("given p_vk context is NULL\n");
		exit(-1);
	}
    void* mapped_data;
    if (vmaMapMemory(p_vk->allocator, buffer.allocation, &mapped_data) != VK_SUCCESS) {
        printf("Failed to map buffer memory!\n");
        exit(-1);
    }
    memset(mapped_data, 0, buffer.size);
    vmaUnmapMemory(p_vk->allocator, buffer.allocation);
}
void updateBuffer( VK* p_vk, Buffer buffer, VkDeviceSize dst_offset, const void* p_src_data, VkDeviceSize size ) {
	if (p_vk == NULL) {
		printf("given p_vk context is NULL\n");
		exit(-1);
	}
	if (p_src_data == NULL) {
		printf("given p_src_data is NULL\n");
		exit(-1);
	}
	if (buffer.size < dst_offset + size) {
		printf("given buffer.size, dst_offset and size implies that writing to buffer will go out of bounds\n");
		exit(-1);
	}
    void* mapped_data;
    if (vmaMapMemory(p_vk->allocator, buffer.allocation, &mapped_data) != VK_SUCCESS) {
        printf("Failed to map buffer memory!\n");
        exit(-1);
    }
    memcpy(mapped_data+dst_offset, p_src_data, size);
    vmaUnmapMemory(p_vk->allocator, buffer.allocation);
}