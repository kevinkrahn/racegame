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
    outNormal = attrNormal;
    outShadowCoord = (shadowViewProjectionBias * vec4(outWorldPosition, 1.0)).xyz;
}

#elif defined FRAG

#include "lighting.glsl"

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec3 inWorldPosition;
layout(location = 2) in vec3 inShadowCoord;

layout(binding = 0) uniform sampler2D colorSampler;
layout(binding = 5) uniform sampler2D normalSampler;
layout(binding = 6) uniform sampler2D specSampler;

void main()
{
    const float texScale = 0.1;

#if 1
    vec3 blend = abs(inNormal);
    blend = max(blend - 0.2, 0.0);
#else
    vec3 blend = pow(inNormal, 4.0);
#endif
    blend /= dot(blend, vec3(1));

    vec2 uvX = inWorldPosition.zy * texScale;
    vec2 uvY = inWorldPosition.xz * texScale;
    vec2 uvZ = inWorldPosition.xy * texScale;

    vec3 xColor = texture(colorSampler, uvX).rgb;
    vec3 yColor = texture(colorSampler, uvY).rgb;
    vec3 zColor = texture(colorSampler, uvZ).rgb;
    vec4 color = vec4(
            xColor * blend.x +
            yColor * blend.y +
            zColor * blend.z, 1.0);

    float xSpec = texture(specSampler, uvX).r;
    float ySpec = texture(specSampler, uvY).r;
    float zSpec = texture(specSampler, uvZ).r;
    float spec = xSpec * blend.x + ySpec * blend.y + zSpec * blend.z;

    // TODO: these normalize calls are probably unnecessary
    vec3 xNormal = normalize(texture(normalSampler, uvX).rgb * 2.0 - 1.0);
    vec3 yNormal = normalize(texture(normalSampler, uvY).rgb * 2.0 - 1.0);
    vec3 zNormal = normalize(texture(normalSampler, uvZ).rgb * 2.0 - 1.0);
    xNormal = vec3(xNormal.xy + inNormal.zy, abs(xNormal.z) * inNormal.x);
    yNormal = vec3(yNormal.xy + inNormal.xz, abs(yNormal.z) * inNormal.y);
    zNormal = vec3(zNormal.xy + inNormal.xy, abs(zNormal.z) * inNormal.z);
    vec3 worldNormal = normalize(
            xNormal.zyx * blend.x +
            yNormal.xzy * blend.y +
            zNormal.xyz * blend.z);
    // TODO: bend normal Z to make track lighting variation more pronounced

    outColor = lighting(color, worldNormal, inShadowCoord, inWorldPosition,
            100.0, spec * 1.85, vec3(1.0), 0.0, 0.0, 0.0, vec3(0, 0, 0), 0.0, 0.0, 0.0);
    //outColor *= (inNormal.z * inNormal.z * inNormal.z * inNormal.z);

    //outColor = vec4((inNormal + 1.0) * 0.5, 1.0);
    //outColor = vec4((worldNormal + 1.0) * 0.5, 1.0);
}
#endif
