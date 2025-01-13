#include "vk.h"

Vk_GraphicsPipeline Vk_GraphicsPipeline_Initialize(
    Vk* p_vk) 
{
    VERIFY(p_vk, "NULL pointer passed to Vk_GraphicsPipeline_Create");
    Vk_GraphicsPipeline rendering;
    memset(&rendering, 0, sizeof(Vk_GraphicsPipeline));
    rendering.p_vk = p_vk; 
    return rendering;
}

void Vk_GraphicsPipeline_AddShader(
    Vk_GraphicsPipeline* p_pipeline, 
    SpvShader spv_shader, 
    shaderc_shader_kind shader_kind) 
{
    VERIFY(p_pipeline, "NULL pointer passed to Vk_GraphicsPipeline_AddShader");
    VERIFY(spv_shader.code, "NULL pointer passed as SPV code");
    VERIFY(spv_shader.size > 0, "SPIR-V code size is zero");

    // Ensure uniqueness of the shader stage
    for (unsigned int i = 0; i < p_pipeline->shaders_count; ++i) {
        VERIFY(shader_kind != p_pipeline->p_shaders[i].shader_kind, 
               "A shader of the given shader_kind already exists in this Vk_GraphicsPipeline instance");
    }

    p_pipeline->shaders_count++;
    TRACK(p_pipeline->p_shaders = alloc(p_pipeline->p_shaders, p_pipeline->shaders_count * sizeof(Gui_Shader)));

    Gui_Shader* p_shader = &p_pipeline->p_shaders[p_pipeline->shaders_count - 1];
    TRACK(p_shader->p_spv_code = alloc(NULL, spv_shader.size * sizeof(unsigned int)));
    memcpy((void*)p_shader->p_spv_code, spv_shader.code, spv_shader.size);

    p_shader->spv_code_size = spv_shader.size;
    p_shader->shader_kind   = shader_kind;

    // Create SPIRV-Reflect module
    TRACK(SpvReflectResult result_0 = spvReflectCreateShaderModule(spv_shader.size, spv_shader.code, &p_shader->reflect_shader_module));
    VERIFY(result_0 == SPV_REFLECT_RESULT_SUCCESS, "Failed to create SPIRV-Reflect shader module");

    // Create Vulkan shader module
    VkShaderModuleCreateInfo create_info = {
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = p_shader->spv_code_size,
        .pCode    = p_shader->p_spv_code
    };
    TRACK(VkResult result_1 = vkCreateShaderModule(p_pipeline->p_vk->device, &create_info, NULL, &p_shader->shader_module));
    VERIFY(result_1 == VK_SUCCESS, "Failed to create Vulkan shader module");
}

