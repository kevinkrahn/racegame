#include "worldinfo.glsl"

#if defined VERT

layout(location = 0) in vec3 attrPosition;
layout(location = 1) in vec3 attrNormal;
layout(location = 2) in vec3 attrColor;
layout(location = 3) in vec2 attrTexCoord;

layout(location = 0) uniform mat4 worldMatrix;
layout(location = 1) uniform mat3 normalMatrix;

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outTexCoord;
layout(location = 3) out vec3 outWorldPosition;

void main()
{
    outColor = attrColor;
    outNormal = normalize(normalMatrix * attrNormal);
    outTexCoord = attrTexCoord;
    outWorldPosition = (worldMatrix * vec4(attrPosition, 1.0)).xyz;
}

#elif defined FRAG

#include "lighting.glsl"

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec3 inColor;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inWorldPosition;
layout(location = 4) in vec3 inShadowCoord;

layout(location = 2) uniform vec3 color;

void main()
{
    outColor = lighting(vec4(inColor * color, 1.0), normalize(inNormal), inShadowCoord, inWorldPosition);
}

#elif defined GEOM

layout(triangles, invocations = VIEWPORT_COUNT) in;
layout(triangle_strip, max_vertices = 3) out;

layout(location = 0) in vec3 inColor[];
layout(location = 1) in vec3 inNormal[];
layout(location = 2) in vec2 inTexCoord[];
layout(location = 3) in vec3 inWorldPosition[];

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outTexCoord;
layout(location = 3) out vec3 outWorldPosition;
layout(location = 4) out vec3 outShadowCoord;

void main()
{
    for (uint i=0; i<3; ++i)
    {
        gl_Layer = gl_InvocationID;
        gl_Position = cameraViewProjection[gl_InvocationID] * vec4(inWorldPosition[i], 1.0);
        outColor = inColor[i];
        outNormal = inNormal[i];
        outTexCoord = inTexCoord[i];
        outWorldPosition = inWorldPosition[i];
        outShadowCoord = (shadowViewProjectionBias[gl_InvocationID] * vec4(inWorldPosition[i], 1.0)).xyz;
        EmitVertex();
    }

    EndPrimitive();
}

#endif
