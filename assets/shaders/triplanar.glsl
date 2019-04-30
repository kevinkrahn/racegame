#include "worldinfo.glsl"

#if defined VERT

layout(location = 0) in vec3 attrPosition;
layout(location = 1) in vec3 attrNormal;
layout(location = 2) in vec3 attrColor;
layout(location = 3) in vec2 attrTexCoord;

layout(location = 0) uniform mat4 worldMatrix;
layout(location = 1) uniform mat3 normalMatrix;

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outTexCoord;
layout(location = 3) out vec3 outWorldPosition;
layout(location = 4) out vec3 outLocalPosition;
layout(location = 5) out vec3 outLocalNormal;

void main()
{
    outColor = attrColor;
    outNormal = normalize(normalMatrix * attrNormal);
    outTexCoord = attrTexCoord;
    outWorldPosition = (worldMatrix * vec4(attrPosition, 1.0)).xyz;
    outLocalPosition = attrPosition;
    outLocalNormal = attrNormal;
}

#elif defined FRAG

#include "lighting.glsl"

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec3 inColor;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inWorldPosition;
layout(location = 4) in vec3 inShadowCoord;
layout(location = 5) in vec3 inLocalPosition;
layout(location = 6) in vec3 inLocalNormal;

layout(location = 2) uniform vec3 color;

layout(binding = 0) uniform sampler2D texSampler;

void main()
{
    const float texScale = 0.25;

    vec3 blending = abs(inLocalNormal);
    blending = normalize(max(blending, 0.00001));
    blending /= vec3(blending.x + blending.y + blending.z);

    vec3 xColor = texture(texSampler, inLocalPosition.yz * texScale).rgb;
    vec3 yColor = texture(texSampler, inLocalPosition.xz * texScale).rgb;
    vec3 zColor = texture(texSampler, inLocalPosition.xy * texScale).rgb;

    vec4 tex = vec4(xColor * blending.x + yColor * blending.y + zColor * blending.z, 1.0);

    vec4 baseColor = tex * vec4(inColor, 1.0);
    outColor = lighting(baseColor, normalize(inNormal), inShadowCoord, inWorldPosition,
            50.0, 1.0, vec3(1.0), -0.1, 0.8, 3.0);
}

#elif defined GEOM

layout(triangles, invocations = VIEWPORT_COUNT) in;
layout(triangle_strip, max_vertices = 3) out;

layout(location = 0) in vec3 inColor[];
layout(location = 1) in vec3 inNormal[];
layout(location = 2) in vec2 inTexCoord[];
layout(location = 3) in vec3 inWorldPosition[];
layout(location = 4) in vec3 inLocalPosition[];
layout(location = 5) in vec3 inLocalNormal[];

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outTexCoord;
layout(location = 3) out vec3 outWorldPosition;
layout(location = 4) out vec3 outShadowCoord;
layout(location = 5) out vec3 outLocalPosition;
layout(location = 6) out vec3 outLocalNormal;

void main()
{
    for (uint i=0; i<3; ++i)
    {
        gl_Layer = gl_InvocationID;
        gl_Position = cameraViewProjection[gl_InvocationID] * vec4(inWorldPosition[i], 1.0);
        outColor = inColor[i];
        outNormal = inNormal[i];
        outTexCoord = inTexCoord[i];
        outWorldPosition = inWorldPosition[i];
        outShadowCoord = (shadowViewProjectionBias[gl_InvocationID] * vec4(inWorldPosition[i], 1.0)).xyz;
        outLocalPosition = inLocalPosition[i];
        outLocalNormal = inLocalNormal[i];
        EmitVertex();
    }

    EndPrimitive();
}

#endif
