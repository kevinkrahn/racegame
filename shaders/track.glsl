#include "worldinfo.glsl"

#if defined VERT

layout(location = 0) in vec3 attrPosition;
layout(location = 1) in vec3 attrNormal;

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec3 outWorldPosition;
layout(location = 2) out vec3 outShadowCoord;

void main()
{
    outWorldPosition = attrPosition;
    gl_Position = cameraViewProjection * vec4(outWorldPosition, 1.0);
#if !defined DEPTH_ONLY
    outNormal = attrNormal;
    outShadowCoord = (shadowViewProjectionBias * vec4(outWorldPosition, 1.0)).xyz;
#endif
}

#elif defined FRAG

#include "lighting.glsl"
#include "trilinear.glsl"

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec3 inWorldPosition;
layout(location = 2) in vec3 inShadowCoord;

layout(binding = 0) uniform sampler2D colorSampler;
layout(binding = 5) uniform sampler2D normalSampler;
layout(binding = 6) uniform sampler2D specSampler;

void main()
{
#if !defined DEPTH_ONLY
    const float texScale = 0.1;
    vec3 p = inWorldPosition * texScale;
    vec3 blend = trilinearBlend(inNormal);
    vec4 color = vec4(trilinearRgb(colorSampler, p, blend), 1.0);
    float spec = trilinearR(specSampler, p, blend);
    vec3 worldNormal = trilinearNormal(normalSampler, inNormal, p, blend);

    outColor = lighting(color, worldNormal, inShadowCoord, inWorldPosition,
            100.0, spec * 2.0, vec3(1.0), 0.0, 0.0, 0.0, vec3(0, 0, 0), 0.0, 0.0, 0.0);
#endif
}
#endif
