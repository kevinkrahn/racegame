#if defined VERT

void main()
{
    gl_Position = vec4(-1.0 + float((gl_VertexID & 1) << 2), -1.0 + float((gl_VertexID & 2) << 1), 0.0, 1.0);
}

#elif defined FRAG

layout(location = 0) out float result;

layout(location = 1) uniform vec4 clipInfo;

layout(binding = 1) uniform sampler2D depthTexture;

void main()
{
    float d = texelFetch(depthTexture, ivec2(gl_FragCoord.xy), 0).r;
    result = clipInfo[0] / (clipInfo[1] * d + clipInfo[2]);
}
#endif
