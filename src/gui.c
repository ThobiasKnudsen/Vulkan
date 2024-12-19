/*
#include "gui.h"

typedef enum {
    NONE,
    IMAGE,
    BOX,
    GROUP,
    RENDERING,
    FUNCTION,
    WINDOW
} Gui_Type;

typedef struct {
    unsigned int    id;
    Gui_Type        type;
    unsigned int    loc;
} Gui_ID;

typedef struct {
    float           pos[2];         // middle_x, middle_y
    float           size[2];        // width, height
    float           rotation;       // rotation angle
    float           corner_radius;  // pixels
    unsigned int    color;          // background color packed as RGBA8
    unsigned int    tex_index;      // texture ID and other info
    float           tex_rect[4];    // texture rectangle (u, v, width, height)
} Gui_Box_Gpu;

typedef struct {
    Gui_ID          id;
    VkImage         image;
    VkImageLayout   layout;
    VkExtent2D      extent;
    VkFormat        format;
    VmaAllocation   allocation;
    VkImageView     image_view;
    VkSampler       sampler;
} Gui_Image;

typedef struct {
    Gui_ID          id;
    Gui_Box_Gpu*    p_box_gpu;      // cannot be NULL
    Gui_Image*      p_image;        // can be NULL    
} Gui_Box_Cpu;

typedef struct {
    Gui_ID          id;
    Gui_ID*         p_elements;         // type can be BOX or GROUP
    size_t          elements_count;
    size_t          elements_capacity;
    Gui_ID*         p_boxes;            // 
    size_t          boxes_count;
    size_t          boxes_capacity;
} Gui_Group;

typedef struct {
    VkCommandBuffer command_buffer;
    Gui_Image*      p_image_target;
    Gui_Group*      p_group;
} Gui_Rendering;
*/