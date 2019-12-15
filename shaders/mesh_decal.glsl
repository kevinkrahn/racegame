#include "worldinfo.glsl"
#if defined VERT

layout(location = 0) in vec3 attrPosition;
layout(location = 1) in vec3 attrNormal;
layout(location = 2) in vec2 attrTexCoord;

layout(location = 0) uniform mat4 worldMatrix;
layout(location = 1) uniform mat3 normalMatrix;

#define NORMAL_MAPPING 0

#if NORMAL_MAPPING
layout(location = 0) out mat3 outTBN;
#else
layout(location = 0) out vec3 outNormal;
#endif
layout(location = 3) out vec2 outTexCoord;
layout(location = 4) out vec3 outWorldPosition;
layout(location = 5) out vec3 outShadowCoord;

void main()
{
    //vec3 T = normalize(vec3(N.z, 0, N.x));
    //vec3 B = normalize(vec3(0, N.z, N.y));
    //vec3 T = cross(vec3(1.0, 0.0, 0.0), N);
    //vec3 B = cross(T, N);

#if NORMAL_MAPPING
    vec3 N = normalize(normalMatrix * attrNormal);
    vec3 T = normalize(normalMatrix * cross(vec3(0.0, 1.0, 0.0), attrNormal));
    vec3 B = cross(N, T);
    outTBN = mat3(T, B, N);
#else
    outNormal = normalMatrix * attrNormal;
#endif
    outTexCoord = attrTexCoord;
    outWorldPosition = (worldMatrix * vec4(attrPosition, 1.0)).xyz;
    gl_Position = cameraViewProjection[0] * vec4(outWorldPosition, 1.0);
    outShadowCoord = (shadowViewProjectionBias[0] * vec4(outWorldPosition, 1.0)).xyz;
}

#elif defined FRAG

#include "lighting.glsl"

layout(location = 0) out vec4 outColor;

#if NORMAL_MAPPING
layout(location = 0) in mat3 inTBN;
#else
layout(location = 0) in vec3 inNormal;
#endif
layout(location = 3) in vec2 inTexCoord;
layout(location = 4) in vec3 inWorldPosition;
layout(location = 5) in vec3 inShadowCoord;

layout(location = 2) uniform vec3 color;
//layout(location = 3) uniform vec3 reflection;
layout(binding = 0) uniform sampler2D texSampler;
layout(binding = 6) uniform sampler2D normalSampler;

void main()
{
#if NORMAL_MAPPING
    vec3 normal = texture(normalSampler, inTexCoord).rgb;
    normal = normalize(normal * 2.0 - 1.0);
    normal = normalize(inTBN * normal);
#else
    vec3 normal = normalize(inNormal);
#endif
    outColor = lighting(texture(texSampler, inTexCoord) * vec4(color, 1.0),
            normal, inShadowCoord, inWorldPosition, 800.0, 0.5, vec3(1.0),
            -0.1, 0.25, 3.0, vec3(0, 0, 0), 0.0, 0.0, 0.0);
}
#endif
