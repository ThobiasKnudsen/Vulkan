#include "vk.h"

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    Vk vk = vk_Create(800, 600, "Vulkan GUI");

    TRACK(Buffer uniform_buffer = vk_Buffer_Create(&vk, sizeof(UniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT ));
    TRACK(Buffer instance_buffer = vk_Buffer_Create(&vk, sizeof(InstanceData) * ALL_INSTANCE_COUNT, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT ));

    TRACK(Image image = vk_Image_CreateFromImageFile(&vk, "/home/tk/dev/Vulkan/images/Bitcoin.png", VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
    printf("create image\n");

    TRACK(SpvShader vert_shader = vk_SpvShader_CreateFromGlslFile(&vk, "shaders/shader.vert.glsl", shaderc_vertex_shader));
    TRACK(SpvShader frag_shader = vk_SpvShader_CreateFromGlslFile(&vk, "shaders/shader.frag.glsl", shaderc_fragment_shader));
    TRACK(Gui_GraphicsPipeline pipeline = Gui_GraphicsPipeline_Initialize(&vk));
    TRACK(Gui_GraphicsPipeline_AddShader(&pipeline, vert_shader, shaderc_vertex_shader));
    TRACK(Gui_GraphicsPipeline_AddShader(&pipeline, frag_shader, shaderc_fragment_shader));
    TRACK(Gui_GraphicsPipeline_CreatePipeline(&pipeline));
    
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
    ));

    return 0;
}
