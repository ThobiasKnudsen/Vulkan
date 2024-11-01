#version 450

// Instance data inputs
layout(location = 0) in vec2 inPos;             // Position in pixels (0 to targetWidth/Height)
layout(location = 1) in vec2 inSize;            // Size in pixels
layout(location = 2) in float inRotation;       // Rotation angle in degrees
layout(location = 3) in float inCornerRadius;   // in pixels
layout(location = 4) in vec4 inColor;           // Color as RGBA
layout(location = 5) in uint inTexIndex;         // Texture information (if used)
layout(location = 6) in vec4 inTexRect;         // Texture rectangle (u, v, width, height)

// Outputs to fragment shader
layout(location = 0) out vec4       fragColor;
layout(location = 1) out vec2       fragTexCoord;
layout(location = 2) flat out uint  fragTexIndex;
layout(location = 3) out float      fragCornerRadiusWidth;
layout(location = 4) out float      fragCornerRadiusHeight;

// Uniforms
layout(set = 0, binding = 0) uniform UniformBufferObject {
    float targetWidth;   // Width of the rendering target in pixels
    float targetHeight;  // Height of the rendering target in pixels
    float padding[2];    // Padding for 16-byte alignment (if using std140)
} ubo;

void main() {
    vec2 positions[4] = vec2[](
        vec2(0.0, 0.0),                // Top-Left
        vec2(inSize.x, 0.0),           // Top-Right
        vec2(0.0, inSize.y),           // Bottom-Left
        vec2(inSize.x, inSize.y)       // Bottom-Right
    );
    uint vertexIndex = gl_VertexIndex % 4;
    vec2 pos = positions[vertexIndex];
    vec2 center = inSize * 0.5;
    vec2 centeredPos = pos - center;
    float rad = radians(inRotation);
    mat2 rotationMatrix = mat2(
        cos(rad), -sin(rad),
        sin(rad),  cos(rad)
    );
    vec2 rotatedPos = rotationMatrix * centeredPos;
    vec2 finalPos = rotatedPos + center + inPos;
    vec2 ndcPos = (finalPos / vec2(ubo.targetWidth, ubo.targetHeight)) * 2.0 - 1.0;
    gl_Position = vec4(ndcPos, 0.0, 1.0);

    
    fragColor = inColor;
    fragTexCoord = inTexRect.xy + (pos / inSize) * inTexRect.zw;
    fragTexIndex = inTexIndex;
    fragCornerRadiusWidth = inCornerRadius/float((ubo.targetWidth+ubo.targetHeight)/2.0);
    fragCornerRadiusHeight = inCornerRadius/float((ubo.targetWidth+ubo.targetHeight)/2.0);
}
