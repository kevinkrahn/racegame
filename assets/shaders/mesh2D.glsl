#if defined VERT

layout(location = 0) in vec3 attrPosition;

layout(location = 0) uniform mat4 transform;

void main()
{
    gl_Position = transform * vec4(attrPosition, 1.0);
}

#elif defined FRAG

layout(location = 0) out vec4 outColor;

layout(location = 1) uniform vec3 color;

void main()
{
    //outColor = vec4(vec3(1.0 - gl_FragCoord.z), 1.0);
    outColor = vec4(color, 1.0);
}

#endif
