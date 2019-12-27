#if defined VERT

layout(location = 0) out vec2 outTexCoord;

void main()
{
    gl_Position = vec4(-1.0 + float((gl_VertexID & 1) << 2), -1.0 + float((gl_VertexID & 2) << 1), 0.0, 1.0);
    outTexCoord = vec2((gl_Position.x + 1.0) * 0.5, (gl_Position.y + 1.0) * 0.5);
}

#elif defined FRAG

layout(location = 0) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D colorSampler;
layout(binding = 1) uniform sampler2D bloomSampler1;
layout(binding = 2) uniform sampler2D bloomSampler2;
layout(binding = 3) uniform sampler2D bloomSampler3;

void main()
{
    outColor = texture(colorSampler, inTexCoord);
#if defined BLOOM_ENABLED
    vec3 bloom = (texture(bloomSampler1, inTexCoord).rgb
                + texture(bloomSampler2, inTexCoord).rgb
                + texture(bloomSampler3, inTexCoord).rgb) / 3.0;
    outColor += vec4(bloom, 0.0);
#endif
}
#endif
