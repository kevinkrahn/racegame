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

layout(binding = 0) uniform sampler2DArray colorSampler;
layout(binding = 1) uniform sampler2DArray bloomSampler1;
layout(binding = 2) uniform sampler2DArray bloomSampler2;
layout(binding = 3) uniform sampler2DArray bloomSampler3;

void main()
{
    outColor = texture(colorSampler, vec3(inTexCoord, gl_Layer));
#if defined BLOOM_ENABLED
    vec3 bloom = (texture(bloomSampler1, vec3(inTexCoord, gl_Layer)).rgb
                + texture(bloomSampler2, vec3(inTexCoord, gl_Layer)).rgb
                + texture(bloomSampler3, vec3(inTexCoord, gl_Layer)).rgb) / 3.0;
    outColor += vec4(bloom, 0.0);
#endif
}

#elif defined GEOM

layout(location = 0) in vec2 inTexCoord[];

layout(location = 0) out vec2 outTexCoord;

layout(triangles, invocations = VIEWPORT_COUNT) in;
layout(triangle_strip, max_vertices = 3) out;

void main()
{
    for (uint i=0; i<3; ++i)
    {
        gl_Layer = gl_InvocationID;
        gl_Position = gl_in[i].gl_Position;
        outTexCoord = inTexCoord[i];
        EmitVertex();
    }

    EndPrimitive();
}

#endif
