#if defined VERT

layout(location = 0) in vec3 attrPosition;
layout(location = 4) in vec3 attrColor;

layout(location = 0) out vec3 outColor;

layout(location = 0) uniform mat4 transform;

void main()
{
    gl_Position = transform * vec4(attrPosition, 1.0);
    outColor = attrColor;
}

#elif defined FRAG

layout(location = 0) in vec3 inColor;

layout(location = 0) out vec4 outColor;

layout(location = 1) uniform vec3 color;
layout(location = 2) uniform int overwriteColor;

void main()
{
    //outColor = vec4(vec3(1.0 - gl_FragCoord.z), 1.0);
    outColor = overwriteColor > 0 ? vec4(color, 1.0) : vec4(color * inColor, 1.0);
}

#endif
