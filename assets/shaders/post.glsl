#if defined VERT

layout(location = 0) out vec2 outTexCoord;

layout(location = 0) uniform mat4 matrix;

vec2 vertices[6] = vec2[](
    vec2(1, 1),
    vec2(1, 0),
    vec2(0, 0),

    vec2(0, 0),
    vec2(0, 1),
    vec2(1, 1)
);

vec2 uvs[6] = vec2[](
    vec2(1, 1),
    vec2(1, 0),
    vec2(0, 0),

    vec2(0, 0),
    vec2(0, 1),
    vec2(1, 1)
);

void main()
{
    gl_Position = matrix * vec4(vertices[gl_VertexID], 0.0, 1.0);
    outTexCoord = uvs[gl_VertexID];
}

#elif defined FRAG

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 inTexCoord;

layout(binding = 0) uniform sampler2D tex;

void main()
{
    //outColor.rgb = outColor.rgb / (outColor.rgb + vec3(1.0));
    outColor = texture(tex, inTexCoord);
}
#endif
