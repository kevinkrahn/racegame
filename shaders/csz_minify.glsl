#if defined VERT

void main()
{
    gl_Position = vec4(-1.0 + float((gl_VertexID & 1) << 2), -1.0 + float((gl_VertexID & 2) << 1), 0.0, 1.0);
}

#elif defined FRAG

layout(location = 0) out float result;

layout(location = 0) uniform int previousMIP;

layout(binding = 3) uniform sampler2DArray tex;

void main()
{
    ivec2 ssP = ivec2(gl_FragCoord.xy);
    result = texelFetch(tex, ivec3(clamp(ssP * 2 + ivec2(ssP.y & 1, ssP.x & 1), ivec2(0),
                textureSize(tex, previousMIP).xy - ivec2(1)), gl_Layer), previousMIP).r;
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
