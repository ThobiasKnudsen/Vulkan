#include "gui.h"
#include "gpi.h"

#define DEBUG
#include "debug.h"

#define ALL_INSTANCE_COUNT 5
#define INDICES_COUNT 5

static Gpi_Context 	ctx = {0};
static Gpi_Cmd*     p_swapchain_cmds = NULL;
static unsigned int swapchain_cmds_count = 0;
static bool   		ctx_created = true;

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

const InstanceData all_instances_2[ALL_INSTANCE_COUNT] = {
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
unsigned int indices_2[INDICES_COUNT] = { 0, 1, 2, 3, 4 };

typedef struct {
	unsigned int  	id_index;
	
	float 			pos[2];
	float  			size[2];
	float 			rotation; 
	float 			corner_radius;
	unsigned int 	color;
	unsigned int   	image_id_index;
	float  			image_sample_rect[4];
} Gui_Box; // 52 bytes

typedef struct {
	unsigned int    id_index;

	float   		size[2];
	VkFormat   		format;
} Gui_Image;	

typedef struct {
	unsigned int  		id_index;

	unsigned int*   	p_id_indices;
	unsigned int  		id_indices_count;
	unsigned int    	id_indices_capacity;

	// ================================================
	// these are found in this group and all sub-groups but not in all sub-renderers
	unsigned int*   	p_id_indices_boxes;
	unsigned int    	id_indices_boxes_count;
	unsigned int    	id_indices_boxes_capacity;

	unsigned int*   	p_id_indices_images;
	unsigned int    	id_indices_images_count;
	unsigned int    	id_indices_image_capacity;

	unsigned int*   	p_id_indices_functions;
	unsigned int    	id_indices_functions_count;
	unsigned int    	id_indices_functions_capacity;
	// ================================================
	// these are found in this group and all sub-groups and sub-renderers
	struct {
		unsigned int    id_index;
		unsigned int  	id_index_parent_renderer;
	}* 					p_renderers;
	unsigned int  		renderers_count;
	unsigned int  		renderers_capacity;
	// ================================================
} Gui_Group;

typedef struct {
	unsigned int    id_index;

	unsigned int    id_index_target_image;
	unsigned int    id_index_group;
	Gpi_Cmd  		rendering_cmd;

	unsigned int*   p_id_indices_image_samples;
	unsigned int    id_indices_image_samples_count;
	unsigned int    id_indices_image_samples_capacity;

	// ================================================
	// this is used when writing to buffer
	struct {
		float 			pos[2];
		float  			size[2];
		float 			rotation; 
		float 			corner_radius;
		unsigned int 	color;
		unsigned int   	tex_index;
		float  			tex_rect[4];
	}* 					p_instances; // 48 bytes per instance
	unsigned int  		instances_count;
	unsigned int  		instances_capacity;

	Gpi_Shader*        	p_shaders;
	unsigned int   		shaders_count;
	Gpi_Buffer 			instance_buffer;
	Gpi_Buffer          indirect_buffer;

} Gui_Renderer;

typedef struct {
	unsigned int   	id_index;

	void 			(*p_fn)(void*, size_t);
	void*   		p_data;
	size_t   		data_size;
} Gui_Function;

void _gui_Group_FindAndOrganizeElements(unsigned int id_index_group);

// =============================================================================================================================
// inside renderer (image), (instance_bufferm indirect_buffer),(pipeline, pipeline_layout), (descriptor_sets) 
// are separate and maybe the user should manually handle these and maybe its easier to implement custom shaders then.

void gui_Initialize(
	const unsigned int width, 
	const unsigned int height, 
	const char* title, 
	const char* vert_shader_path_glsl, 
	const char* frag_shader_path_glsl) 
{
	VERIFY(title, "NULL pointer");
	VERIFY(width < 8192, "window width has to be less than 8192");
	VERIFY(height < 8192, "window height has to be less than 8192");
	VERIFY(!ctx_created, "gui is allready initialized. you should not try to initialize it again");

	ctx = gpi_Context_Create(width, height, title);

	TRACK(Gpi_Buffer uniform_buffer = gpi_Buffer_Create(&ctx, sizeof(UniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT ));
    UniformBufferObject ubo = {};
    ubo.targetWidth = (float)ctx.p_images[0].extent.width;
    ubo.targetHeight = (float)ctx.p_images[0].extent.height;
    gpi_Buffer_Update(&ctx, uniform_buffer, 0, &ubo, sizeof(UniformBufferObject));

    TRACK(Gpi_Buffer instance_buffer = gpi_Buffer_Create(&ctx, sizeof(InstanceData) * ALL_INSTANCE_COUNT, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT ));
    VERIFY(instance_buffer.buffer!=VK_NULL_HANDLE,  "instance_buffer is VK_NULL_HANDLE");
    TRACK(gpi_Buffer_Update(&ctx, instance_buffer, 0, all_instances_2, 5 * sizeof(InstanceData)));

    TRACK(Gpi_Image image = gpi_Image_Create_FromImageFile(&ctx, "./images/Bitcoin.png", VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));

    Gpi_Shader shaders[2];
    TRACK(shaders[0] = gpi_Shader_CreateFromGlslFile(&ctx, vert_shader_path_glsl, shaderc_vertex_shader));
    TRACK(shaders[1] = gpi_Shader_CreateFromGlslFile(&ctx, frag_shader_path_glsl, shaderc_fragment_shader));
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


	ctx_created = true;
}
void Gui_StartApplication() {
	VERIFY(ctx_created, "gui has to be initialized before starting application");
	//gpi_Context_StartApplication(&ctx, );
}
void Gui_StopApplication() {

}
void Gui_Destroy() {
	VERIFY(ctx_created, "gui has to be initialized before attempting to destroy it");
	gpi_Context_Destroy(&ctx);
	ctx_created = false;
}