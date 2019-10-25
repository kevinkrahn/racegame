#if defined VERT

layout(location = 0) in vec3 attrPosition;
layout(location = 2) in vec3 attrColor;

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec3 outPosition;

void main()
{
    outColor = attrColor;
    outPosition = attrPosition;
}

#elif defined FRAG

layout(location = 0) in vec3 inColor;

layout(location = 0) out vec3 outColor;

layout(location = 1) uniform vec3 color;

void main()
{
    outColor = color * inColor;
}

#elif defined GEOM

#include "worldinfo.glsl"

layout(triangles, invocations = 1) in;
layout(triangle_strip, max_vertices = 3) out;

layout(location = 0) in vec3 inColor[];
layout(location = 1) in vec3 inPosition[];

layout(location = 0) out vec3 outColor;

layout(location = 0) uniform mat4 worldMatrix;
layout(location = 2) uniform int layerIndex;

void main()
{
    float scaleFactor = 0.01;
    mat4 mvp = cameraViewProjection[layerIndex] * worldMatrix;
    float w = (mvp * vec4(0, 0, 0, 1)).w * scaleFactor;
    for (uint i=0; i<3; ++i)
    {
        gl_Layer = layerIndex;
        gl_Position = mvp * vec4(inPosition[i] * w, 1.0);
        outColor = inColor[i];
        EmitVertex();
    }

    EndPrimitive();
}

#endif
