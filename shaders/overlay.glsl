#if defined VERT

#include "worldinfo.glsl"

layout(location = 0) in vec3 attrPosition;
layout(location = 2) in vec3 attrColor;

layout(location = 0) out vec3 outColor;

layout(location = 0) uniform mat4 worldMatrix;

void main()
{
    float scaleFactor = 0.015;
    mat4 mvp = cameraViewProjection * worldMatrix;
    float w = (mvp * vec4(0, 0, 0, 1)).w * scaleFactor;
    gl_Position = mvp * vec4(attrPosition * w, 1.0);
    outColor = attrColor;
}

#elif defined FRAG

layout(location = 0) in vec3 inColor;

layout(location = 0) out vec3 outColor;

layout(location = 1) uniform vec3 color;

void main()
{
    outColor = color * inColor;
}

#endif
