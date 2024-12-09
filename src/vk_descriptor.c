#include "vk.h"
#include <stdlib.h>
#include <stdio.h>

VkDescriptorSetLayoutCreateInfo* vk_DescriptorSetLayoutCreateInfo_Create(Vk* p_vk, const SpvReflectShaderModule* p_shader_modules, unsigned int shader_modules_count, size_t* p_create_info_count) {
    VERIFY(p_vk, "NULL pointer");
    VERIFY(p_shader_modules, "NULL pointer");
    VERIFY(p_create_info_count, "NULL pointer");

    VkDescriptorSetLayoutCreateInfo*    p_create_info = NULL;
    size_t                              create_info_count = 0;

    for (unsigned int module = 0; module < shader_modules_count; module++) {
        printf("module %d\n", module);

        // for each binding there is a binding and set number
        for (unsigned int binding_i = 0; binding_i < p_shader_modules[module].descriptor_binding_count; binding_i++) {
            printf("binding_i %d\n", binding_i);

            unsigned int set_number = p_shader_modules[module].descriptor_bindings[binding_i].set ;
            unsigned int binding_number = p_shader_modules[module].descriptor_bindings[binding_i].binding;

            printf("set_number %d\n", set_number);
            printf("binding_number %d\n", binding_number);

            // is set_number allready in p_desc_set_layouts?
            bool set_allready_exists = false;
            if (p_create_info) {
                if (create_info_count > set_number) {
                    if (p_create_info[set_number].sType == VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO)
                        set_allready_exists = true;
                }
            }

            // if not set allready exists in p_create_info then it needs to be allocated
            if (!set_allready_exists) {
                if (create_info_count <= set_number) {

                    size_t prev_count = create_info_count;
                    create_info_count = set_number+1;

                    TRACK(p_create_info = alloc(  p_create_info,   create_info_count * sizeof(VkDescriptorSetLayoutCreateInfo)  ));
                    memset(  &p_create_info[prev_count],  0,  (create_info_count - prev_count) * sizeof(VkDescriptorSetLayoutCreateInfo)  );
                    for (unsigned int new = create_info_count - prev_count; new < create_info_count; ++new) {
                        p_create_info[new].sType = VK_STRUCTURE_TYPE_MAX_ENUM;
                    }

                    printf("%p  p_create_info\n", p_create_info);
                }
                p_create_info[set_number].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            }

            if (p_create_info[set_number].pBindings) {
                for (unsigned int i = 0; i < p_create_info[set_number].bindingCount; i++) {
                    VERIFY(p_create_info[set_number].pBindings[i].binding != binding_number, "binding-set pair already exists");
                }
            }

            // allocating space for new binding
            p_create_info[set_number].bindingCount++;
            TRACK(p_create_info[set_number].pBindings = alloc(
                p_create_info[set_number].pBindings,  
                p_create_info[set_number].bindingCount * sizeof(VkDescriptorSetLayoutBinding)
            ));
            printf("%p p_create_info[set_number].pBindings \n", p_create_info[set_number].pBindings);

            // writing binding data
            VkDescriptorSetLayoutBinding* p_new_binding = &p_create_info[set_number].pBindings[p_create_info[set_number].bindingCount-1];
            p_new_binding->binding = binding_number;
            p_new_binding->descriptorType = p_shader_modules[module].descriptor_bindings[binding_i].descriptor_type;
            p_new_binding->descriptorCount = p_shader_modules[module].descriptor_bindings[binding_i].count;
            p_new_binding->stageFlags = (VkShaderStageFlags)p_shader_modules[module].shader_stage;
            p_new_binding->pImmutableSamplers = NULL; // Update if using immutable samplers

        }
    }

    for (unsigned int i = 0; i < create_info_count; i++) {
        VERIFY(p_create_info[i].sType == VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, "set numbers is not continuous");
    }

    TRACK(vk_DescriptorSetLayoutCreateInfo_Print(p_create_info, create_info_count));

    *p_create_info_count = create_info_count;
    return p_create_info;
}
void vk_DescriptorSetLayoutCreateInfo_Print(const VkDescriptorSetLayoutCreateInfo* p_create_info, const size_t create_info_count) {
    VERIFY(p_create_info, "NULL pointer");
    for (unsigned int i = 0; i < create_info_count; ++i) {
        printf("set_number %u\n", i);
        printf("binding_count %u\n", p_create_info[i].bindingCount);
        for (unsigned int j = 0; j < p_create_info[i].bindingCount; j++) {
            printf("\tbinding %u\n", p_create_info[i].pBindings[j].binding);
            printf("\tdescriptorType %u\n", p_create_info[i].pBindings[j].descriptorType);
            printf("\tdescriptorCount %u\n", p_create_info[i].pBindings[j].descriptorCount);
            printf("\tstageFlags %u\n", p_create_info[i].pBindings[j].stageFlags);
            printf("\tsampler %p\n", (void*)p_create_info[i].pBindings[j].pImmutableSamplers);
        }
    }
}
VkDescriptorSetLayout* vk_DescriptorSetLayout_Create(Vk* p_vk, const VkDescriptorSetLayoutCreateInfo* p_create_info, const size_t create_info_count) {

    VERIFY(p_vk, "NULL pointer");
    VERIFY(p_create_info, "NULL pointer");

    TRACK(VkDescriptorSetLayout* p_set_layout = alloc(NULL, create_info_count*sizeof(VkDescriptorSetLayout)));
    memset(p_set_layout, 0, create_info_count*sizeof(VkDescriptorSetLayout));

    for (unsigned int i = 0; i < create_info_count; ++i) {
        VERIFY(vkCreateDescriptorSetLayout(p_vk->device, &p_create_info[i], NULL, &p_set_layout[i]) == VK_SUCCESS, "failed to create");
    }

    return p_set_layout;
}
VkDescriptorSetLayout vk_DescriptorSetLayout_Create_0(Vk* p_vk) {
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
    VERIFY(vkCreateDescriptorSetLayout(p_vk->device, &layout_info, NULL, &descriptorSetLayout) == VK_SUCCESS, "Failed to create descriptor set layout\n");

    return descriptorSetLayout;
}
VkDescriptorSet* vk_DescriptorSet_Create(Vk* p_vk, const VkDescriptorSetLayout* p_desc_set_layout, size_t desc_set_layouts_count, VkBuffer buffer, Image* p_image) {
    
    VERIFY(p_vk, "NULL pointer");
    VERIFY(p_desc_set_layout, "NULL pointer");
    VERIFY(p_image, "NULL pointer");
    VERIFY(p_image->image_view != VK_NULL_HANDLE, "image view is VK_NULL_HANDLE");
    VERIFY(p_image->sampler != VK_NULL_HANDLE, "sampler is VK_NULL_HANDLE");
    VERIFY(desc_set_layouts_count == 2, "uhhhh");
    TRACK(VkDescriptorSet* p_desc_sets = alloc(NULL, desc_set_layouts_count * sizeof(VkDescriptorSet)));
    VERIFY(p_desc_sets, "Descriptor sets allocation failed");

    for (size_t set = 0; set < desc_set_layouts_count; ++set) {
        printf("set %d\n", set);
        VkDescriptorSetAllocateInfo alloc_info = {
            .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool     = p_vk->descriptor_pool,
            .descriptorSetCount = 1,
            .pSetLayouts        = &p_desc_set_layout[set]
        };
        TRACK(VkResult result = vkAllocateDescriptorSets(p_vk->device, &alloc_info, &p_desc_sets[set]));
        VERIFY(result == VK_SUCCESS, "Failed to allocate descriptor set. result = %d", result);
    }

    TRACK(VkWriteDescriptorSet* desc_writes = alloc(NULL, desc_set_layouts_count * sizeof(VkWriteDescriptorSet)));
    VERIFY(desc_writes, "Descriptor writes allocation failed");

    desc_writes[0] = (VkWriteDescriptorSet){
        .sType              = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet             = p_desc_sets[0],
        .dstBinding         = 0, // Must match binding in shader
        .dstArrayElement    = 0,
        .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // Adjust based on set layout
        .descriptorCount    = 1,
        .pBufferInfo        = &(VkDescriptorBufferInfo){
            .buffer = buffer,
            .offset = 0,
            .range  = sizeof(UniformBufferObject),
        }
    };
    desc_writes[1] = (VkWriteDescriptorSet){
        .sType              = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet             = p_desc_sets[1],
        .dstBinding         = 0, // Must match binding in shader
        .dstArrayElement    = 0,
        .descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount    = 1,
        .pImageInfo         = &(VkDescriptorImageInfo){
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .imageView   = p_image->image_view,
            .sampler     = p_image->sampler,
        },
    };

    for (size_t set = 0; set < desc_set_layouts_count; ++set) {
        TRACK(vkUpdateDescriptorSets(p_vk->device, 1, &desc_writes[set], 0, NULL));
    }
    return p_desc_sets;
}