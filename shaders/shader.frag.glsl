#version 450

layout(location = 0) in vec4        fragColor;
layout(location = 1) in vec2        fragTexCoord;
layout(location = 2) flat in uint   fragTexIndex;
layout(location = 3) in float       fragCornerRadiusWidth;
layout(location = 4) in float       fragCornerRadiusHeight;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D textures[16];

void main() {

    vec4 textureColor = texture(textures[fragTexIndex - 1], fragTexCoord);
    vec4 color = 
        fragTexIndex == 0 || fragTexIndex >= 16 ? 
        fragColor : 
        fragColor*textureColor;

    float x = fragTexCoord.x; x = x>0.5 ? 1.0-x : x;
    float y = fragTexCoord.y; y = y>0.5 ? 1.0-y : y;
    float edge = 0.003;

    if (fragCornerRadiusWidth < 0.0001 || fragCornerRadiusHeight < 0.0001 || x >= fragCornerRadiusWidth+edge || y >= fragCornerRadiusHeight + edge) {
        outColor = color;
        return;
    }
    x=x-fragCornerRadiusWidth*0.98;
    y=y-fragCornerRadiusHeight*0.98;
    float a = (pow(x/fragCornerRadiusWidth,2)) + (pow(y/fragCornerRadiusHeight,2));
    if (a < 1.0) {
        if (a > 0.9) {
            outColor = vec4(color.rgb, color.a*(1.0-a)*10.0);
            return;
        }
        outColor = color;
        return;
    }
    outColor = vec4(0.0,0.0,0.0,0.0);
}