void Vk_GraphicsPipeline_CreatePipeline(
    Vk_GraphicsPipeline* p_pipeline,
    VkFormat format) 
{

    VERIFY(p_pipeline, "NULL pointer passed to Vk_GraphicsPipeline_CreateDescriptorSets");
    VERIFY(p_pipeline->shaders_count >= 2, "At least vertex and fragment shaders are required");

    // Gather all reflect shader modules
    TRACK(SpvReflectShaderModule* p_shader_modules = alloc(NULL, p_pipeline->shaders_count * sizeof(SpvReflectShaderModule)));
    for (unsigned int i = 0; i < p_pipeline->shaders_count; ++i) {
        p_shader_modules[i] = p_pipeline->p_shaders[i].reflect_shader_module;
    }

    // Create descriptor set layouts from reflection
    TRACK(p_pipeline->p_desc_sets_layout_create_info = vk_DescriptorSetLayoutCreateInfo_Create(
        p_pipeline->p_vk, 
        p_shader_modules, 
        (unsigned int)p_pipeline->shaders_count, 
        &p_pipeline->desc_sets_count));

    // Create the descriptor set layouts
    TRACK(p_pipeline->p_desc_sets_layout = vk_DescriptorSetLayout_Create(
        p_pipeline->p_vk, 
        p_pipeline->p_desc_sets_layout_create_info, 
        p_pipeline->desc_sets_count));

    TRACK(free(p_shader_modules));

    // Create pipeline layout
    VkPipelineLayoutCreateInfo vk_pipeline_layoutInfo = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount         = (uint32_t)p_pipeline->desc_sets_count,
        .pSetLayouts            = p_pipeline->p_desc_sets_layout,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges    = NULL
    };

    TRACK(VkResult result = vkCreatePipelineLayout(p_pipeline->p_vk->device, &vk_pipeline_layoutInfo, NULL, &p_pipeline->pipeline_layout));
    VERIFY(result == VK_SUCCESS, "Failed to create pipeline layout");

    // Find vertex shader index
    unsigned int vertex_index = 0;
    for (; vertex_index < p_pipeline->shaders_count; ++vertex_index) {
        if (p_pipeline->p_shaders[vertex_index].shader_kind == shaderc_vertex_shader) {
            break;
        }
    }
    VERIFY(vertex_index < p_pipeline->shaders_count, "Could not find vertex shader");

    uint32_t vertex_attrib_count;
    uint32_t vertex_binding_stride;
    SpvShader spv_vertex_shader = {
        .code = p_pipeline->p_shaders[vertex_index].p_spv_code,
        .size = p_pipeline->p_shaders[vertex_index].spv_code_size,
    };

    // Reflect vertex input attributes
    TRACK(VkVertexInputAttributeDescription* vertex_input_attrib_desc = vk_VertexInputAttributeDescriptions_CreateFromVertexShader(
        spv_vertex_shader, 
        &vertex_attrib_count,
        &vertex_binding_stride
    ));

    // Set up shader stages
    const unsigned int stage_count = (unsigned int)p_pipeline->shaders_count;
    TRACK(VkPipelineShaderStageCreateInfo* p_stages = alloc(NULL, stage_count * sizeof(VkPipelineShaderStageCreateInfo)));
    VERIFY(p_stages, "Failed to allocate memory for pipeline stages");
    memset(p_stages, 0, stage_count * sizeof(VkPipelineShaderStageCreateInfo));

    for (unsigned int i = 0; i < stage_count; ++i) {
        p_stages[i].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        p_stages[i].pName  = "main";
        p_stages[i].module = p_pipeline->p_shaders[i].shader_module;

        if (p_pipeline->p_shaders[i].shader_kind == shaderc_fragment_shader) {
            p_stages[i].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        } else if (p_pipeline->p_shaders[i].shader_kind == shaderc_vertex_shader) {
            p_stages[i].stage = VK_SHADER_STAGE_VERTEX_BIT;
        } else {
            VERIFY(false, "Unsupported shader kind encountered");
        }
    }

    VkPipelineRenderingCreateInfo rendering_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &format,
    };

    // Setting up the dynamic rendering pipeline
    VkGraphicsPipelineCreateInfo pipeline_info = {
        .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .renderPass          = VK_NULL_HANDLE,
        .pNext               = &rendering_info,
        .stageCount          = stage_count,
        .pStages             = p_stages,
        .pVertexInputState   = &(VkPipelineVertexInputStateCreateInfo) {
            .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount   = 1,
            .pVertexBindingDescriptions      = (VkVertexInputBindingDescription[1]) {{
                .binding   = 0,
                .stride    = vertex_binding_stride,
                .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE
            }},
            .vertexAttributeDescriptionCount = vertex_attrib_count,
            .pVertexAttributeDescriptions    = vertex_input_attrib_desc,
        },
        .pInputAssemblyState = &(VkPipelineInputAssemblyStateCreateInfo) {
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
            .primitiveRestartEnable = VK_FALSE
        },
        .pViewportState      = &(VkPipelineViewportStateCreateInfo) {
            .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .pViewports    = NULL,
            .scissorCount  = 1,
            .pScissors     = NULL,
        },
        .pDynamicState       = &(VkPipelineDynamicStateCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = 2,
            .pDynamicStates = (VkDynamicState[]) {
                VK_DYNAMIC_STATE_VIEWPORT,
                VK_DYNAMIC_STATE_SCISSOR
            },
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
            .depthWriteEnable       = VK_FALSE,
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
        .layout              = p_pipeline->pipeline_layout,
        .subpass             = 0,
        .basePipelineHandle  = VK_NULL_HANDLE,
        .basePipelineIndex   = -1
    };

    TRACK(result = vkCreateGraphicsPipelines(p_pipeline->p_vk->device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &p_pipeline->graphics_pipeline));
    VERIFY(result == VK_SUCCESS, "Failed to create graphics pipeline");
    free(p_stages);
    free(vertex_input_attrib_desc);
}

