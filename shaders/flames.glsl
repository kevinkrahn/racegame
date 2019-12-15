#include "worldinfo.glsl"
#if defined VERT

layout(location = 0) in vec3 attrPosition;

layout(location = 0) out vec3 outPosition;

layout(location = 0) uniform mat4 worldMatrix;

void main()
{
    outPosition = attrPosition;
    vec3 worldPosition = (worldMatrix * vec4(attrPosition, 1.0)).xyz;
    gl_Position = cameraViewProjection[0] * vec4(worldPosition, 1.0);
}

#elif defined FRAG

#include "lighting.glsl"

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec3 inPosition;

layout(binding = 0) uniform sampler2D texSampler;

layout(location = 1) uniform float offset;
layout(location = 2) uniform float alpha;

void main()
{
    float a = texture(texSampler,
            vec2(-inPosition.x * 0.5 - time * 3.0 + offset, inPosition.y * 0.5 + offset)).r;
    outColor = vec4(vec3(0.95, 0.47, 0.01) * 4.0, a * pow(1.0 - min(-inPosition.x * 0.5f, 1.0), 2.2) * alpha);
}

#endif
