#include "vk.h"

VkPipelineLayout createPipelineLayout(VkDevice device, VkDescriptorSetLayout* p_set_layouts, size_t set_layout_count) {
    VkPipelineLayoutCreateInfo vk_pipeline_layoutInfo = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount         = set_layout_count,
        .pSetLayouts            = p_set_layouts,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges    = NULL
    };
    VkPipelineLayout pipelineLayout;
    if (vkCreatePipelineLayout(device, &vk_pipeline_layoutInfo, NULL, &pipelineLayout) != VK_SUCCESS) {
        printf("Failed to create pipeline layout\n");
        exit(EXIT_FAILURE);
    }
    return pipelineLayout;
}
VkPipeline createGraphicsPipeline(
    VK* p_vk,
    VkPipelineLayout pipelineLayout
) {
    VkPipelineRenderingCreateInfo pipelineRenderingInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .pNext = NULL,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &p_vk->swap_chain.image_format,
        // Add depth and stencil formats if used
    };
    VkGraphicsPipelineCreateInfo pipelineInfo = {
        .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext               = &(VkPipelineRenderingCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .pNext = NULL,
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &p_vk->swap_chain.image_format,
        },
        .stageCount          = 2,
        .pStages             = (VkPipelineShaderStageCreateInfo[2]) {{
            .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage  = VK_SHADER_STAGE_VERTEX_BIT,
            .module = createShaderModuleFromGlslFile(p_vk, "shaders/shader.vert.glsl", shaderc_glsl_vertex_shader),
            .pName  = "main"
        },{
            .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = createShaderModuleFromGlslFile(p_vk, "shaders/shader.frag.glsl", shaderc_glsl_fragment_shader),
            .pName  = "main"
        }},
        .pVertexInputState = &(VkPipelineVertexInputStateCreateInfo) {
            .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount   = 1,
            .pVertexBindingDescriptions      = (VkVertexInputBindingDescription[1]) {{
                .binding   = 0,
                .stride    = sizeof(InstanceData),
                .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE
            }},
            .vertexAttributeDescriptionCount = 7,
            .pVertexAttributeDescriptions    = (VkVertexInputAttributeDescription[7]) {{
                .binding  = 0,
                .location = 0,
                .format   = VK_FORMAT_R32G32_SFLOAT,
                .offset   = offsetof(InstanceData, pos)
                },{
                .binding  = 0,
                .location = 1,
                .format   = VK_FORMAT_R32G32_SFLOAT,
                .offset   = offsetof(InstanceData, size)
                },{
                .binding  = 0,
                .location = 2,
                .format   = VK_FORMAT_R32_SFLOAT,
                .offset   = offsetof(InstanceData, rotation)
                },{
                .binding  = 0,
                .location = 3,
                .format   = VK_FORMAT_R32_SFLOAT,
                .offset   = offsetof(InstanceData, corner_radius)
                },{
                .binding  = 0,
                .location = 4,
                .format   = VK_FORMAT_R8G8B8A8_UNORM,
                .offset   = offsetof(InstanceData, color)
                },{
                .binding  = 0,
                .location = 5,
                .format   = VK_FORMAT_R32_UINT,
                .offset   = offsetof(InstanceData, tex_index)
                },{
                .binding  = 0,
                .location = 6,
                .format   = VK_FORMAT_R32G32B32A32_SFLOAT,
                .offset   = offsetof(InstanceData, tex_rect)
            }},
        }, 
        .pInputAssemblyState = &(VkPipelineInputAssemblyStateCreateInfo) {
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
            .primitiveRestartEnable = VK_FALSE
        },
        .pViewportState      = &(VkPipelineViewportStateCreateInfo) {
            .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .pViewports    = &(VkViewport) {
                .x        = 0.0f,
                .y        = 0.0f,
                .width    = (float)p_vk->swap_chain.extent.width,
                .height   = (float)p_vk->swap_chain.extent.height,
                .minDepth = 0.0f,
                .maxDepth = 1.0f
            },
            .scissorCount  = 1,
            .pScissors     = &(VkRect2D) {
                .offset = (VkOffset2D){0, 0},
                .extent = p_vk->swap_chain.extent
            }
        },
        .pRasterizationState = &(VkPipelineRasterizationStateCreateInfo) {
            .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .depthClampEnable        = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode             = VK_POLYGON_MODE_FILL,
            .lineWidth               = 1.0f,
            .cullMode                = VK_CULL_MODE_BACK_BIT,
            .frontFace               = VK_FRONT_FACE_CLOCKWISE,
            .depthBiasEnable         = VK_FALSE
        },
        .pMultisampleState   = &(VkPipelineMultisampleStateCreateInfo) {
            .sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .sampleShadingEnable  = VK_FALSE,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
        },
        .pDepthStencilState  = &(VkPipelineDepthStencilStateCreateInfo) {
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .depthTestEnable        = VK_FALSE,
            .depthWriteEnable       = VK_FALSE, // Disable depth writes
            .depthCompareOp         = VK_COMPARE_OP_LESS,
            .depthBoundsTestEnable  = VK_FALSE,
            .stencilTestEnable      = VK_FALSE
        },
        .pColorBlendState    = &(VkPipelineColorBlendStateCreateInfo) {
            .sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOpEnable     = VK_FALSE,
            .logicOp           = VK_LOGIC_OP_COPY,
            .attachmentCount   = 1,
            .pAttachments      = &(VkPipelineColorBlendAttachmentState) {
                .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | 
                                       VK_COLOR_COMPONENT_G_BIT | 
                                       VK_COLOR_COMPONENT_B_BIT | 
                                       VK_COLOR_COMPONENT_A_BIT,
                .blendEnable         = VK_TRUE,
                .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
                .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                .colorBlendOp        = VK_BLEND_OP_ADD,
                .srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
                .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                .alphaBlendOp        = VK_BLEND_OP_ADD
            },
            .blendConstants[0] = 0.0f,
            .blendConstants[1] = 0.0f,
            .blendConstants[2] = 0.0f,
            .blendConstants[3] = 0.0f
        },
        .pDynamicState       = NULL,
        .layout              = pipelineLayout,
        .subpass             = 0,
        .basePipelineHandle  = VK_NULL_HANDLE,
        .basePipelineIndex   = -1
    };

    VkPipeline graphicsPipeline;
    if (vkCreateGraphicsPipelines(p_vk->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &graphicsPipeline) != VK_SUCCESS) {
        printf("Failed to create graphics pipeline\n");
        exit(EXIT_FAILURE);
    }

    vkDestroyShaderModule(p_vk->device, pipelineInfo.pStages[0].module, NULL);
    vkDestroyShaderModule(p_vk->device, pipelineInfo.pStages[1].module, NULL);

    return graphicsPipeline;
}