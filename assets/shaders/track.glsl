#include "worldinfo.glsl"

#if defined VERT

layout(location = 0) in vec3 attrPosition;
layout(location = 1) in vec3 attrNormal;
layout(location = 2) in vec3 attrColor;

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outWorldPosition;

void main()
{
    outColor = attrColor;
    outNormal = attrNormal;
    outWorldPosition = attrPosition;
}

#elif defined FRAG

#include "lighting.glsl"

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec3 inColor;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inWorldPosition;
layout(location = 3) in vec3 inShadowCoord;

layout(binding = 0) uniform sampler2D texSampler;

void main()
{
    const float texScale = 0.2;

    vec3 blending = abs(inNormal);
    blending = normalize(max(blending, 0.00001));
    blending /= vec3(blending.x + blending.y + blending.z);

    vec3 xColor = texture(texSampler, inWorldPosition.yz * texScale).rgb;
    vec3 yColor = texture(texSampler, inWorldPosition.xz * texScale).rgb;
    vec3 zColor = texture(texSampler, inWorldPosition.xy * texScale).rgb;

    vec4 tex = vec4(xColor * blending.x + yColor * blending.y + zColor * blending.z, 1.0);

    //vec4 baseColor = tex * vec4(inColor, 1.0);
    vec4 baseColor = tex;
    outColor = lighting(baseColor, normalize(inNormal), inShadowCoord, inWorldPosition,
            70.0, 0.2, vec3(1.0), -0.13, 0.18, 3.0);
}

#elif defined GEOM

layout(triangles, invocations = VIEWPORT_COUNT) in;
layout(triangle_strip, max_vertices = 3) out;

layout(location = 0) in vec3 inColor[];
layout(location = 1) in vec3 inNormal[];
layout(location = 2) in vec3 inWorldPosition[];

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outWorldPosition;
layout(location = 3) out vec3 outShadowCoord;

void main()
{
    for (uint i=0; i<3; ++i)
    {
        gl_Layer = gl_InvocationID;
        gl_Position = cameraViewProjection[gl_InvocationID] * vec4(inWorldPosition[i], 1.0);
        outColor = inColor[i];
        outNormal = inNormal[i];
        outWorldPosition = inWorldPosition[i];
        outShadowCoord = (shadowViewProjectionBias[gl_InvocationID] * vec4(inWorldPosition[i], 1.0)).xyz;
        EmitVertex();
    }

    EndPrimitive();
}

#endif