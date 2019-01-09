#include "worldinfo.glsl"

#if defined VERT

layout(location = 0) in vec3 attrPosition;
layout(location = 1) in vec3 attrNormal;
layout(location = 2) in vec4 attrColor;
layout(location = 3) in vec2 attrTexCoord;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outTexCoord;
layout(location = 3) out vec3 outWorldPosition;

void main()
{
    outColor = attrColor;
    outNormal = attrNormal;
    outTexCoord = attrTexCoord;
    outWorldPosition = attrPosition;
}

#elif defined FRAG

#include "sample_shadow.glsl"

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec4 inColor;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inShadowCoord;

layout(binding = 2) uniform sampler2DArrayShadow shadowDepthSampler;
layout(binding = 4) uniform sampler2DArray ssaoTexture;

void main()
{
    outColor = inColor;

    float directLight = max(dot(normalize(inNormal), sunDirection)
            * getShadow(shadowDepthSampler, inShadowCoord), 0.0);
    outColor.rgb *= max(directLight, 0.4);
    float ssaoAmount = texture(ssaoTexture, vec3(gl_FragCoord.xy / textureSize(ssaoTexture, 0).xy, gl_Layer)).r;
    outColor.rgb *= clamp(ssaoAmount + directLight * 0.5, 0.0, 1.0);
}

#elif defined GEOM

layout(triangles, invocations = VIEWPORT_COUNT) in;
layout(triangle_strip, max_vertices = 3) out;

layout(location = 0) in vec4 inColor[];
layout(location = 1) in vec3 inNormal[];
layout(location = 2) in vec2 inTexCoord[];
layout(location = 3) in vec3 inWorldPosition[];

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outTexCoord;
layout(location = 3) out vec3 outShadowCoord;

void main()
{
    for (uint i=0; i<3; ++i)
    {
        gl_Layer = gl_InvocationID;
        gl_Position = cameraViewProjection[gl_InvocationID] * vec4(inWorldPosition[i], 1.0);
        outColor = inColor[i];
        outNormal = inNormal[i];
        outTexCoord = inTexCoord[i];
        outShadowCoord = (shadowViewProjectionBias[gl_InvocationID] * vec4(inWorldPosition[i], 1.0)).xyz;
        EmitVertex();
    }

    EndPrimitive();
}

#endif