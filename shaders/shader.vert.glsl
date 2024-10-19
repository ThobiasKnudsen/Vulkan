#version 450

layout(location = 0) in vec2 inPosition; // Vertex position input
layout(location = 1) in vec3 inColor;    // Vertex color input

layout(location = 0) out vec3 fragColor; // Color output to fragment shader

layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 view;
    mat4 proj;
} pushConstants;

void main() {
    // Transform the vertex position using model, view, and projection matrices
    gl_Position = pushConstants.proj * pushConstants.view * pushConstants.model * vec4(inPosition, 0.0, 1.0);

    // Pass the vertex color to the fragment shader
    fragColor = inColor;
}
