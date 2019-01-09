#if defined VERT

layout(location = 0) in vec3 attrPosition;
layout(location = 2) in vec3 attrColor;

layout(location = 0) uniform mat4 worldMatrix;

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec3 outWorldPosition;

void main()
{
    outColor = attrColor;
    outWorldPosition = (worldMatrix * vec4(attrPosition, 1.0)).xyz;
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
layout(location = 1) in vec3 inWorldPosition[];

layout(location = 0) out vec3 outColor;

layout(location = 2) uniform int layerIndex;

void main()
{
    for (uint i=0; i<3; ++i)
    {
        gl_Layer = layerIndex;
        gl_Position = cameraViewProjection[layerIndex] * vec4(inWorldPosition[i], 1.0);
        outColor = inColor[i];
        EmitVertex();
    }

    EndPrimitive();
}

#endif
