#if defined VERT

void main()
{
    gl_Position = vec4(-1.0 + float((gl_VertexID & 1) << 2), -1.0 + float((gl_VertexID & 2) << 1), 0.0, 1.0);
}

#elif defined FRAG

layout(location = 0) out float result;

layout(location = 0) uniform vec4 clipInfo[VIEWPORT_COUNT];

layout(binding = 1) uniform sampler2DArray depthTexture;

void main()
{
    float d = texelFetch(depthTexture, ivec3(ivec2(gl_FragCoord.xy), gl_Layer), 0).r;
    result = clipInfo[gl_Layer][0] / (clipInfo[gl_Layer][1] * d + clipInfo[gl_Layer][2]);
}

#elif defined GEOM

layout(triangles, invocations = VIEWPORT_COUNT) in;
layout(triangle_strip, max_vertices = 3) out;

void main()
{
    for (uint i=0; i<3; ++i)
    {
        gl_Layer = gl_InvocationID;
        gl_Position = gl_in[i].gl_Position;
        EmitVertex();
    }

    EndPrimitive();
}

#endif
