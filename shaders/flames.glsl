#include "worldinfo.glsl"
#if defined VERT

layout(location = 0) in vec3 attrPosition;

layout(location = 0) out vec3 outPosition;
layout(location = 1) out vec3 outWorldPosition;

layout(location = 0) uniform mat4 worldMatrix;

void main()
{
    outPosition = attrPosition;
    outWorldPosition = (worldMatrix * vec4(attrPosition, 1.0)).xyz;
}

#elif defined FRAG

#include "lighting.glsl"

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec3 inPosition;

layout(binding = 0) uniform sampler2D texSampler;

layout(location = 1) uniform float offset;
layout(location = 2) uniform float alpha;

void main()
{
    float a = texture(texSampler,
            vec2(-inPosition.x * 0.5 - time * 3.0 + offset, inPosition.y * 0.5 + offset)).r;
    outColor = vec4(vec3(0.95, 0.47, 0.01) * 4.0, a * pow(1.0 - min(-inPosition.x * 0.5f, 1.0), 2.2) * alpha);
}

#elif defined GEOM

layout(triangles, invocations = VIEWPORT_COUNT) in;
layout(triangle_strip, max_vertices = 3) out;

layout(location = 0) out vec3 outPosition;

layout(location = 0) in vec3 inPosition[];
layout(location = 1) in vec3 inWorldPosition[];

void main()
{
    for (uint i=0; i<3; ++i)
    {
        gl_Layer = gl_InvocationID;
        gl_Position = cameraViewProjection[gl_InvocationID] * vec4(inWorldPosition[i], 1.0);
        outPosition = inPosition[i];
        EmitVertex();
    }

    EndPrimitive();
}

#endif
