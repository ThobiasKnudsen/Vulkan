#pragma once

#include <vulkan/vulkan.h>
#include <shaderc/shaderc.h>
#include <vk_mem_alloc.h>
#include "spirv_reflect.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h> 

#define DEBUG
#include "debug.h"
void* alloc(void* ptr, size_t size);