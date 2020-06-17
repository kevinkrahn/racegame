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
#if defined VEHICLE
layout(location = 5) out vec3 outLocalPosition;
#endif
#if defined NORMAL_MAP
layout(location = 6) out mat3 outTBN;
#endif

void main()
{
    outWorldPosition = (worldMatrix * vec4(attrPosition, 1.0)).xyz;
#if !defined VEHICLE
    outWorldPosition.x += sin(outWorldPosition.x + outWorldPosition.y * 0.5f + outWorldPosition.z * 0.2f + time * 0.8f) * windAmount * attrTexCoord.y;
#endif
    //outWorldPosition.x += sin(outWorldPosition.x * 0.5f + outWorldPosition.y * 0.1f + outWorldPosition.z * 0.25f + time * 0.2f) * 0.15f * windAmount * attrTexCoord.y;
    gl_Position = cameraViewProjection * vec4(outWorldPosition, 1.0);

#if defined ALPHA_DISCARD || !defined DEPTH_ONLY
    outTexCoord = attrTexCoord;
#endif

#if !defined DEPTH_ONLY
    outNormal = normalize(normalMatrix * attrNormal);
    outShadowCoord = (shadowViewProjectionBias * vec4(outWorldPosition, 1.0)).xyz;
    outColor = attrColor;
#if defined VEHICLE
    outLocalPosition = attrPosition;
#endif
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
#if defined VEHICLE
layout(location = 5) in vec3 inLocalPosition;
#endif
#if defined NORMAL_MAP
layout(location = 6) in mat3 inTBN;
#endif

layout(location = 2) uniform vec3 color;
layout(location = 3) uniform vec3 fresnel; // x: bias, y: scale, z: power
layout(location = 4) uniform vec3 specular; // x: power, y: strength
layout(location = 5) uniform float minAlpha;
layout(location = 6) uniform vec3 emit;
layout(location = 7) uniform vec3 reflection;

#if defined VEHICLE
layout(location = 10) uniform vec4 shield;
layout(location = 11) uniform vec4 wrapColor[3];
layout(binding = 6) uniform sampler2D wrapSampler1;
layout(binding = 7) uniform sampler2D wrapSampler2;
layout(binding = 8) uniform sampler2D wrapSampler3;
#endif

layout(binding = 0) uniform sampler2D texSampler;
#if defined NORMAL_MAP
layout(binding = 5) uniform sampler2D normalSampler;
#endif

void main()
{
#if !defined VEHICLE
#if defined ALPHA_DISCARD || !defined DEPTH_ONLY
    vec4 tex = texture(texSampler, inTexCoord);
#endif

#if defined ALPHA_DISCARD
    if (tex.a < minAlpha) { discard; }
#endif
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
#if defined VEHICLE
    /*
    vec3 blend = abs(normalize(inNormal));
    blend = max(blend - 0.2, 0.0);
    blend /= dot(blend, vec3(1));
    vec3 texOffset = paintPatternParams.xyz;
    float texScale = paintPatternParams.w;
    vec3 localPos = (inLocalPosition + texOffset) * texScale;
    vec2 uvX = localPos.zy;
    vec2 uvY = localPos.xz;
    vec2 uvZ = localPos.xy;
    vec4 xColor = texture(paintPatternSampler, uvX);
    vec4 yColor = texture(paintPatternSampler, uvY);
    vec4 zColor = texture(paintPatternSampler, uvZ);
    vec4 patternColor =
        vec4(xColor * blend.x + yColor * blend.y + zColor * blend.z) * paintPatternColor;
    vec4 baseColor = vec4(mix(color, patternColor.rgb, patternColor.a), 1.0);
    */

    /*
    float vehicleLength = 4.5;
    float vehicleWidth = 0.98;
    float vehicleHeight = 1.02;
    float vehicleTopZ = 0.72;

    float tX = inLocalPosition.x / vehicleLength;
    float tY = inLocalPosition.y / vehicleWidth;
    float tZ = -(inLocalPosition.z - vehicleTopZ) / vehicleHeight;

    float texU = 0.5 + tX;
    float texV = 0.5 + tY + sign(tY) * tZ * abs(tY) * 2.0;

    float zz = inLocalPosition.z - 0.42;
    vec4 patternColor = texture(paintPatternSampler, vec2(texU, texV)) * paintPatternColor;
    vec4 baseColor = vec4(mix(color, patternColor.rgb, patternColor.a), 1.0);
    */

    vec4 wrap1 = texture(wrapSampler1, inTexCoord) * wrapColor[0];
    vec4 wrap2 = texture(wrapSampler2, inTexCoord) * wrapColor[1];
    vec4 wrap3 = texture(wrapSampler3, inTexCoord) * wrapColor[2];
    vec4 baseColor =
        vec4(mix(mix(mix(color, wrap1.rgb, wrap1.a), wrap2.rgb, wrap2.a), wrap3.rgb, wrap3.a), 1.0);
#else
    vec4 baseColor = tex * vec4(inColor * color, 1.0);
#endif
    outColor = lighting(baseColor,
            normalize(normal), inShadowCoord, inWorldPosition, specular.x, specular.y, vec3(1.0),
            fresnel.x, fresnel.y, fresnel.z, emit, reflection.x, reflection.y, reflection.z);
#if defined VEHICLE
    float shieldAmount =
        pow((sin(time * 4.f + inLocalPosition.x * 4.f + inLocalPosition.z * 4.f) + 1.f) * 0.5f, 2.f);
    outColor.rgb = mix(outColor.rgb, shield.rgb, shield.a * shieldAmount * 0.5f);
    outColor.rgb += shield.rgb * shield.a * shieldAmount;
#endif
#endif
#endif
}

#endif
