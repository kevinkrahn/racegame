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

layout(location = 0) uniform vec4 highlightColor;

layout(binding = 0) uniform sampler2D colorSampler;
layout(binding = 1) uniform sampler2D bloomSampler1;
layout(binding = 2) uniform sampler2D bloomSampler2;
layout(binding = 3) uniform sampler2D bloomSampler3;
#if defined OUTLINE_ENABLED
layout(binding = 4) uniform usampler2D stencilSampler;
#endif

void main()
{
    outColor = texture(colorSampler, inTexCoord);
#if defined BLOOM_ENABLED
    vec3 bloom = (texture(bloomSampler1, inTexCoord).rgb
                + texture(bloomSampler2, inTexCoord).rgb
                + texture(bloomSampler3, inTexCoord).rgb) / 3.0;
    outColor += vec4(bloom, 0.0);
#endif
#if defined OUTLINE_ENABLED
    ivec2 coord = ivec2(gl_FragCoord.xy);
    uint id = texelFetch(stencilSampler, coord, 0).r;
    float amount = 0.0;
    for (int x = -1; x<=1; ++x)
    {
        for (int y = -1; y<=1; ++y)
        {
            uint adjacentID = texelFetch(stencilSampler, coord + ivec2(x, y), 0).r;
            if (adjacentID == 1 && id == 0 || adjacentID == 0 && id == 1)
            {
                amount += 1.0;
            }
        }
    }
    outColor = mix(outColor, highlightColor, amount / 6.0);
#endif
}
#endif
