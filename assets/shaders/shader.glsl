#if defined VERT
layout (location = 0) in vec3 attrPosition;
layout (location = 1) in vec3 attrNormal;
layout (location = 2) in vec3 attrColor;
layout (location = 3) in vec2 attrTexCoord;

layout (location = 0) uniform mat4 worldViewProjectionMatrix;

void main()
{
    gl_Position = worldViewProjectionMatrix * vec4(attrPosition, 1.0);
}

#elif defined FRAG

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(1.0, 0.0, 0.0, 1.0);
}

#elif defined GEOM

#endif
