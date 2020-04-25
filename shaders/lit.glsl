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
layout(location = 4) out vec3 outShadowCoord;

void main()
{
    outWorldPosition = (worldMatrix * vec4(attrPosition, 1.0)).xyz;
    gl_Position = cameraViewProjection * vec4(outWorldPosition, 1.0);
    outColor = attrColor;
    outNormal = normalize(normalMatrix * attrNormal);
    outTexCoord = attrTexCoord;
    outShadowCoord = (shadowViewProjectionBias * vec4(outWorldPosition, 1.0)).xyz;
}

#elif defined FRAG

#include "lighting.glsl"

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec3 inColor;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inWorldPosition;
layout(location = 4) in vec3 inShadowCoord;

layout(location = 2) uniform vec3 color;
layout(location = 3) uniform vec3 fresnel; // x: bias, y: scale, z: power
layout(location = 4) uniform vec3 specular; // x: power, y: strength
layout(location = 5) uniform float minAlpha;
layout(location = 6) uniform vec3 emit;
layout(location = 7) uniform vec3 reflection;

layout(binding = 0) uniform sampler2D texSampler;

void main()
{
    vec4 tex = texture(texSampler, inTexCoord);
#if defined ALPHA_DISCARD
    if (tex.a < minAlpha) { discard; }
#endif
    outColor = lighting(tex * vec4(inColor * color, 1.0),
            normalize(inNormal), inShadowCoord, inWorldPosition, specular.x, specular.y, vec3(1.0),
            fresnel.x, fresnel.y, fresnel.z, emit, reflection.x, reflection.y, reflection.z);
}

#endif
