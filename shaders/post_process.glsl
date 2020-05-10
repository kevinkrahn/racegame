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
#if defined EDITOR_OUTLINE_ENABLED
layout(binding = 4) uniform usampler2D stencilSampler;
#endif

float sharpenKernel[9] = { 0.0, -1.0, 0.0, -1.0, 5.0, -1.0, 0.0, -1.0, 0.0 };
vec2 sampleOffset[9] = {
    vec2(-1.0, -1.0),
    vec2(0.0, -1.0),
    vec2(1.0, -1.0),
    vec2(-1.0, 0.0),
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(-1.0, 1.0),
    vec2(0.0, 1.0),
    vec2(1.0, 1.0)
};

void main()
{
    outColor = texture(colorSampler, inTexCoord);
#if SHARPEN_ENABLED
    vec3 sum = vec3(0.0);
    vec2 step = 1.0 / vec2(textureSize(colorSampler, 0));
    for (int i=0; i<9; ++i)
    {
        vec3 color = texture(colorSampler, inTexCoord + sampleOffset[i] * step).rgb;
        sum += color * sharpenKernel[i];
    }
    outColor = mix(outColor, vec4(sum, 1.0), 0.25);
#endif

#if BLOOM_ENABLED
    vec3 bloom = (texture(bloomSampler1, inTexCoord).rgb
                + texture(bloomSampler2, inTexCoord).rgb
                + texture(bloomSampler3, inTexCoord).rgb) / 3.0;
    outColor += vec4(bloom, 0.0);
#endif
#if defined EDITOR_OUTLINE_ENABLED
    ivec2 coord = ivec2(gl_FragCoord.xy);
    uint id = texelFetch(stencilSampler, coord, 0).r;
    uint mask = (~1);
    float amount = 0.0;
    for (int x = -1; x<=1; ++x)
    {
        for (int y = -1; y<=1; ++y)
        {
            uint adjacentID = texelFetch(stencilSampler, coord + ivec2(x, y), 0).r;
            if ((adjacentID & mask) != (id & mask))
            {
                if ((adjacentID & 1) == 1 && (id & 1) == 0 || (id & 1) == 1 && (adjacentID & 1) == 0)
                {
                    amount += 0.2;
                }
                else if ((adjacentID & 1) == 0 && (id & 1) == 0)
                {
                    amount += 1.0;
                }
            }
        }
    }
    if ((id & 1) == 1)
    {
        coord /= 2;
        amount = max(amount, ((coord.x & 1) == 0 || (coord.y & 1) == 1) ? 0.2 : 0.0);
    }
    outColor = mix(outColor, highlightColor, amount / 6.0);
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
    if (id == 1)
    {
        amount = max(amount, 0.5);
    }
    outColor = mix(outColor, highlightColor, amount / 6.0);
#endif
}
#endif
