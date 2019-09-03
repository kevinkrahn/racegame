#include "worldinfo.glsl"

#if defined VERT

layout(location = 0) in vec3 attrPosition;
layout(location = 1) in vec3 attrNormal;
layout(location = 2) in vec3 attrColor;

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outWorldPosition;

void main()
{
    outColor = attrColor;
    outNormal = attrNormal;
    outWorldPosition = attrPosition;
}

#elif defined FRAG

#include "lighting.glsl"

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec3 inColor;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inWorldPosition;
layout(location = 3) in vec3 inShadowCoord;

layout(location = 2) uniform vec3 color;
layout(location = 3) uniform vec3 brushSettings;
layout(location = 4) uniform vec3 brushPosition;

layout(binding = 0) uniform sampler2D texSampler;
layout(binding = 1) uniform sampler2D texSampler2;

void main()
{
    const float texScale = 0.1;

    vec3 blending = abs(inNormal);
    blending.z *= (blending.z + 0.5);
    blending.z *= (blending.z + 0.5);
    blending.z *= (blending.z + 0.5);
    blending.z *= (blending.z + 0.5);
    blending = normalize(max(blending, 0.00001));
    blending /= vec3(blending.x + blending.y + blending.z);

    vec3 xColor = texture(texSampler2, inWorldPosition.yz * texScale).rgb;
    vec3 yColor = texture(texSampler2, inWorldPosition.xz * texScale).rgb;
    vec3 zColor = texture(texSampler, inWorldPosition.xy * texScale).rgb;

    vec4 tex = vec4(xColor * blending.x + yColor * blending.y + zColor * blending.z, 1.0);

    vec4 baseColor = tex * vec4(inColor, 1.0);
    outColor = lighting(baseColor, normalize(inNormal), inShadowCoord, inWorldPosition,
            20.0, 0.01, vec3(1.0), -0.15, 0.15, 2.2);

    float d = length(inWorldPosition - brushPosition);
    float t = clamp(pow((1.f - (d / brushSettings.x)), brushSettings.y), 0.f, 1.f) * clamp(abs(brushSettings.z / 16.f), 0.2f, 1.f);

    if (brushSettings.z >= 0.0) outColor += vec4(vec3(0.05, 0.2, 1.0) * t, 1.0);
    else outColor += vec4(vec3(0.8, 0.02, 0.02) * t, 1.0);
}

#elif defined GEOM

layout(triangles, invocations = VIEWPORT_COUNT) in;
layout(triangle_strip, max_vertices = 3) out;

layout(location = 0) in vec3 inColor[];
layout(location = 1) in vec3 inNormal[];
layout(location = 2) in vec3 inWorldPosition[];

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outWorldPosition;
layout(location = 3) out vec3 outShadowCoord;

void main()
{
    for (uint i=0; i<3; ++i)
    {
        gl_Layer = gl_InvocationID;
        gl_Position = cameraViewProjection[gl_InvocationID] * vec4(inWorldPosition[i], 1.0);
        outColor = inColor[i];
        outNormal = inNormal[i];
        outWorldPosition = inWorldPosition[i];
        outShadowCoord = (shadowViewProjectionBias[gl_InvocationID] * vec4(inWorldPosition[i], 1.0)).xyz;
        EmitVertex();
    }

    EndPrimitive();
}

#endif
