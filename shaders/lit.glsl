#include "worldinfo.glsl"

#if defined VERT

layout(location = 0) in vec3 attrPosition;
layout(location = 1) in vec3 attrNormal;
layout(location = 2) in vec4 attrTangent;
layout(location = 3) in vec2 attrTexCoord;
layout(location = 4) in vec3 attrColor;

layout(location = 0) uniform mat4 worldMatrix;
layout(location = 1) uniform mat3 normalMatrix;
layout(location = 8) uniform float windAmount;

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec2 outTexCoord;
layout(location = 2) out vec3 outWorldPosition;
layout(location = 3) out vec3 outShadowCoord;
layout(location = 4) out vec3 outColor;
layout(location = 5) out vec3 outLocalPosition;
#if defined NORMAL_MAP
layout(location = 6) out mat3 outTBN;
#endif

void main()
{
    outWorldPosition = (worldMatrix * vec4(attrPosition, 1.0)).xyz;
    outWorldPosition.x += sin(outWorldPosition.x + outWorldPosition.y * 0.5f + outWorldPosition.z * 0.2f + time * 0.8f) * windAmount * attrTexCoord.y;
    //outWorldPosition.x += sin(outWorldPosition.x * 0.5f + outWorldPosition.y * 0.1f + outWorldPosition.z * 0.25f + time * 0.2f) * 0.15f * windAmount * attrTexCoord.y;
    gl_Position = cameraViewProjection * vec4(outWorldPosition, 1.0);

#if defined ALPHA_DISCARD || !defined DEPTH_ONLY
    outTexCoord = attrTexCoord;
#endif

#if !defined DEPTH_ONLY
    outNormal = normalize(normalMatrix * attrNormal);
    outShadowCoord = (shadowViewProjectionBias * vec4(outWorldPosition, 1.0)).xyz;
    outColor = attrColor;
    outLocalPosition = attrPosition;
#if defined NORMAL_MAP
    vec3 normalWorldSpace    = normalize(normalMatrix * attrNormal);
    vec3 tangentWorldSpace   = normalize(normalMatrix * attrTangent.xyz);
    vec3 biTangentWorldSpace = cross(normalWorldSpace, tangentWorldSpace) * attrTangent.w;
    outTBN = mat3(tangentWorldSpace, biTangentWorldSpace, normalWorldSpace);
#endif
#endif
}

#elif defined FRAG

#include "lighting.glsl"

#if defined OUT_ID
layout(location = 0) out uint outID;
layout(location = 9) uniform uint highlightID;
#else
layout(location = 0) out vec4 outColor;
#endif

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inWorldPosition;
layout(location = 3) in vec3 inShadowCoord;
layout(location = 4) in vec3 inColor;
layout(location = 5) in vec3 inLocalPosition;
#if defined NORMAL_MAP
layout(location = 6) in mat3 inTBN;
#endif

layout(location = 2) uniform vec3 color;
layout(location = 3) uniform vec3 fresnel; // x: bias, y: scale, z: power
layout(location = 4) uniform vec3 specular; // x: power, y: strength
layout(location = 5) uniform float minAlpha;
layout(location = 6) uniform vec3 emit;
layout(location = 7) uniform vec3 reflection;
layout(location = 10) uniform vec3 shieldColor;

layout(binding = 0) uniform sampler2D texSampler;
#if defined NORMAL_MAP
layout(binding = 5) uniform sampler2D normalSampler;
#endif

void main()
{
#if defined ALPHA_DISCARD || !defined DEPTH_ONLY
    vec4 tex = texture(texSampler, inTexCoord);
#endif

#if defined ALPHA_DISCARD
    if (tex.a < minAlpha) { discard; }
#endif

#if !defined DEPTH_ONLY
    vec3 normal = inNormal;

#if defined NORMAL_MAP
    const float normalMapStrength = 1.8;
    normal = texture2D(normalSampler, inTexCoord).rgb * 2.0 - 1.0;
    normal = normal * vec3(1.0, 1.0, normalMapStrength);
    normal = inTBN * normal;
#endif
#if defined OUT_ID
    outID = highlightID;
#else
    outColor = lighting(tex * vec4(inColor * color, 1.0),
            normalize(normal), inShadowCoord, inWorldPosition, specular.x, specular.y, vec3(1.0),
            fresnel.x, fresnel.y, fresnel.z, emit, reflection.x, reflection.y, reflection.z);
#if defined VEHICLE
    outColor.rgb += shieldColor *
        pow((sin(time * 4.f + inLocalPosition.x * 4.f + inLocalPosition.z * 4.f) + 1.f) * 0.5f, 2.f);
#endif
#endif
#endif
}

#endif
