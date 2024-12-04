#include "vk.h"
#include <stdlib.h>
#include <stdio.h>

VkDescriptorSetLayout createDescriptorSetLayout_0(VK* p_vk) {
    VkDescriptorSetLayoutCreateInfo layout_info = {
        .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings    = &(VkDescriptorSetLayoutBinding) {
            .binding            = 0, // Binding 0 in the shader
            .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount    = 1,
            .stageFlags         = VK_SHADER_STAGE_VERTEX_BIT, // Used in vertex shader
            .pImmutableSamplers = NULL
        }
    };

    VkDescriptorSetLayout descriptorSetLayout;
    if (vkCreateDescriptorSetLayout(p_vk->device, &layout_info, NULL, &descriptorSetLayout) != VK_SUCCESS) {
        printf("Failed to create descriptor set layout\n");
        exit(-1);
    }

    return descriptorSetLayout;
}

VkDescriptorSet createDescriptorSet(VK* p_vk, VkDescriptorSetLayout desc_set_layout, VkBuffer buffer) {
    if (p_vk == NULL) {
        printf("given p_vk is NULL\n");
        exit(-1);
    }

    VkDescriptorSetAllocateInfo allocInfo = {
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool     = p_vk->descriptor_pool,
        .descriptorSetCount = 1,
        .pSetLayouts        = &desc_set_layout
    };

    VkDescriptorSet descriptorSet;
    if (vkAllocateDescriptorSets(p_vk->device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
        printf("Failed to allocate descriptor set\n");
        exit(-1);
    }

    VkWriteDescriptorSet descriptor_write = {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet          = descriptorSet,
        .dstBinding      = 0, // Must match binding in shader
        .dstArrayElement = 0,
        .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .pBufferInfo     = &(VkDescriptorBufferInfo) {
            .buffer = buffer,
            .offset = 0,
            .range  = sizeof(UniformBufferObject)
        }
    };

    vkUpdateDescriptorSets(p_vk->device, 1, &descriptor_write, 0, NULL);

    return descriptorSet;
}