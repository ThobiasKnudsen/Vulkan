#include "vk.h"


// Function to read GLSL shader source code from a file
size_t readFile(const char* filename, char** dst_buffer) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("Failed to open shader source file '%s'\n", filename);
        return 0;
    }

    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    *dst_buffer = (char*)alloc(NULL, file_size + 1);
    if (!*dst_buffer) {
        printf("Failed to allocate memory for shader source '%s'\n", filename);
        fclose(file);
        return 0;
    }

    size_t readSize = fread(*dst_buffer, 1, file_size, file);
    (*dst_buffer)[file_size] = '\0'; 

    if (readSize != file_size) {
        printf("Failed to read shader source '%s'\n", filename);
        free(*dst_buffer);
        fclose(file);
        return 0;
    }

    fclose(file);
    return (unsigned int)file_size;
}

// Function to compile GLSL code into SPIR-V
SpvShader createSpvShader(VK* p_vk, const char* p_glsl_code, size_t glsl_size, const char* glsl_filename, shaderc_shader_kind kind) {
    shaderc_compilation_result_t result = shaderc_compile_into_spv(
        p_vk->shaderc_compiler,
        p_glsl_code,
        glsl_size,
        kind,
        glsl_filename,
        "main",
        p_vk->shaderc_options
    );

    if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) {
        printf("Shader compilation error in '%s':\n%s\n", glsl_filename, shaderc_result_get_error_message(result));
        shaderc_result_release(result);
        exit(-1);
    }

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

// Function to create a SPIRV-Reflect shader module from SPIR-V code
SpvReflectShaderModule createSpvReflectShaderModule(SpvShader spv_shader) {
    SpvReflectShaderModule shaderModuleReflection;
    SpvReflectResult reflectResult = spvReflectCreateShaderModule(spv_shader.size, spv_shader.code, &shaderModuleReflection);
    if (reflectResult != SPV_REFLECT_RESULT_SUCCESS) {
        printf("Failed to create SPIRV-Reflect shader module\n");
        exit(-1);
    }

    return shaderModuleReflection;
}

// Function to create a Vulkan shader module from SPIR-V code
VkShaderModule createShaderModule(VK* p_vk, SpvShader spv_shader) {
    VkShaderModuleCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = spv_shader.size,
        .pCode = spv_shader.code
    };

    VkShaderModule shader_module = VK_NULL_HANDLE;
    if (vkCreateShaderModule(p_vk->device, &create_info, NULL, &shader_module) != VK_SUCCESS) {
        printf("Failed to create Vulkan shader module from '%s'\n");
        exit(-1);
    }

    return shader_module;
}

VkShaderModule createShaderModuleFromGlslFile(VK* p_vk, const char* filename, shaderc_shader_kind shader_kind) {

	char* p_glsl_code = NULL;
	size_t glsl_code_size = readFile(filename, &p_glsl_code);
	SpvShader spv_shader = createSpvShader(p_vk, p_glsl_code, glsl_code_size, filename, shader_kind);
	free(p_glsl_code);

    return createShaderModule(p_vk, spv_shader);
}