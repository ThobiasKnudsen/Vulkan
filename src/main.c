#include "gpi.h"

#define DEBUG
#include "debug.h"


// shaders => (pipeline && layout
// layout => descriptor set
// layout => (vertices && instances && indices)
// layout => image attachment
// (pipeline && layout && descriptor set && instances && image attachment) => cmd



#define ALL_INSTANCE_COUNT 5
#define INDICES_COUNT 5

typedef struct {
    float pos[2];        // middle_x, middle_y
    float size[2];       // width, height
    float rotation;      // rotation angle
    float corner_radius; // pixels
    unsigned int color;      // background color packed as RGBA8
    unsigned int tex_index;  // texture ID and other info
    float tex_rect[4];   // texture rectangle (u, v, width, height)
} InstanceData;

typedef struct {
    float targetWidth;
    float targetHeight;
    // Padding to align to 16 bytes
    float padding[2];
} UniformBufferObject;

const InstanceData all_instances[ALL_INSTANCE_COUNT] = {
    // Instance 0
    {
        .pos = {0.0f, 0.0f},
        .size = {200.0f, 200.0f},
        .rotation = 0.0f,
        .corner_radius = 0.0f,
        .color = 0x00FFFF88, // Red color in RGBA8
        .tex_index = 1,
        .tex_rect = {0.0f, 0.0f, 1.0f, 1.0f}
    },
    // Instance 1
    {
        .pos = {100.0f, 100.0f},
        .size = {100.0f, 100.0f},
        .rotation = 30.0f,
        .corner_radius = 0.0f,
        .color = 0x0000FF88, // Green color in RGBA8
        .tex_index = 1,
        .tex_rect = {0.0f, 0.0f, 1.0f, 1.0f}
    },
    // Instance 2
    {
        .pos = {200.f, 200.0f},
        .size = {100.0f, 200.0f},
        .rotation = 60.0f,
        .corner_radius = 0.0f,
        .color = 0x00FF0088, // Blue color in RGBA8
        .tex_index = 1,
        .tex_rect = {0.0f, 0.0f, 1.0f, 1.0f}
    },
    // Instance 3
    {
        .pos = {200.0f, 200.0f},
        .size = {200.0f, 200.0f},
        .rotation = 0.0f,
        .corner_radius = 0.0f,
        .color = 0xFFFFFF88, // Cyan color in RGBA8
        .tex_index = 1,
        .tex_rect = {0.0f, 0.0f, 0.5f, 1.0f}
    },
    // Instance 4
    {
        .pos = {400.0f, 400.0f},
        .size = {100.0f, 100.0f},
        .rotation = 120.0f,
        .corner_radius = 0.0f,
        .color = 0xFF00FF88, // Yellow color in RGBA8
        .tex_index = 1,
        .tex_rect = {0.0f, 0.0f, 1.0f, 1.0f}
    },
};
unsigned int indices[INDICES_COUNT] = { 0, 1, 2, 3, 4 };

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    Gpi_Context ctx = gpi_Context_Create(800, 600, "Vulkan GUI");

    TRACK(Gpi_Buffer uniform_buffer = gpi_Buffer_Create(&ctx, sizeof(UniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT ));
    UniformBufferObject ubo = {};
    ubo.targetWidth = (float)ctx.p_images[0].extent.width;
    ubo.targetHeight = (float)ctx.p_images[0].extent.height;
    gpi_Buffer_Update(&ctx, uniform_buffer, 0, &ubo, sizeof(UniformBufferObject));

    TRACK(Gpi_Buffer instance_buffer = gpi_Buffer_Create(&ctx, sizeof(InstanceData) * ALL_INSTANCE_COUNT, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT ));
    VERIFY(instance_buffer.buffer!=VK_NULL_HANDLE,  "instance_buffer is VK_NULL_HANDLE");
    TRACK(gpi_Buffer_Update(&ctx, instance_buffer, 0, all_instances, 5 * sizeof(InstanceData)));

    TRACK(Gpi_Image image = gpi_Image_Create_FromImageFile(&ctx, "./images/Bitcoin.png", VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));

    /*
    TRACK(SpvShader vert_shader = vk_SpvShader_CreateFromGlslFile(&ctx, "shaders/shader.vert.glsl", shaderc_vertex_shader));
    TRACK(SpvShader frag_shader = vk_SpvShader_CreateFromGlslFile(&ctx, "shaders/shader.frag.glsl", shaderc_fragment_shader));
    TRACK(Vk_GraphicsPipeline pipeline = Vk_GraphicsPipeline_Initialize(&ctx));
    TRACK(Vk_GraphicsPipeline_AddShader(&pipeline, vert_shader, shaderc_vertex_shader));
    TRACK(Vk_GraphicsPipeline_AddShader(&pipeline, frag_shader, shaderc_fragment_shader));
    TRACK(Vk_GraphicsPipeline_CreatePipeline_0(&pipeline, VK_FORMAT_B8G8R8A8_SRGB));
    */

    Gpi_Shader shaders[2];
    TRACK(shaders[0] = gpi_Shader_CreateFromGlslFile(&ctx, "shaders/shader.vert.glsl", shaderc_vertex_shader));
    TRACK(shaders[1] = gpi_Shader_CreateFromGlslFile(&ctx, "shaders/shader.frag.glsl", shaderc_fragment_shader));
    TRACK(Gpi_Pipeline_Graphics pipeline = gpi_Pipeline_Graphics_Create(&ctx, shaders, 2, VK_FORMAT_B8G8R8A8_SRGB));
    TRACK(Gpi_DescriptorSets desc_sets_0 = gpi_DescriptorSets_Create(&pipeline));
    TRACK(gpi_DescriptorSets_UpdateUniformBuffer(&desc_sets_0, 0, 0, 0, &uniform_buffer, 0, VK_WHOLE_SIZE));
    TRACK(gpi_DescriptorSets_UpdateCombinedImageSampler(&desc_sets_0, 1, 0, 0, &image));

    Gpi_Buffer indirect_buffer = gpi_Buffer_Create(&ctx, sizeof(Gpi_DrawIndirectCommand), VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    Gpi_DrawIndirectCommand draw_cmd_data = {
        .vertex_count   = 4,
        .instance_count = 5, 
        .first_vertex   = 0,
        .first_instance = 0
    };
    gpi_Buffer_Update(&ctx, indirect_buffer, 0, &draw_cmd_data, sizeof(draw_cmd_data));

    //TRACK(VkCommandBuffer* swapChainCommandBuffers = alloc(NULL, sizeof(VkCommandBuffer) * ctx.images_count));
    TRACK(Gpi_Cmd* swap_chain_cmds = alloc(NULL, sizeof(Gpi_Cmd) * ctx.images_count));
    for (unsigned int i = 0; i < ctx.images_count; ++i) {
        TRACK(swap_chain_cmds[i] = gpi_Cmd_CreateAndStartRecording(&ctx));
        TRACK(gpi_Cmd_RecordStaticRendering_Indirect(
            &swap_chain_cmds[i], &ctx, &ctx.p_images[i], desc_sets_0.p_desc_sets, desc_sets_0.desc_sets_count,
            pipeline.graphics_pipeline, pipeline.pipeline_layout, instance_buffer.buffer, 5, indirect_buffer));
        TRACK(gpi_Cmd_StopRecording(&swap_chain_cmds[i]));
    }

    TRACK(gpi_Context_StartApplication(
        &ctx,
        swap_chain_cmds,
        indirect_buffer
    ));

    return 0;
}
