#include "worldinfo.glsl"

#if defined VERT

layout(location = 0) in vec3 attrPosition;

layout(location = 0) out vec3 outWorldPosition;

layout(location = 0) uniform mat4 worldMatrix;

void main()
{
    outWorldPosition = (worldMatrix * vec4(attrPosition, 1.0)).xyz;
}

#elif defined FRAG

void main() { }

#elif defined GEOM

layout(triangles, invocations = VIEWPORT_COUNT) in;
layout(triangle_strip, max_vertices = 3) out;

layout(location = 0) in vec3 inWorldPosition[];

void main()
{
    for (uint i=0; i<3; ++i)
    {
        gl_Layer = gl_InvocationID;
        gl_Position = shadowViewProjection[gl_InvocationID] * vec4(inWorldPosition[i], 1.0);
        EmitVertex();
    }

    EndPrimitive();
}

#endif
