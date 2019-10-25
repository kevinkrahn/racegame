#if defined VERT

layout (location = 0) in vec3 attrPosition;
layout (location = 1) in vec4 attrColor;

layout (location = 1) uniform mat4 worldViewProjectionMatrix;
layout (location = 2) uniform vec4 color;
layout (location = 3) uniform int overwriteColor;

layout(location = 0) out vec4 outColor;

void main()
{
    gl_Position = worldViewProjectionMatrix * vec4(attrPosition, 1.0);
    outColor = overwriteColor > 0 ? color : attrColor;
}

#elif defined FRAG

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec4 inColor;

void main()
{
    outColor = inColor;
}

#endif
