
#include <vulkan/vulkan.h>

typedef enum {
	NONE,
	ID,
	IMAGE,
	WINDOW,
	BOX,
	GROUP,
	RENDERER,
	FUNCTION
} GUI_TYPE;

typedef struct {
    unsigned int    number;
    GUI_TYPE        type;
    unsigned int    index;
} Gui_ID;

const Gui_ID GUI_NULL_ID = {
	.number = 0,
	.type = NONE,
	.index = 0
};

// =============================================================================================================================
void 				Gui_Initialize(unsigned int width, unsigned int height, const char* title);
void   				Gui_StartApplication();
void  				Gui_StopAppliation();
void  				Gui_Destroy();

// box =========================================================================================================================
Gui_ID    			Gui_Box_Create(float x, float y, float width, float height, unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha, float rotation_radians, float corner_radius_pixels, Gui_ID sample_image, float sample_x, float sample_y, float sample_width, float sample_height);
Gui_ID    			Gui_Box_CreateEmpty();
void   				Gui_Box_Copy(Gui_ID src_box, Gui_ID dst_box);
void   				Gui_Box_SetPosition(Gui_ID box, float x, float y);
void   				Gui_Box_AddPosition(Gui_ID box, float d_x, float d_y);
void   				Gui_Box_SetSize(Gui_ID box, float width, float height);
void   				Gui_Box_AddSize(Gui_ID box, float d_width, float d_height);
void    			Gui_Box_SetColor(Gui_ID box, unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha);
void   				Gui_Box_SetRotation(Gui_ID box, float rotation_radians);
void   				Gui_Box_AddRotation(Gui_ID box, float d_rotation_radians);
void   				Gui_Box_SetCornerRadius(Gui_ID box, float corner_radius_pixels);
void   				Gui_Box_SetImage(Gui_ID box, Gui_ID image);
void   				Gui_Box_SetImageSamplePos(Gui_ID box, float x, float y);
void   				Gui_Box_SetImageSampleSize(Gui_ID box, float width, float height);
void   				Gui_Box_Destroy(Gui_ID box);

// image ========================================================================================================================
Gui_ID  			Gui_Image_Create(unsigned int width, unsigned int height, VkFormat format);
Gui_ID   			Gui_Image_GetWindowImage();
void 	  			Gui_Image_Write(Gui_ID image, void* p_src_data, unsigned int src_width, unsigned int src_height, unsigned int src_x, unsigned int src_y);
void   				Gui_Image_Read(Gui_ID image, void* p_dst_data, unsigned int dst_width, unsigned int dst_height, unsigned int dst_x, unsigned int dst_y);
void   				Gui_Image_Destroy(Gui_ID image);  

// group ========================================================================================================================
// group can contain box, group, function and render.
Gui_ID   			Gui_Group_Create(Gui_ID* p_elements, size_t elements_count);
void    			Gui_Group_Copy(Gui_ID src_group, Gui_ID dst_group);
void   				Gui_Group_GetWindowGroup();
void  				Gui_Group_AppendElement(Gui_ID group, Gui_ID new_element);
void 				Gui_Group_RemoveElement(Gui_ID group, Gui_ID element);
void   				Gui_Group_InsertElement(Gui_ID group, Gui_ID new_element, size_t index);
void   				Gui_Group_Move(Gui_ID group, float d_x, float d_y);
void   				Gui_Group_Rotate(Gui_ID group, float rotation_radians);
void  				Gui_Group_Destroy(Gui_ID group);

// render =======================================================================================================================
// window contains renderer and this is the ground renderer which starts all rendering. If you want other renderer to render you 
// have two options. 1. the group within the window renderer can contain other renderers which will be rendered when the parent
// window renderer is rendered. similarly will any child renderer be rendered when the parent is rendered 
// 2. you can manually call a renderer to render
Gui_ID   			Gui_Renderer_Create(Gui_ID target_image, Gui_ID group);
Gui_ID  			Gui_Renderer_GetWindowRender();
void   				Gui_Renderer_Render(Gui_ID render);
void    			Gui_Renderer_Destroy(Gui_ID render);

// function =====================================================================================================================
// function can be added to group. It is executed every iteration the function is "drawn" before all elements that actually gets drawn in the same render is drawn
Gui_ID   			Gui_Function_Create(void (*p_fn)(void*, size_t), void* p_data, size_t data_size);
void  				Gui_Function_Destroy(Gui_ID function);