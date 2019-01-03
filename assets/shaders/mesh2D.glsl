#if defined VERT

layout(location = 0) in vec3 attrPosition;

layout(location = 0) out vec3 outColor;

layout(location = 0) uniform mat4 transform;

void main()
{
    gl_Position = transform * vec4(attrPosition, 1.0);
}

#elif defined FRAG

layout(location = 0) in vec3 inColor;

layout(location = 0) out vec4 outColor;

layout(location = 1) uniform vec2 depthRange;

void main()
{
    outColor = vec4(1.0);
}

#endif
