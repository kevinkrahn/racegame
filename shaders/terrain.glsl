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

#include "lighting.glsl"

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

vec4 trilinear(sampler2D colorSampler, vec3 inWorldPosition, float texScale, vec3 blend)
{
    vec2 uvX = inWorldPosition.zy * texScale;
    vec2 uvY = inWorldPosition.xz * texScale;
    vec2 uvZ = inWorldPosition.xy * texScale;

    vec3 xColor = texture(colorSampler, uvX).rgb;
    vec3 yColor = texture(colorSampler, uvY).rgb;
    vec3 zColor = texture(colorSampler, uvZ).rgb;
    vec4 color = vec4(xColor * blend.x + yColor * blend.y + zColor * blend.z, 1.0);

    return color;
}

vec3 trilinearNormal(sampler2D normalSampler, vec3 inWorldPosition, float texScale, vec3 blend)
{
    vec2 uvX = inWorldPosition.zy * texScale;
    vec2 uvY = inWorldPosition.xz * texScale;
    vec2 uvZ = inWorldPosition.xy * texScale;

#if 0
    // TODO: compare visual difference when normalization is enabled
    vec3 xNormal = normalize(texture(normalSampler, uvX).rgb * 2.0 - 1.0);
    vec3 yNormal = normalize(texture(normalSampler, uvY).rgb * 2.0 - 1.0);
    vec3 zNormal = normalize(texture(normalSampler, uvZ).rgb * 2.0 - 1.0);
#else
    vec3 xNormal = texture(normalSampler, uvX).rgb * 2.0 - 1.0;
    vec3 yNormal = texture(normalSampler, uvY).rgb * 2.0 - 1.0;
    vec3 zNormal = texture(normalSampler, uvZ).rgb * 2.0 - 1.0;
#endif
    xNormal = vec3(xNormal.xy + inNormal.zy, abs(xNormal.z) * inNormal.x);
    yNormal = vec3(yNormal.xy + inNormal.xz, abs(yNormal.z) * inNormal.y);
    zNormal = vec3(zNormal.xy + inNormal.xy, abs(zNormal.z) * inNormal.z);
    vec3 worldNormal = normalize(
            xNormal.zyx * blend.x +
            yNormal.xzy * blend.y +
            zNormal.xyz * blend.z);
    return worldNormal;
}

void main()
{
#if !defined DEPTH_ONLY
    vec3 blend = abs(inNormal);
    blend = max(blend - 0.2, 0.0);
    blend /= dot(blend, vec3(1));
#if 1
    // TODO: find out where the NaNs are coming from
    if (isnan(blend.x)) blend.x = 0.0;
    if (isnan(blend.y)) blend.y = 0.0;
    if (isnan(blend.z)) blend.z = 0.0;
#endif

    vec4 tex1 = inBlend.x > 0.001 ? trilinear(colorSampler1, inWorldPosition, texScale.x, blend) : vec4(0.0);
    vec4 tex2 = inBlend.y > 0.001 ? trilinear(colorSampler2, inWorldPosition, texScale.y, blend) : vec4(0.0);
    vec4 tex3 = inBlend.z > 0.001 ? trilinear(colorSampler3, inWorldPosition, texScale.z, blend) : vec4(0.0);
    vec4 tex4 = inBlend.w > 0.001 ? trilinear(colorSampler4, inWorldPosition, texScale.w, blend) : vec4(0.0);
    vec4 baseColor = inBlend.x*tex1 + inBlend.y*tex2 + inBlend.z*tex3 + inBlend.w*tex4;

#if HIGH_QUALITY_TERRAIN_ENABLED
    vec3 normal1 = inBlend.x > 0.001 ? trilinearNormal(normalSampler1, inWorldPosition, texScale.x, blend) : vec3(0.0);
    vec3 normal2 = inBlend.y > 0.001 ? trilinearNormal(normalSampler2, inWorldPosition, texScale.y, blend) : vec3(0.0);
    vec3 normal3 = inBlend.z > 0.001 ? trilinearNormal(normalSampler3, inWorldPosition, texScale.z, blend) : vec3(0.0);
    vec3 normal4 = inBlend.w > 0.001 ? trilinearNormal(normalSampler4, inWorldPosition, texScale.w, blend) : vec3(0.0);
    vec3 normal = normalize(inBlend.x*normal1 + inBlend.y*normal2 + inBlend.z*normal3 + inBlend.w*normal4);
#else
    vec3 normal = inNormal;
#endif

    vec3 fresnel = inBlend.x*fresnel1 + inBlend.y*fresnel2 + inBlend.z*fresnel3 + inBlend.w*fresnel4;

    outColor = lighting(baseColor, normal, inShadowCoord, inWorldPosition,
            20.0, 0.01, vec3(1.0), fresnel.x, fresnel.y, fresnel.z, vec3(0, 0, 0), 0.0, 0.0, 0.0);

#if defined BRUSH_ENABLED
    float d = length(inWorldPosition - brushPosition);
    float t = pow(clamp((1.f - (d / brushSettings.x)), 0.f, 1.f), brushSettings.y) * clamp(abs(brushSettings.z / 16.f), 0.2f, 1.f);

    if (brushSettings.z >= 0.0) outColor += vec4(vec3(0.05, 0.2, 1.0) * t, 1.0);
    else outColor += vec4(vec3(0.8, 0.02, 0.02) * t, 1.0);
#endif
#endif
}
#endif
