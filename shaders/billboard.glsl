#include "worldinfo.glsl"

#if defined VERT

layout(location = 0) out vec2 outTexCoord;
#if defined LIT
layout(location = 1) out vec3 outWorldPos;
layout(location = 2) out vec3 outShadowCoord;
#endif

layout(location = 1) uniform mat4 translation;
layout(location = 2) uniform vec3 scale;
layout(location = 3) uniform mat4 rotation;

const vec2 vertices[6] = vec2[](
    vec2(-1, -1),
    vec2( 1, -1),
    vec2( 1,  1),

    vec2( 1,  1),
    vec2(-1,  1),
    vec2(-1, -1)
);

const vec2 uvs[6] = vec2[](
    vec2(0, 0),
    vec2(1, 0),
    vec2(1, 1),

    vec2(1, 1),
    vec2(0, 1),
    vec2(0, 0)
);

void main()
{
    mat4 modelView = cameraView * translation;
    modelView[0][0] = scale.x;
    modelView[0][1] = 0;
    modelView[0][2] = 0;
    modelView[1][0] = 0;
    modelView[1][1] = scale.y;
    modelView[1][2] = 0;
    modelView[2][0] = 0;
    modelView[2][1] = 0;
    modelView[2][2] = scale.z;
    mat4 matrix = cameraProjection * modelView * rotation;

    gl_Position = matrix * vec4(vertices[gl_VertexID], 0.0, 1.0);
    outTexCoord = uvs[gl_VertexID];
#if defined LIT && SHADOWS_ENABLED
    outWorldPos = (translation * rotation * vec4(vertices[gl_VertexID], 0.0, 1.0)).xyz;
    outShadowCoord = (shadowViewProjectionBias
            * (translation * rotation * vec4(vertices[gl_VertexID], 0.0, 1.0))).xyz;
#endif
}

#elif defined FRAG

#if defined LIT
#define NO_SSAO
#include "lighting.glsl"
#endif

layout(location = 0) in vec2 inTexCoord;
#if defined LIT
layout(location = 1) in vec3 inWorldPosition;
layout(location = 2) in vec3 inShadowCoord;
#endif

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D texSampler;
layout(binding = 1) uniform sampler2D depthSampler;

layout(location = 0) uniform vec4 color;

void main()
{
    outColor = texture(texSampler, inTexCoord) * color;
#if defined LIT && SHADOWS_ENABLED
    // TODO: use lower quality shadow filtering for billboards
    outColor = lighting(outColor,
            vec3(0, 0, 1), inShadowCoord, inWorldPosition, 0.f, 0.f, vec3(1.0),
            0.f, 0.f, 0.f, vec3(0.f), 0.0, 0.0, 0.0);
#endif

    float depth = texture(depthSampler, gl_FragCoord.xy / textureSize(depthSampler, 0)).r;
    float d = depth - gl_FragCoord.z;
    outColor.a *= clamp(d, 0.0, 0.002) * 800.0;
    outColor.a = clamp(outColor.a, 0.0, 1.0);
}
#endif
