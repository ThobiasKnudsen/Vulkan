#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "vk.h"
#include <libgen.h>

Image vk_Image_Create_ReadWrite(Vk* p_vk, VkExtent2D extent, VkFormat format) {
	Image image;
	image.layout = VK_IMAGE_LAYOUT_UNDEFINED;
	image.extent = extent;
	image.format = format; //VK_FORMAT_R8G8B8A8_SRGB

	VkImageCreateInfo image_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.extent.width = extent.width,
		.extent.height = extent.height,
		.extent.depth = 1,
		.mipLevels = 1,
		.arrayLayers = 1,
		.format = format,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	};

	VmaAllocationCreateInfo alloc_info = {
		.usage = VMA_MEMORY_USAGE_GPU_ONLY,
	};	

	TRACK(VkResult result = vmaCreateImage(p_vk->allocator, &image_info, &alloc_info, &image.image, &image.allocation, NULL));
	VERIFY(result == VK_SUCCESS, "failed to create\n ");
	VERIFY(image.image != VK_NULL_HANDLE, "failed to create");

	VkImageViewCreateInfo view_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = image.image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = format, //VK_FORMAT_R8G8B8A8_SRGB
		.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.subresourceRange.baseMipLevel = 0,
		.subresourceRange.levelCount = 1,
		.subresourceRange.baseArrayLayer = 0,
		.subresourceRange.layerCount = 1,
	};

	TRACK(result = vkCreateImageView(p_vk->device, &view_info, NULL, &image.image_view));
	VERIFY(result == VK_SUCCESS, "failed to create\n ");
	VERIFY(image.image_view != VK_NULL_HANDLE, "failed to create");

	VkSamplerCreateInfo sampler_info = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter = VK_FILTER_LINEAR,
		.minFilter = VK_FILTER_LINEAR,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.anisotropyEnable = VK_TRUE,
		.maxAnisotropy = 16,
		.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		.unnormalizedCoordinates = VK_FALSE,
		.compareEnable = VK_FALSE,
		.compareOp = VK_COMPARE_OP_ALWAYS,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
		.mipLodBias = 0.0f,
		.minLod = 0.0f,
		.maxLod = 0.0f,
	};

	TRACK(result = vkCreateSampler(p_vk->device, &sampler_info, NULL, &image.sampler));
	VERIFY(result == VK_SUCCESS, "failed to create\n ");
	VERIFY(image.sampler != VK_NULL_HANDLE, "failed to create");

	return image;
}
void vk_Image_TransitionLayout(VkCommandBuffer command_buffer, Image* p_image, VkImageLayout new_layout) {

	VERIFY(p_image, "NULL pointer");

	VkImageLayout old_layout = p_image->layout;
	if (old_layout == new_layout) {
		printf("WARNING: image layout is already in the prefered image layout\n");
		return;
	}

    VkImageAspectFlags aspect_mask = 0;
    if (p_image->format == VK_FORMAT_D32_SFLOAT ||
        p_image->format == VK_FORMAT_D24_UNORM_S8_UINT ||
        p_image->format == VK_FORMAT_D32_SFLOAT_S8_UINT) {
        aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;
        // Add stencil aspect if present
        if (p_image->format == VK_FORMAT_D24_UNORM_S8_UINT || p_image->format == VK_FORMAT_D32_SFLOAT_S8_UINT) {
            aspect_mask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    } else {
        aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    VkImageSubresourceRange subresource_range = {
        .aspectMask = aspect_mask,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };

    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = old_layout,
        .newLayout = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = p_image->image,
        .subresourceRange = subresource_range,
    };

    VkPipelineStageFlags src_stage;
    VkPipelineStageFlags dst_stage;

    // Define source access mask and pipeline stage based on old_layout
    switch (old_layout) {
        case VK_IMAGE_LAYOUT_UNDEFINED:
            barrier.srcAccessMask = 0;
            src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            break;
        case VK_IMAGE_LAYOUT_PREINITIALIZED:
            barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
            src_stage = VK_PIPELINE_STAGE_HOST_BIT;
            break;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            src_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            break;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            src_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            break;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            break;
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            break;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            src_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            break;
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            src_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            break;
        default:
            VERIFY(false, "unsupported old layout\n");
    }

    // Define destination access mask and pipeline stage based on new_layout
    switch (new_layout) {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            break;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            break;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dst_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            break;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            dst_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            break;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            break;
        case VK_IMAGE_LAYOUT_GENERAL:
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            break;
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            dst_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            break;
        case VK_IMAGE_LAYOUT_UNDEFINED:
            barrier.dstAccessMask = 0;
            dst_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            break;
        case VK_IMAGE_LAYOUT_PREINITIALIZED:
            barrier.dstAccessMask = VK_ACCESS_HOST_WRITE_BIT;
            dst_stage = VK_PIPELINE_STAGE_HOST_BIT;
            break;
        default:
            VERIFY(false, "unsupported old layout\n");
    }
    
    TRACK(vkCmdPipelineBarrier( command_buffer, src_stage, dst_stage, 0, 0, NULL, 0, NULL, 1, &barrier ));

    p_image->layout = new_layout;
}
void vk_Image_CopyData( Vk* p_vk, Image* p_image, VkImageLayout final_layout, const void* p_data, const VkRect2D rect, const size_t pixel_size ) {

	VERIFY(p_vk, "NULL pointer");
	VERIFY(p_image, "NULL pointer");
	VERIFY(p_data, "NULL pointer");

	size_t image_size = rect.extent.width*rect.extent.height*pixel_size;
	TRACK(Buffer staging_buffer = vk_Buffer_Create(  p_vk,  image_size,  VK_BUFFER_USAGE_TRANSFER_SRC_BIT  ));
	VERIFY(image_size, "image_size is 0");

	void* p_dst_data;
    TRACK(vmaMapMemory(p_vk->allocator, staging_buffer.allocation, &p_dst_data));
    VERIFY(p_dst_data, "NULL pointer");
    TRACK(memcpy(p_dst_data, p_data, image_size));
    TRACK(vmaUnmapMemory(p_vk->allocator, staging_buffer.allocation));

    VkBufferImageCopy region = {
        .bufferOffset = 0,
        .bufferRowLength = 0, 
        .bufferImageHeight = 0, 
        .imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .imageSubresource.mipLevel = 0,
        .imageSubresource.baseArrayLayer = 0,
        .imageSubresource.layerCount = 1,
        .imageOffset = {
            .x = rect.offset.x,
            .y = rect.offset.y,
            .z = 0
        },
        .imageExtent = {
            .width = rect.extent.width,
            .height = rect.extent.height,
            .depth = 1
        },
    };

    TRACK(VkCommandBuffer command_buffer = vk_CommandBuffer_CreateAndBeginSingleTimeUsage(p_vk));
    TRACK(vk_Image_TransitionLayout(command_buffer, p_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL));
    TRACK(vkCmdCopyBufferToImage( command_buffer, staging_buffer.buffer, p_image->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region ));
    TRACK(vk_Image_TransitionLayout(command_buffer, p_image, final_layout));
    TRACK(vk_CommandBuffer_EndAndDestroySingleTimeUsage(p_vk, command_buffer));
	TRACK(vmaDestroyBuffer(p_vk->allocator, staging_buffer.buffer, staging_buffer.allocation));
}
void vk_Image_CopyImageFile( Vk* p_vk, Image* p_image, VkImageLayout final_layout, const char* filename ) {

    VERIFY(p_vk, "Vk pointer is NULL.");
    VERIFY(p_image, "Image pointer is NULL.");
    VERIFY(p_vk->allocator, "VmaAllocator is NULL.");
    VERIFY(filename, "Filename is NULL.");

    int width, height, channels;
    TRACK(unsigned char* p_data = stbi_load(filename, &width, &height, &channels, STBI_rgb_alpha));
    VERIFY(p_data, "Failed to load image file: %s\n", filename);

    size_t pixel_size = 4;
    VkRect2D rect = {
        .offset = {0, 0},
        .extent = {width, height}
    };

    TRACK(vk_Image_CopyData( p_vk, p_image, final_layout, p_data, rect, pixel_size ));
    TRACK(stbi_image_free(p_data));
}

Image vk_Image_CreateFromImageFile( Vk* p_vk, const char* filename, VkFormat format, VkImageLayout layout ) {

    VERIFY(p_vk, "Vk pointer is NULL.");
    VERIFY(p_vk->allocator, "VmaAllocator is NULL.");
    VERIFY(filename, "Filename is NULL.");

    char absolute_path[PATH_MAX];
	VERIFY(realpath(filename, absolute_path), "realpath");
	printf("Loading image from: %s\n", absolute_path);

	TRACK(FILE* file = fopen(filename, "rb"));
	VERIFY(file, "Cannot open file: %s\n", filename);
	TRACK(fclose(file));

    int width, height, channels;
    TRACK(unsigned char* p_data = stbi_load(filename, &width, &height, &channels, STBI_rgb_alpha));
    VERIFY(p_data, "Failed to load image file: %s\n", filename);

    size_t pixel_size = 4;
    VkRect2D rect = {
        .offset = {0, 0},
        .extent = {width, height}
    };

    TRACK(Image image = vk_Image_Create_ReadWrite(p_vk, rect.extent, format));

    TRACK(vk_Image_CopyData( p_vk, &image, layout, p_data, rect, pixel_size ));
    TRACK(stbi_image_free(p_data));

    return image;

}