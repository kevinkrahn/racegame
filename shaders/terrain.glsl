#include "worldinfo.glsl"

#if defined VERT

layout(location = 0) in vec3 attrPosition;
layout(location = 1) in vec3 attrNormal;
layout(location = 2) in vec4 attrBlend;

layout(location = 0) out vec3 outWorldPosition;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outShadowCoord;
layout(location = 3) out vec4 outBlend;

void main()
{
    gl_Position = cameraViewProjection * vec4(attrPosition, 1.0);
#if !defined DEPTH_ONLY
    outNormal = attrNormal;
    outWorldPosition = attrPosition;
    outShadowCoord = (shadowViewProjectionBias * vec4(attrPosition, 1.0)).xyz;
    outBlend = attrBlend;
#endif
}

#elif defined FRAG

#define SSAO_LUMINANCE_BIAS 0.35
#define NO_REFLECTIONS 1
#include "lighting.glsl"
#include "trilinear.glsl"

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec3 inWorldPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inShadowCoord;
layout(location = 3) in vec4 inBlend;

layout(location = 3) uniform vec3 brushSettings;
layout(location = 4) uniform vec3 brushPosition;
layout(location = 5) uniform vec4 texScale;
layout(location = 6) uniform vec3 fresnel1;
layout(location = 7) uniform vec3 fresnel2;
layout(location = 8) uniform vec3 fresnel3;
layout(location = 9) uniform vec3 fresnel4;

layout(binding = 6)  uniform sampler2D colorSampler1;
layout(binding = 7)  uniform sampler2D colorSampler2;
layout(binding = 8)  uniform sampler2D colorSampler3;
layout(binding = 9)  uniform sampler2D colorSampler4;

layout(binding = 10) uniform sampler2D normalSampler1;
layout(binding = 11) uniform sampler2D normalSampler2;
layout(binding = 12) uniform sampler2D normalSampler3;
layout(binding = 13) uniform sampler2D normalSampler4;

vec3 linear(sampler2D colorSampler, vec3 inWorldPosition)
{
    return texture(colorSampler, inWorldPosition.xy).rgb;
}

void main()
{
#if !defined DEPTH_ONLY
    vec3 blend = trilinearBlend(inNormal);
#if 1
    // TODO: find out where the NaNs are coming from
    if (isnan(blend.x)) blend.x = 0.0;
    if (isnan(blend.y)) blend.y = 0.0;
    if (isnan(blend.z)) blend.z = 0.0;
#endif

    const float THRESHOLD = 0.002;

#if HIGH_QUALITY_TERRAIN_ENABLED
    vec3 tex1 = inBlend.x > THRESHOLD ?
        trilinearRgb(colorSampler1, inWorldPosition * texScale.x, blend) : vec3(0.0);
    vec3 tex2 = inBlend.y > THRESHOLD ?
        trilinearRgb(colorSampler2, inWorldPosition * texScale.y, blend) : vec3(0.0);
    vec3 tex3 = inBlend.z > THRESHOLD ?
        trilinearRgb(colorSampler3, inWorldPosition * texScale.z, blend) : vec3(0.0);
    vec3 tex4 = inBlend.w > THRESHOLD ?
        trilinearRgb(colorSampler4, inWorldPosition * texScale.w, blend) : vec3(0.0);
    vec4 baseColor = vec4(inBlend.x*tex1 + inBlend.y*tex2 + inBlend.z*tex3 + inBlend.w*tex4, 1.0);

    vec3 normal1 = inBlend.x > THRESHOLD ?
        trilinearNormal(normalSampler1, inNormal, inWorldPosition * texScale.x, blend) : vec3(0.0);
    vec3 normal2 = inBlend.y > THRESHOLD ?
        trilinearNormal(normalSampler2, inNormal, inWorldPosition * texScale.y, blend) : vec3(0.0);
    vec3 normal3 = inBlend.z > THRESHOLD ?
        trilinearNormal(normalSampler3, inNormal, inWorldPosition * texScale.z, blend) : vec3(0.0);
    vec3 normal4 = inBlend.w > THRESHOLD ?
        trilinearNormal(normalSampler4, inNormal, inWorldPosition * texScale.w, blend) : vec3(0.0);
    vec3 normal = normalize(inBlend.x*normal1 + inBlend.y*normal2 + inBlend.z*normal3 + inBlend.w*normal4);

    vec3 fresnel = inBlend.x*fresnel1 + inBlend.y*fresnel2 + inBlend.z*fresnel3 + inBlend.w*fresnel4;

    outColor = lighting(baseColor, normal, inShadowCoord, inWorldPosition,
            20.0, 0.01, vec3(1.0), fresnel.x, fresnel.y, fresnel.z, vec3(0, 0, 0), 0.0, 0.0, 0.0);
#else
    vec3 tex1 = inBlend.x > THRESHOLD ?
        trilinearRgb(colorSampler1, inWorldPosition * texScale.x, blend) : vec3(0.0);
    vec3 tex2 = inBlend.y > THRESHOLD ?
        trilinearRgb(colorSampler2, inWorldPosition * texScale.y, blend) : vec3(0.0);
    vec3 tex3 = inBlend.z > THRESHOLD ?
        linear(colorSampler3, inWorldPosition * texScale.z) : vec3(0.0);
    vec4 baseColor = vec4(inBlend.x*tex1 + inBlend.y*tex2 + inBlend.z*tex3 + inBlend.w*tex1, 1.0);
    outColor = lighting(baseColor, inNormal, inShadowCoord, inWorldPosition,
            20.0, 0.01, vec3(1.0), 0.0, 0.0, 0.0, vec3(0, 0, 0), 0.0, 0.0, 0.0);
#endif

#if defined BRUSH_ENABLED
    float d = length(inWorldPosition - brushPosition);
    float t = pow(clamp((1.f - (d / brushSettings.x)), 0.f, 1.f), brushSettings.y) * clamp(abs(brushSettings.z / 16.f), 0.2f, 1.f);

    if (brushSettings.z >= 0.0) outColor += vec4(vec3(0.05, 0.2, 1.0) * t, 1.0);
    else outColor += vec4(vec3(0.8, 0.02, 0.02) * t, 1.0);
#endif
#endif
}
#endif
