#if defined VERT

#include "worldinfo.glsl"

layout(location = 0) uniform vec4 points[4];
layout(location = 4) uniform vec4 color;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec2 outTexCoord;
#ifdef BLUR
layout(location = 2) out vec2 outScreenTexCoord;
#endif

void main()
{
    gl_Position = orthoProjection * vec4(points[gl_VertexID].xy, 0.0, 1.0);
#ifdef BLUR
    outScreenTexCoord = (gl_Position.xy + 1.0) * 0.5;
#endif
    outColor = color;
    outTexCoord = points[gl_VertexID].zw;
}

#elif defined FRAG

#ifdef BLUR
layout(binding = 0) uniform sampler2D texBlurBg;
#endif
#ifdef TEXARRAY
layout(binding = 1) uniform sampler2DArray tex;
#else
layout(binding = 1) uniform sampler2D tex;
#endif

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec4 inColor;
layout(location = 1) in vec2 inTexCoord;
#ifdef BLUR
layout(location = 2) in vec2 inScreenTexCoord;
#endif

void main()
{
#ifdef COLOR
    outColor = inColor * texture(tex, inTexCoord);
#elif defined TEXARRAY
    outColor = inColor * texture(tex, vec3(inTexCoord, 0));
#elif defined BLUR
    vec4 color = inColor * texture(tex, inTexCoord);
    outColor = vec4(mix(texture(texBlurBg, inScreenTexCoord).rgb, color.rgb, color.a), 1.0);
#else
    outColor = inColor * vec4(1.0, 1.0, 1.0, texture(tex, inTexCoord).r);
#endif
}

#endif
