#include "worldinfo.glsl"

#if defined VERT

layout(location = 0) in vec3 attrPosition;
layout(location = 1) in vec3 attrNormal;
layout(location = 2) in vec4 attrBlend;

layout(location = 0) out vec3 outWorldPosition;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec4 outBlend;

void main()
{
    outWorldPosition = attrPosition;
    outNormal = attrNormal;
    outBlend = attrBlend;
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

layout(binding = 6) uniform sampler2D texSampler1;
layout(binding = 7) uniform sampler2D texSampler2;
layout(binding = 8) uniform sampler2D texSampler3;
layout(binding = 9) uniform sampler2D texSampler4;

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

    // TODO: make this better
    if (isnan(blending.x))
    {
        blending.x = 0.00001;
    }
    if (isnan(blending.y))
    {
        blending.y = 0.00001;
    }
    if (isnan(blending.z))
    {
        blending.z = 0.00001;
    }

    vec3 xColor = texture(texSampler2, inWorldPosition.yz * texScale).rgb;
    vec3 yColor = texture(texSampler2, inWorldPosition.xz * texScale).rgb;
    vec3 zColor = texture(texSampler1, inWorldPosition.xy * texScale).rgb;
    vec4 tex1 = vec4(xColor * blending.x + yColor * blending.y + zColor * blending.z, 1.0);

    zColor = texture(texSampler2, inWorldPosition.xy * texScale).rgb;
    vec4 tex2 = vec4(xColor * blending.x + yColor * blending.y + zColor * blending.z, 1.0);

    xColor = texture(texSampler3, inWorldPosition.yz * texScale).rgb;
    yColor = texture(texSampler3, inWorldPosition.xz * texScale).rgb;
    zColor = texture(texSampler3, inWorldPosition.xy * texScale).rgb;
    vec4 tex3 = vec4(xColor * blending.x + yColor * blending.y + zColor * blending.z, 1.0);

    xColor = texture(texSampler4, inWorldPosition.yz * texScale).rgb;
    yColor = texture(texSampler4, inWorldPosition.xz * texScale).rgb;
    zColor = texture(texSampler4, inWorldPosition.xy * texScale).rgb;
    vec4 tex4 = vec4(xColor * blending.x + yColor * blending.y + zColor * blending.z, 1.0);

    vec4 baseColor = inBlend.x*tex1 + inBlend.y*tex2 + inBlend.z*tex3 + inBlend.w*tex4;

    outColor = lighting(baseColor, normalize(inNormal), inShadowCoord, inWorldPosition,
            20.0, 0.01, vec3(1.0), -0.1, 0.08, 2.5, vec3(0, 0, 0));

    float d = length(inWorldPosition - brushPosition);
    float t = pow(clamp((1.f - (d / brushSettings.x)), 0.f, 1.f), brushSettings.y) * clamp(abs(brushSettings.z / 16.f), 0.2f, 1.f);

    if (brushSettings.z >= 0.0) outColor += vec4(vec3(0.05, 0.2, 1.0) * t, 1.0);
    else outColor += vec4(vec3(0.8, 0.02, 0.02) * t, 1.0);
}

#elif defined GEOM

layout(triangles, invocations = VIEWPORT_COUNT) in;
layout(triangle_strip, max_vertices = 3) out;

layout(location = 0) in vec3 inWorldPosition[];
layout(location = 1) in vec3 inNormal[];
layout(location = 2) in vec4 inBlend[];

layout(location = 0) out vec3 outWorldPosition;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outShadowCoord;
layout(location = 3) out vec4 outBlend;

void main()
{
    for (uint i=0; i<3; ++i)
    {
        gl_Layer = gl_InvocationID;
        gl_Position = cameraViewProjection[gl_InvocationID] * vec4(inWorldPosition[i], 1.0);
        outNormal = inNormal[i];
        outWorldPosition = inWorldPosition[i];
        outShadowCoord = (shadowViewProjectionBias[gl_InvocationID] * vec4(inWorldPosition[i], 1.0)).xyz;
        outBlend = inBlend[i];
        EmitVertex();
    }

    EndPrimitive();
}

#endif
