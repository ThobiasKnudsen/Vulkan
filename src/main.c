#include "vk.h"

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    Vk vk = vk_Create(800, 600, "Vulkan GUI");

    TRACK(Buffer uniform_buffer = vk_Buffer_Create(&vk, sizeof(UniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT ));
    UniformBufferObject ubo = {};
    ubo.targetWidth = (float)vk.p_images[0].extent.width;
    ubo.targetHeight = (float)vk.p_images[0].extent.height;
    vk_Buffer_Update(&vk, uniform_buffer, 0, &ubo, sizeof(UniformBufferObject));

    TRACK(Buffer instance_buffer = vk_Buffer_Create(&vk, sizeof(InstanceData) * ALL_INSTANCE_COUNT, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT ));
    VERIFY(instance_buffer.buffer!=VK_NULL_HANDLE,  "instance_buffer is VK_NULL_HANDLE");

    TRACK(Image image = vk_Image_CreateFromImageFile(&vk, "/home/tk/dev/Vulkan/images/Bitcoin.png", VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
    printf("create image\n");

    /*
    TRACK(SpvShader vert_shader = vk_SpvShader_CreateFromGlslFile(&vk, "shaders/shader.vert.glsl", shaderc_vertex_shader));
    TRACK(SpvShader frag_shader = vk_SpvShader_CreateFromGlslFile(&vk, "shaders/shader.frag.glsl", shaderc_fragment_shader));
    TRACK(Vk_GraphicsPipeline pipeline = Vk_GraphicsPipeline_Initialize(&vk));
    TRACK(Vk_GraphicsPipeline_AddShader(&pipeline, vert_shader, shaderc_vertex_shader));
    TRACK(Vk_GraphicsPipeline_AddShader(&pipeline, frag_shader, shaderc_fragment_shader));
    TRACK(Vk_GraphicsPipeline_CreatePipeline_0(&pipeline, VK_FORMAT_B8G8R8A8_SRGB));
    */

    // FOR TOMORROW
    // 1. try only using one descriptor
    // 2. remove dynamic scissor and viewport
    // 3. remove indirect comparibility

    
    Vk_GraphicsPipeline                    p;
    
    SpvReflectShaderModule                  spv_reflect_shader_modules[2];
    TRACK(spv_reflect_shader_modules[0] = vk_SpvReflectShaderModule_Create(vk_SpvShader_CreateFromGlslFile(&vk, "shaders/shader.frag.glsl", shaderc_fragment_shader)));
    TRACK(spv_reflect_shader_modules[1] = vk_SpvReflectShaderModule_Create(vk_SpvShader_CreateFromGlslFile(&vk, "shaders/shader.vert.glsl", shaderc_vertex_shader)));
    TRACK(p.p_desc_sets_layout_create_info = vk_DescriptorSetLayoutCreateInfo_Create(&vk, spv_reflect_shader_modules, 2, &p.desc_sets_count));
    TRACK(p.p_desc_sets_layout = vk_DescriptorSetLayout_Create(&vk, p.p_desc_sets_layout_create_info, p.desc_sets_count));
    TRACK(p.pipeline_layout = vk_PipelineLayout_Create(vk.device, p.p_desc_sets_layout, p.desc_sets_count));
    TRACK(p.graphics_pipeline = vk_Pipeline_Graphics_Create(&vk, p.pipeline_layout));
    
    
    /*
    TRACK(VkCommandBuffer* swapChainCommandBuffers = alloc(NULL, sizeof(VkCommandBuffer) * vk.images_count));
    TRACK(Vk_Rendering* p_renderings = alloc(NULL, sizeof(Vk_Rendering) * vk.images_count));
    for (unsigned int i = 0; i < vk.images_count; ++i) {
        TRACK(p_renderings[i] = Vk_Rendering_Create());
        TRACK(Vk_Rendering_SetGraphicsPipeline(&p_renderings[i], &p, uniform_buffer.buffer, &image));
        //TRACK(Vk_Rendering_UpdateUniformBuffer(&p_renderings[i], 0, 0, 0, uniform_buffer.buffer, 0, uniform_buffer.size));
        //TRACK(Vk_Rendering_UpdateCombinedImageSampler(&p_renderings[i], 1, 0, 0, &image));
        TRACK(Vk_Rendering_SetTargetImage(&p_renderings[i], &vk.p_images[i]));
        TRACK(Vk_Rendering_UpdateInstanceBufferWithBuffer(&p_renderings[i], 0, instance_buffer, 0, instance_buffer.size));
        TRACK(Vk_Rendering_UpdateInstanceDrawRange(&p_renderings[i], 0, 5));
        TRACK(Vk_Rendering_RecordCommandBuffer_0(&p_renderings[i]));
        swapChainCommandBuffers[i] = p_renderings[i].command_buffer;
    }
    */  
    
    TRACK(VkDescriptorSet*                  p_desc_sets = vk_DescriptorSet_Create_0(&vk, p.p_desc_sets_layout, p.desc_sets_count, uniform_buffer.buffer, &image));
    
    TRACK(VkCommandBuffer*                  swapChainCommandBuffers = vk_CommandBuffer_CreateForSwapchain(
        &vk,
        p_desc_sets,
        p.desc_sets_count,
        p.graphics_pipeline,
        p.pipeline_layout,
        instance_buffer.buffer,
        &image
    ));
    

    /*
    Vk_GraphicsPipeline                    p;
    SpvReflectShaderModule                  spv_reflect_shader_modules[2];
    TRACK(spv_reflect_shader_modules[0] = vk_SpvReflectShaderModule_Create(vk_SpvShader_CreateFromGlslFile(&vk, "shaders/shader.frag.glsl", shaderc_fragment_shader)));
    TRACK(spv_reflect_shader_modules[1] = vk_SpvReflectShaderModule_Create(vk_SpvShader_CreateFromGlslFile(&vk, "shaders/shader.vert.glsl", shaderc_vertex_shader)));
    size_t                                  desc_create_info_count;
    TRACK(VkDescriptorSetLayoutCreateInfo*  p_desc_set_layout_create_info = vk_DescriptorSetLayoutCreateInfo_Create(&vk, spv_reflect_shader_modules, 2, &desc_create_info_count));
    TRACK(VkDescriptorSetLayout*            p_desc_set_layout = vk_DescriptorSetLayout_Create(&vk, p_desc_set_layout_create_info, desc_create_info_count));
    TRACK(VkDescriptorSet*                  p_desc_set = vk_DescriptorSet_Create_0(&vk, p_desc_set_layout, desc_create_info_count, uniform_buffer.buffer, &image));

    TRACK(VkPipelineLayout                  graphicsPipelineLayout = vk_PipelineLayout_Create(vk.device, p_desc_set_layout, desc_create_info_count));
    TRACK(VkPipeline                        graphicsPipeline = vk_Pipeline_Graphics_Create(&vk, graphicsPipelineLayout));
    
    TRACK(VkCommandBuffer*                  swapChainCommandBuffers = vk_CommandBuffer_CreateForSwapchain(
        &vk,
        p_desc_set,
        desc_create_info_count,
        graphicsPipeline,
        graphicsPipelineLayout,
        instance_buffer.buffer,
        &image
    ));
    */
    TRACK(VkSemaphore imageAvailableSemaphore = vk_Semaphore_Create(vk.device));
    TRACK(VkSemaphore renderFinishedSemaphore = vk_Semaphore_Create(vk.device));
    TRACK(VkFence inFlightFence = vk_Fence_Create(vk.device));
    TRACK(vk_StartApp(
        &vk,
        imageAvailableSemaphore,
        renderFinishedSemaphore,
        inFlightFence,
        swapChainCommandBuffers,
        uniform_buffer.buffer,
        uniform_buffer.allocation,
        instance_buffer
    ));
    /*
    TRACK(vk_Destroy(
        &vk,
        graphicsPipeline,
        graphicsPipelineLayout,
        p_desc_set_layout[0],
        uniform_buffer.buffer,
        uniform_buffer.allocation,
        instance_buffer.buffer,
        instance_buffer.allocation,
        p_desc_set[0],
        swapChainCommandBuffers,
        imageAvailableSemaphore,
        renderFinishedSemaphore,
        inFlightFence
    ));*/
    TRACK(vk_Destroy(
        &vk,
        p.graphics_pipeline,
        p.p_desc_sets_layout,
        p.p_desc_sets_layout[0],
        uniform_buffer.buffer,
        uniform_buffer.allocation,
        instance_buffer.buffer,
        instance_buffer.allocation,
        p_desc_sets[0],
        swapChainCommandBuffers,
        imageAvailableSemaphore,
        renderFinishedSemaphore,
        inFlightFence
    ));

    return 0;
}
