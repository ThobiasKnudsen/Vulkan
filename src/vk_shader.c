 #include "vk.h"


size_t readFile(const char* filename, char** dst_buffer) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("Failed to open shader source file '%s'\n", filename);
        exit(-1);
    }

    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    *dst_buffer = (char*)alloc(NULL, file_size + 1);
    if (!*dst_buffer) {
        printf("Failed to allocate memory for shader source '%s'\n", filename);
        fclose(file);
        exit(-1);
    }

    size_t readSize = fread(*dst_buffer, 1, file_size, file);
    (*dst_buffer)[file_size] = '\0'; 

    if (readSize != file_size) {
        printf("Failed to read shader source '%s'\n", filename);
        free(*dst_buffer);
        fclose(file);
        exit(-1);
    }

    fclose(file);
    return (unsigned int)file_size;
}
SpvShader vk_SpvShader_Create(Vk* p_vk, const char* p_glsl_code, size_t glsl_size, const char* glsl_filename, shaderc_shader_kind shader_kind) {

    // Compile the GLSL code to SPIR-V
    shaderc_compilation_result_t result = shaderc_compile_into_spv(
        p_vk->shaderc_compiler,
        p_glsl_code,
        glsl_size,
        shader_kind,
        glsl_filename,
        "main",
        p_vk->shaderc_options
    );

    // Check for compilation errors
    if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) {
        printf("Shader compilation error in '%s':\n%s\n", glsl_filename, shaderc_result_get_error_message(result));
        shaderc_result_release(result);
        exit(-1);
    }

    // Get the SPIR-V code size and pointer
    size_t spv_code_size = shaderc_result_get_length(result);
    const uint32_t* p_spv_code = (const uint32_t*)shaderc_result_get_bytes(result);

    // Allocate memory for the SPIR-V code
    uint32_t* p_spv_code_copy = (uint32_t*)malloc(spv_code_size);
    if (p_spv_code_copy == NULL) {
        printf("Failed to allocate memory for SPIR-V code\n");
        shaderc_result_release(result);
        exit(-1);
    }
    memcpy(p_spv_code_copy, p_spv_code, spv_code_size);

    // Release the compilation result after copying the code
    shaderc_result_release(result);

    // Create the SpvShader struct
    SpvShader spv_shader = {
        .code = p_spv_code_copy,
        .size = spv_code_size
    };

    if (spv_shader.code == NULL || spv_shader.size == 0) {
        printf("Error: Compiled SPIR-V code is NULL or size is zero\n");
        free(p_spv_code_copy);
        exit(-1);
    }

    return spv_shader;
}
SpvShader vk_SpvShader_CreateFromGlslFile(Vk* p_vk, const char* glsl_filename, shaderc_shader_kind shader_kind) {
    char* p_glsl_code = NULL;
    size_t glsl_code_size = readFile(glsl_filename, &p_glsl_code);
    SpvShader spv_shader = vk_SpvShader_Create(p_vk, p_glsl_code, glsl_code_size, glsl_filename, shader_kind);
    free(p_glsl_code);

    return spv_shader;
}
void vk_SpvShader_CreateSpvFileFromGlslFile(Vk* p_vk, const char* glsl_filename, const char* spv_filename, shaderc_shader_kind shader_kind) {
    SpvShader spv_shader = vk_SpvShader_CreateFromGlslFile(p_vk, glsl_filename, shader_kind);
    FILE* spv_file = fopen(spv_filename, "wb");
    VERIFY(spv_file, "failed to open file");
    size_t written = fwrite(spv_shader.code, 1, spv_shader.size, spv_file);
    VERIFY(written == spv_shader.size, "?");
    fclose(spv_file);
}
SpvReflectShaderModule vk_SpvReflectShaderModule_Create(SpvShader spv_shader) {
    SpvReflectShaderModule shaderModuleReflection;
    SpvReflectResult reflectResult = spvReflectCreateShaderModule(spv_shader.size, spv_shader.code, &shaderModuleReflection);
    if (reflectResult != SPV_REFLECT_RESULT_SUCCESS) {
        printf("Failed to create SPIRV-Reflect shader module\n");
        exit(-1);
    }

    return shaderModuleReflection;
}
VkShaderModule vk_ShaderModule_Create(Vk* p_vk, SpvShader spv_shader) {
    VkShaderModuleCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = spv_shader.size,
        .pCode = spv_shader.code
    };

    VkShaderModule shader_module = VK_NULL_HANDLE;
    if (vkCreateShaderModule(p_vk->device, &create_info, NULL, &shader_module) != VK_SUCCESS) {
        printf("Failed to create Vulkan shader module\n");
        exit(-1);
    }

    return shader_module;
}
VkShaderModule vk_ShaderModule_CreateFromGlslFile(Vk* p_vk, const char* filename, shaderc_shader_kind shader_kind) {

	char* p_glsl_code = NULL;
	size_t glsl_code_size = readFile(filename, &p_glsl_code);
	SpvShader spv_shader = vk_SpvShader_Create(p_vk, p_glsl_code, glsl_code_size, filename, shader_kind);
	free(p_glsl_code);

    return vk_ShaderModule_Create(p_vk, spv_shader);
}

