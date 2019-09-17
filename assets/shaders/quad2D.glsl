#if defined VERT

#include "worldinfo.glsl"

layout(location = 0) uniform vec4 points[4];
layout(location = 4) uniform vec4 color;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec2 outTexCoord;

void main()
{
    gl_Position = orthoProjection * vec4(points[gl_VertexID].xy, 0.0, 1.0);
    outColor = color;
    outTexCoord = points[gl_VertexID].zw;
}

#elif defined FRAG

#ifdef BLUR
layout(binding = 0) uniform sampler2D texRenderColor;
#endif
layout(binding = 1) uniform sampler2D tex;

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec4 inColor;
layout(location = 1) in vec2 inTexCoord;

void main()
{
#ifdef COLOR
    outColor = inColor * texture(tex, inTexCoord);
#elif defined BLUR
    outColor = inColor * texture(tex, inTexCoord);
#else
    outColor = inColor * vec4(1.0, 1.0, 1.0, texture(tex, inTexCoord).r);
#endif
}

#endif
