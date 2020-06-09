#if defined VERT

#include "worldinfo.glsl"

layout(location = 0) uniform vec4 points[4];

layout(location = 0) out vec2 outTexCoord;
#ifdef BLUR
layout(location = 2) out vec2 outScreenTexCoord;
#endif

void main()
{
    gl_Position = orthoProjection * vec4(points[gl_VertexID].xy, 0.0, 1.0);
#ifdef BLUR
    outScreenTexCoord = (gl_Position.xy + 1.0) * 0.5;
#endif
    outTexCoord = points[gl_VertexID].zw;
}

#elif defined FRAG

#ifdef BLUR
layout(binding = 0) uniform sampler2D texBlurBg;
#endif
layout(binding = 1) uniform sampler2D tex;
layout(location = 4) uniform vec4 uColor;
layout(location = 5) uniform float uAlpha;
#ifdef GRADIENT
layout(location = 6) uniform vec4 uColor2;
#endif

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 inTexCoord;
#ifdef BLUR
layout(location = 2) in vec2 inScreenTexCoord;
#endif

void main()
{
#ifdef GRADIENT
    vec4 color = mix(uColor, uColor2, inTexCoord.x);
#else
    vec4 color = uColor;
#endif

#ifdef COLOR
    outColor = color * texture(tex, inTexCoord);
#elif defined BLUR
    vec4 texColor = texture(tex, inTexCoord);
    vec4 c = color * texColor;
    outColor = vec4(mix(texture(texBlurBg, inScreenTexCoord).rgb, c.rgb, c.a), texColor.a * uAlpha);
#else
    outColor = color * vec4(1.0, 1.0, 1.0, texture(tex, inTexCoord).r);
#endif
}

#endif