static uint32_t FormatSize(VkFormat format) {
  uint32_t result = 0;
  switch (format) {
    case VK_FORMAT_UNDEFINED:
      result = 0;
      break;
    case VK_FORMAT_R4G4_UNORM_PACK8:
      result = 1;
      break;
    case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
      result = 2;
      break;
    case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
      result = 2;
      break;
    case VK_FORMAT_R5G6B5_UNORM_PACK16:
      result = 2;
      break;
    case VK_FORMAT_B5G6R5_UNORM_PACK16:
      result = 2;
      break;
    case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
      result = 2;
      break;
    case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
      result = 2;
      break;
    case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
      result = 2;
      break;
    case VK_FORMAT_R8_UNORM:
      result = 1;
      break;
    case VK_FORMAT_R8_SNORM:
      result = 1;
      break;
    case VK_FORMAT_R8_USCALED:
      result = 1;
      break;
    case VK_FORMAT_R8_SSCALED:
      result = 1;
      break;
    case VK_FORMAT_R8_UINT:
      result = 1;
      break;
    case VK_FORMAT_R8_SINT:
      result = 1;
      break;
    case VK_FORMAT_R8_SRGB:
      result = 1;
      break;
    case VK_FORMAT_R8G8_UNORM:
      result = 2;
      break;
    case VK_FORMAT_R8G8_SNORM:
      result = 2;
      break;
    case VK_FORMAT_R8G8_USCALED:
      result = 2;
      break;
    case VK_FORMAT_R8G8_SSCALED:
      result = 2;
      break;
    case VK_FORMAT_R8G8_UINT:
      result = 2;
      break;
    case VK_FORMAT_R8G8_SINT:
      result = 2;
      break;
    case VK_FORMAT_R8G8_SRGB:
      result = 2;
      break;
    case VK_FORMAT_R8G8B8_UNORM:
      result = 3;
      break;
    case VK_FORMAT_R8G8B8_SNORM:
      result = 3;
      break;
    case VK_FORMAT_R8G8B8_USCALED:
      result = 3;
      break;
    case VK_FORMAT_R8G8B8_SSCALED:
      result = 3;
      break;
    case VK_FORMAT_R8G8B8_UINT:
      result = 3;
      break;
    case VK_FORMAT_R8G8B8_SINT:
      result = 3;
      break;
    case VK_FORMAT_R8G8B8_SRGB:
      result = 3;
      break;
    case VK_FORMAT_B8G8R8_UNORM:
      result = 3;
      break;
    case VK_FORMAT_B8G8R8_SNORM:
      result = 3;
      break;
    case VK_FORMAT_B8G8R8_USCALED:
      result = 3;
      break;
    case VK_FORMAT_B8G8R8_SSCALED:
      result = 3;
      break;
    case VK_FORMAT_B8G8R8_UINT:
      result = 3;
      break;
    case VK_FORMAT_B8G8R8_SINT:
      result = 3;
      break;
    case VK_FORMAT_B8G8R8_SRGB:
      result = 3;
      break;
    case VK_FORMAT_R8G8B8A8_UNORM:
      result = 4;
      break;
    case VK_FORMAT_R8G8B8A8_SNORM:
      result = 4;
      break;
    case VK_FORMAT_R8G8B8A8_USCALED:
      result = 4;
      break;
    case VK_FORMAT_R8G8B8A8_SSCALED:
      result = 4;
      break;
    case VK_FORMAT_R8G8B8A8_UINT:
      result = 4;
      break;
    case VK_FORMAT_R8G8B8A8_SINT:
      result = 4;
      break;
    case VK_FORMAT_R8G8B8A8_SRGB:
      result = 4;
      break;
    case VK_FORMAT_B8G8R8A8_UNORM:
      result = 4;
      break;
    case VK_FORMAT_B8G8R8A8_SNORM:
      result = 4;
      break;
    case VK_FORMAT_B8G8R8A8_USCALED:
      result = 4;
      break;
    case VK_FORMAT_B8G8R8A8_SSCALED:
      result = 4;
      break;
    case VK_FORMAT_B8G8R8A8_UINT:
      result = 4;
      break;
    case VK_FORMAT_B8G8R8A8_SINT:
      result = 4;
      break;
    case VK_FORMAT_B8G8R8A8_SRGB:
      result = 4;
      break;
    case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A8B8G8R8_UINT_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A8B8G8R8_SINT_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A2R10G10B10_UINT_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A2R10G10B10_SINT_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A2B10G10R10_UINT_PACK32:
      result = 4;
      break;
    case VK_FORMAT_A2B10G10R10_SINT_PACK32:
      result = 4;
      break;
    case VK_FORMAT_R16_UNORM:
      result = 2;
      break;
    case VK_FORMAT_R16_SNORM:
      result = 2;
      break;
    case VK_FORMAT_R16_USCALED:
      result = 2;
      break;
    case VK_FORMAT_R16_SSCALED:
      result = 2;
      break;
    case VK_FORMAT_R16_UINT:
      result = 2;
      break;
    case VK_FORMAT_R16_SINT:
      result = 2;
      break;
    case VK_FORMAT_R16_SFLOAT:
      result = 2;
      break;
    case VK_FORMAT_R16G16_UNORM:
      result = 4;
      break;
    case VK_FORMAT_R16G16_SNORM:
      result = 4;
      break;
    case VK_FORMAT_R16G16_USCALED:
      result = 4;
      break;
    case VK_FORMAT_R16G16_SSCALED:
      result = 4;
      break;
    case VK_FORMAT_R16G16_UINT:
      result = 4;
      break;
    case VK_FORMAT_R16G16_SINT:
      result = 4;
      break;
    case VK_FORMAT_R16G16_SFLOAT:
      result = 4;
      break;
    case VK_FORMAT_R16G16B16_UNORM:
      result = 6;
      break;
    case VK_FORMAT_R16G16B16_SNORM:
      result = 6;
      break;
    case VK_FORMAT_R16G16B16_USCALED:
      result = 6;
      break;
    case VK_FORMAT_R16G16B16_SSCALED:
      result = 6;
      break;
    case VK_FORMAT_R16G16B16_UINT:
      result = 6;
      break;
    case VK_FORMAT_R16G16B16_SINT:
      result = 6;
      break;
    case VK_FORMAT_R16G16B16_SFLOAT:
      result = 6;
      break;
    case VK_FORMAT_R16G16B16A16_UNORM:
      result = 8;
      break;
    case VK_FORMAT_R16G16B16A16_SNORM:
      result = 8;
      break;
    case VK_FORMAT_R16G16B16A16_USCALED:
      result = 8;
      break;
    case VK_FORMAT_R16G16B16A16_SSCALED:
      result = 8;
      break;
    case VK_FORMAT_R16G16B16A16_UINT:
      result = 8;
      break;
    case VK_FORMAT_R16G16B16A16_SINT:
      result = 8;
      break;
    case VK_FORMAT_R16G16B16A16_SFLOAT:
      result = 8;
      break;
    case VK_FORMAT_R32_UINT:
      result = 4;
      break;
    case VK_FORMAT_R32_SINT:
      result = 4;
      break;
    case VK_FORMAT_R32_SFLOAT:
      result = 4;
      break;
    case VK_FORMAT_R32G32_UINT:
      result = 8;
      break;
    case VK_FORMAT_R32G32_SINT:
      result = 8;
      break;
    case VK_FORMAT_R32G32_SFLOAT:
      result = 8;
      break;
    case VK_FORMAT_R32G32B32_UINT:
      result = 12;
      break;
    case VK_FORMAT_R32G32B32_SINT:
      result = 12;
      break;
    case VK_FORMAT_R32G32B32_SFLOAT:
      result = 12;
      break;
    case VK_FORMAT_R32G32B32A32_UINT:
      result = 16;
      break;
    case VK_FORMAT_R32G32B32A32_SINT:
      result = 16;
      break;
    case VK_FORMAT_R32G32B32A32_SFLOAT:
      result = 16;
      break;
    case VK_FORMAT_R64_UINT:
      result = 8;
      break;
    case VK_FORMAT_R64_SINT:
      result = 8;
      break;
    case VK_FORMAT_R64_SFLOAT:
      result = 8;
      break;
    case VK_FORMAT_R64G64_UINT:
      result = 16;
      break;
    case VK_FORMAT_R64G64_SINT:
      result = 16;
      break;
    case VK_FORMAT_R64G64_SFLOAT:
      result = 16;
      break;
    case VK_FORMAT_R64G64B64_UINT:
      result = 24;
      break;
    case VK_FORMAT_R64G64B64_SINT:
      result = 24;
      break;
    case VK_FORMAT_R64G64B64_SFLOAT:
      result = 24;
      break;
    case VK_FORMAT_R64G64B64A64_UINT:
      result = 32;
      break;
    case VK_FORMAT_R64G64B64A64_SINT:
      result = 32;
      break;
    case VK_FORMAT_R64G64B64A64_SFLOAT:
      result = 32;
      break;
    case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
      result = 4;
      break;
    case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
      result = 4;
      break;

    default:
      break;
  }
  return result;
}
VkVertexInputAttributeDescription* vk_VertexInputAttributeDescriptions_CreateFromVertexShader( SpvShader spv_shader, uint32_t* p_attribute_count, uint32_t* p_binding_stride) {

    // Create SPIRV-Reflect shader module
    SpvReflectShaderModule shader_module;
    SpvReflectResult result = spvReflectCreateShaderModule(spv_shader.size, spv_shader.code, &shader_module);
    if (result != SPV_REFLECT_RESULT_SUCCESS) {
        printf("Failed to create SPIRV-Reflect shader module\n");
        exit(EXIT_FAILURE);
    }

    // Ensure the shader module is a vertex shader
    if (shader_module.shader_stage != SPV_REFLECT_SHADER_STAGE_VERTEX_BIT) {
        printf("Provided shader is not a vertex shader\n");
        exit(EXIT_FAILURE);
    }

    // Enumerate input variables
    uint32_t input_var_count = 0;
    result = spvReflectEnumerateInputVariables(&shader_module, &input_var_count, NULL);
    if (result != SPV_REFLECT_RESULT_SUCCESS) {
        printf("Failed to enumerate input variables\n");
        exit(EXIT_FAILURE);
    }

    SpvReflectInterfaceVariable** input_vars = alloc(NULL, input_var_count * sizeof(SpvReflectInterfaceVariable*));
    if (!input_vars) {
        printf("Failed to allocate memory for input variables\n");
        exit(EXIT_FAILURE);
    }

    result = spvReflectEnumerateInputVariables(&shader_module, &input_var_count, input_vars);
    if (result != SPV_REFLECT_RESULT_SUCCESS) {
        printf("Failed to get input variables\n");
        exit(EXIT_FAILURE);
    }

    // Create an array to hold VkVertexInputAttributeDescription
    VkVertexInputAttributeDescription* attribute_descriptions = alloc(NULL, input_var_count * sizeof(VkVertexInputAttributeDescription));
    if (!attribute_descriptions) {
        printf("Failed to allocate memory for vertex input attribute descriptions\n");
        exit(EXIT_FAILURE);
    }

    uint32_t attribute_index = 0;
    for (uint32_t i = 0; i < input_var_count; ++i) {
        SpvReflectInterfaceVariable* refl_var = input_vars[i];

        // Ignore built-in variables
        if (refl_var->decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN) {
            continue;
        }

        VkVertexInputAttributeDescription attr_desc = {};
        attr_desc.location = refl_var->location;
        attr_desc.binding = 0;
        attr_desc.format = (VkFormat)refl_var->format;
        attr_desc.offset = 0; // WILL CALCULATE OFFSET LATER
        attribute_descriptions[attribute_index++] = attr_desc;
    }

    // Update the attribute count
    *p_attribute_count = attribute_index;

    // Sort attributes by location
    for (uint32_t i = 0; i < attribute_index - 1; ++i) {
        for (uint32_t j = 0; j < attribute_index - i - 1; ++j) {
            if (attribute_descriptions[j].location > attribute_descriptions[j + 1].location) {
                VkVertexInputAttributeDescription temp = attribute_descriptions[j];
                attribute_descriptions[j] = attribute_descriptions[j + 1];
                attribute_descriptions[j + 1] = temp;
            }
        }
    }

    // Compute offsets and binding stride
    uint32_t offset = 0;
    for (uint32_t i = 0; i < attribute_index; ++i) {
        VkVertexInputAttributeDescription* attr_desc = &attribute_descriptions[i];
        uint32_t format_size = FormatSize(attr_desc->format);

        if (format_size == 0) {
            printf("Unsupported format for input variable at location %u\n", attr_desc->location);
            continue;
        }

        // Align the offset if necessary (e.g., 4-byte alignment)
        uint32_t alignment = 4;
        offset = (offset + (alignment - 1)) & ~(alignment - 1);
        attr_desc->offset = offset;
        offset += format_size;
    }

    *p_binding_stride = offset;
    free(input_vars);
    spvReflectDestroyShaderModule(&shader_module);

    return attribute_descriptions;
}