void Vk_GraphicsPipeline_CreatePipeline_0(
    Vk_GraphicsPipeline* p_pipeline,
    VkFormat format) 
{
    VERIFY(p_pipeline, "NULL pointer passed to Vk_GraphicsPipeline_CreateDescriptorSets");
    VERIFY(p_pipeline->shaders_count >= 2, "At least vertex and fragment shaders are required");

    // Gather all reflect shader modules
    TRACK(SpvReflectShaderModule* p_shader_modules = alloc(NULL, p_pipeline->shaders_count * sizeof(SpvReflectShaderModule)));
    for (unsigned int i = 0; i < p_pipeline->shaders_count; ++i) {
        p_shader_modules[i] = p_pipeline->p_shaders[i].reflect_shader_module;
    }

    // Create descriptor set layouts from reflection
    TRACK(p_pipeline->p_desc_sets_layout_create_info = vk_DescriptorSetLayoutCreateInfo_Create(
        p_pipeline->p_vk, 
        p_shader_modules, 
        (unsigned int)p_pipeline->shaders_count, 
        &p_pipeline->desc_sets_count));

    // Create the descriptor set layouts
    TRACK(p_pipeline->p_desc_sets_layout = vk_DescriptorSetLayout_Create(
        p_pipeline->p_vk, 
        p_pipeline->p_desc_sets_layout_create_info, 
        p_pipeline->desc_sets_count));

    TRACK(free(p_shader_modules));

    // Create pipeline layout
    VkPipelineLayoutCreateInfo vk_pipeline_layoutInfo = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount         = (uint32_t)p_pipeline->desc_sets_count,
        .pSetLayouts            = p_pipeline->p_desc_sets_layout,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges    = NULL
    };

    TRACK(VkResult result = vkCreatePipelineLayout(p_pipeline->p_vk->device, &vk_pipeline_layoutInfo, NULL, &p_pipeline->pipeline_layout));
    VERIFY(result == VK_SUCCESS, "Failed to create pipeline layout");

    // Find vertex shader index
    unsigned int vertex_index = 0;
    for (; vertex_index < p_pipeline->shaders_count; ++vertex_index) {
        if (p_pipeline->p_shaders[vertex_index].shader_kind == shaderc_vertex_shader) {
            break;
        }
    }
    VERIFY(vertex_index < p_pipeline->shaders_count, "Could not find vertex shader");

    uint32_t vertex_attrib_count;
    uint32_t vertex_binding_stride;
    SpvShader spv_vertex_shader = {
        .code = p_pipeline->p_shaders[vertex_index].p_spv_code,
        .size = p_pipeline->p_shaders[vertex_index].spv_code_size,
    };

    // Reflect vertex input attributes
    TRACK(VkVertexInputAttributeDescription* vertex_input_attrib_desc = vk_VertexInputAttributeDescriptions_CreateFromVertexShader(
        spv_vertex_shader, 
        &vertex_attrib_count,
        &vertex_binding_stride
    ));

    // Set up shader stages
    const unsigned int stage_count = (unsigned int)p_pipeline->shaders_count;
    TRACK(VkPipelineShaderStageCreateInfo* p_stages = alloc(NULL, stage_count * sizeof(VkPipelineShaderStageCreateInfo)));
    VERIFY(p_stages, "Failed to allocate memory for pipeline stages");
    memset(p_stages, 0, stage_count * sizeof(VkPipelineShaderStageCreateInfo));

    for (unsigned int i = 0; i < stage_count; ++i) {
        p_stages[i].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        p_stages[i].pName  = "main";
        p_stages[i].module = p_pipeline->p_shaders[i].shader_module;

        if (p_pipeline->p_shaders[i].shader_kind == shaderc_fragment_shader) {
            p_stages[i].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        } else if (p_pipeline->p_shaders[i].shader_kind == shaderc_vertex_shader) {
            p_stages[i].stage = VK_SHADER_STAGE_VERTEX_BIT;
        } else {
            VERIFY(false, "Unsupported shader kind encountered");
        }
    }

    VkPipelineRenderingCreateInfo rendering_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &format,
    };

    // Match the pipeline configuration from vk_Pipeline_Graphics_Create
    VkViewport viewport = {
        .x        = 0.0f,
        .y        = 0.0f,
        .width    = (float)p_pipeline->p_vk->p_images[0].extent.width,
        .height   = (float)p_pipeline->p_vk->p_images[0].extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = p_pipeline->p_vk->p_images[0].extent
    };

    VkGraphicsPipelineCreateInfo pipeline_info = {
        .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext               = &rendering_info,
        .stageCount          = stage_count,
        .pStages             = p_stages,
        .pVertexInputState   = &(VkPipelineVertexInputStateCreateInfo) {
            .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount   = 1,
            .pVertexBindingDescriptions      = (VkVertexInputBindingDescription[1]) {{
                .binding   = 0,
                .stride    = vertex_binding_stride,
                .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE
            }},
            .vertexAttributeDescriptionCount = vertex_attrib_count,
            .pVertexAttributeDescriptions    = vertex_input_attrib_desc,
        },
        .pInputAssemblyState = &(VkPipelineInputAssemblyStateCreateInfo) {
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
            .primitiveRestartEnable = VK_FALSE
        },
        .pViewportState      = &(VkPipelineViewportStateCreateInfo) {
            .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .pViewports    = &viewport,
            .scissorCount  = 1,
            .pScissors     = &scissor
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
            .depthWriteEnable       = VK_FALSE,
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
        .pDynamicState       = NULL, // Matching the old method: no dynamic viewport/scissor
        .layout              = p_pipeline->pipeline_layout,
        .renderPass          = VK_NULL_HANDLE,
        .subpass             = 0,
        .basePipelineHandle  = VK_NULL_HANDLE,
        .basePipelineIndex   = -1
    };

    TRACK(result = vkCreateGraphicsPipelines(p_pipeline->p_vk->device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &p_pipeline->graphics_pipeline));
    VERIFY(result == VK_SUCCESS, "Failed to create graphics pipeline");

    free(p_stages);
    free(vertex_input_attrib_desc);

    // Note: If you need to destroy the shader modules like in vk_Pipeline_Graphics_Create(), 
    // do so here, or ensure they're destroyed elsewhere after pipeline creation.
}
