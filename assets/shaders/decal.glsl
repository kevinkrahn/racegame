#include "worldinfo.glsl"

#if defined VERT

layout(location = 0) uniform mat4 worldMatrix;

layout(location = 0) out vec3 outWorldPosition;

const vec3 vertices[36] = vec3[](
   vec3(-1.0,-1.0,-1.0),
   vec3(-1.0,-1.0, 1.0),
   vec3(-1.0, 1.0, 1.0),
   vec3(1.0, 1.0,-1.0),
   vec3(-1.0,-1.0,-1.0),
   vec3(-1.0, 1.0,-1.0),
   vec3(1.0,-1.0, 1.0),
   vec3(-1.0,-1.0,-1.0),
   vec3(1.0,-1.0,-1.0),
   vec3(1.0, 1.0,-1.0),
   vec3(1.0,-1.0,-1.0),
   vec3(-1.0,-1.0,-1.0),
   vec3(-1.0,-1.0,-1.0),
   vec3(-1.0, 1.0, 1.0),
   vec3(-1.0, 1.0,-1.0),
   vec3(1.0,-1.0, 1.0),
   vec3(-1.0,-1.0, 1.0),
   vec3(-1.0,-1.0,-1.0),
   vec3(-1.0, 1.0, 1.0),
   vec3(-1.0,-1.0, 1.0),
   vec3(1.0,-1.0, 1.0),
   vec3(1.0, 1.0, 1.0),
   vec3(1.0,-1.0,-1.0),
   vec3(1.0, 1.0,-1.0),
   vec3(1.0,-1.0,-1.0),
   vec3(1.0, 1.0, 1.0),
   vec3(1.0,-1.0, 1.0),
   vec3(1.0, 1.0, 1.0),
   vec3(1.0, 1.0,-1.0),
   vec3(-1.0, 1.0,-1.0),
   vec3(1.0, 1.0, 1.0),
   vec3(-1.0, 1.0,-1.0),
   vec3(-1.0, 1.0, 1.0),
   vec3(1.0, 1.0, 1.0),
   vec3(-1.0, 1.0, 1.0),
   vec3(1.0,-1.0, 1.0)
);

void main()
{
    outWorldPosition = (worldMatrix * vec4(vertices[gl_VertexID], 1.0)).xyz;
}

#elif defined FRAG

#include "lighting.glsl"

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec3 inWorldPosition;

layout(location = 1) uniform mat4 inverseWorldMatrix;

layout(binding = 0) uniform sampler2D tex;
layout(binding = 1) uniform sampler2DArray depthTexture;

void main()
{
    float depth = texelFetch(depthTexture, ivec3(gl_FragCoord.xy, gl_Layer), 0).r * 2.0 - 1.0;
    vec4 clipSpacePosition = vec4(gl_FragCoord.xy / textureSize(depthTexture, 0).xy * 2.0 - 1.0, depth, 1.0);
    vec4 viewSpacePosition = inverse(cameraProjection[gl_Layer]) * clipSpacePosition;
    viewSpacePosition /= viewSpacePosition.w;
    vec3 worldSpacePosition = (inverse(cameraView[gl_Layer]) * viewSpacePosition).xyz;

    vec3 localPos = abs((inverseWorldMatrix * vec4(worldSpacePosition, 1.0)).xyz);
    if (localPos.x > 1.0 || localPos.y > 1.0 || localPos.z > 1.0)
    {
        discard;
    }
    vec2 uv = localPos.xy * 0.5 + 0.5;

    vec3 normal = normalize(-cross(dFdy(worldSpacePosition), dFdx(worldSpacePosition)));
    vec3 shadowCoord = (shadowViewProjectionBias[gl_Layer] * vec4(worldSpacePosition, 1.0)).xyz;

    outColor = lighting(texture(tex, uv), normalize(normal), shadowCoord, worldSpacePosition);
}

#elif defined GEOM

layout(triangles, invocations = VIEWPORT_COUNT) in;
layout(triangle_strip, max_vertices = 3) out;

layout(location = 0) in vec3 inWorldPosition[];

layout(location = 0) out vec3 outWorldPosition;

void main()
{
    for (uint i=0; i<3; ++i)
    {
        gl_Layer = gl_InvocationID;
        gl_Position = cameraViewProjection[gl_InvocationID] * vec4(inWorldPosition[i], 1.0);
        outWorldPosition = inWorldPosition[i];
        EmitVertex();
    }

    EndPrimitive();
}

#endif
